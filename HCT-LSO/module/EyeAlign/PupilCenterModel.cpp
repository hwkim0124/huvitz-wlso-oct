#include "pch.h"
#include "PupilCenterModel.h"
#include "GrayResampler.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <iterator>
#include <string>

// The WsoBoard C type macros (wso_ctypes.h, reached through the precompiled header)
// collide with identifiers in the ONNX Runtime headers; hide them around the includes.
#pragma push_macro("U32")
#undef U32
#include <onnxruntime_cxx_api.h>
#include <onnxruntime_session_options_config_keys.h>
#pragma pop_macro("U32")


using namespace eye_align;


namespace
{
	// Contract of the exported pupil model (see PupilCenterModel.h).
	constexpr int DEFAULT_INPUT_WIDTH = 256;
	constexpr int DEFAULT_INPUT_HEIGHT = 192;
	constexpr float INPUT_MEAN = 0.5f;
	constexpr float INPUT_STD = 0.5f;
	constexpr const char* INPUT_NAME = "input";
	constexpr const char* OUTPUT_CENTER_NAME = "center";
	constexpr const char* OUTPUT_PRESENCE_NAME = "presence";

	// Inferences between two timing summaries in the log (~10 s at 30 fps per camera).
	constexpr int TIMING_LOG_PERIOD = 300;

	using Clock = std::chrono::steady_clock;

	double elapsedMs(Clock::time_point since)
	{
		return std::chrono::duration<double, std::milli>(Clock::now() - since).count();
	}

	void ORT_API_CALL onOrtLogMessage(void* /*param*/, OrtLoggingLevel severity, const char* category,
		const char* /*logid*/, const char* location, const char* message) noexcept
	{
		try {
			const char* cat = category ? category : "";
			const char* msg = message ? message : "";
			if (severity >= ORT_LOGGING_LEVEL_ERROR) {
				LogE() << "ORT[" << cat << "] " << msg << " @ " << (location ? location : "");
			}
			else if (severity == ORT_LOGGING_LEVEL_WARNING) {
				LogW() << "ORT[" << cat << "] " << msg;
			}
			else {
				LogD() << "ORT[" << cat << "] " << msg;
			}
		}
		catch (...) {
		}
	}

	// Process-wide ONNX Runtime environment (ORT keeps one per process). Deliberately
	// leaked, a documented exception to the no-raw-new rule: the environment owns the
	// runtime's global state and destroying it during DLL_PROCESS_DETACH, after the
	// runtime's own threads were torn down, is the classic ORT exit hang. Sessions are
	// released explicitly (EyeAligner::releaseEyeAligner), the environment is not.
	Ort::Env& ortEnvironment(void)
	{
		static Ort::Env* environment = [] {
			auto* created = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "eyealign", &onOrtLogMessage, nullptr);
			created->DisableTelemetryEvents();
			return created;
		}();
		return *environment;
	}

	std::string absolutePathForLog(const std::wstring& path)
	{
		wchar_t buffer[MAX_PATH * 2] = {};
		const DWORD length = GetFullPathNameW(path.c_str(), static_cast<DWORD>(std::size(buffer)), buffer, nullptr);
		if (length == 0 || length >= std::size(buffer)) {
			return cpp_util::wtoa(path);
		}
		return cpp_util::wtoa(std::wstring(buffer, length));
	}
}


struct PupilCenterModel::PupilCenterModelImpl
{
	PupilModelConfig config;

	std::mutex mutex;
	bool loadAttempted = false;
	bool loaded = false;
	bool runFailureLogged = false;
	std::atomic<float> presenceThreshold{ 0.9f };
	int inputWidth = DEFAULT_INPUT_WIDTH;
	int inputHeight = DEFAULT_INPUT_HEIGHT;

	std::unique_ptr<Ort::Session> session;
	Ort::MemoryInfo memoryInfo{ nullptr };
	std::vector<float> input;				// inputHeight * inputWidth, referenced by the input tensor of every Run
	GrayResampler resampler;				// kernels cached for the last source size

	// timing
	std::int64_t inferenceCount = 0;
	int periodCount = 0;
	double periodSumMs = 0.0;
	double periodMaxMs = 0.0;

	explicit PupilCenterModelImpl(const PupilModelConfig& cfg) :
		config(cfg), presenceThreshold(cfg.presenceThreshold)
	{}

	void recordTiming(double ms)
	{
		++inferenceCount;
		if (inferenceCount == 1) {
			LogI() << "PupilCenterModel first inference: " << ms << " ms (includes warm-up)";
			return;
		}
		++periodCount;
		periodSumMs += ms;
		periodMaxMs = std::max(periodMaxMs, ms);
		if (periodCount >= TIMING_LOG_PERIOD) {
			LogI() << "PupilCenterModel: " << inferenceCount << " inferences, last " << periodCount
				<< ": mean " << (periodSumMs / periodCount) << " ms, max " << periodMaxMs << " ms";
			periodCount = 0;
			periodSumMs = 0.0;
			periodMaxMs = 0.0;
		}
	}
};


PupilCenterModel::PupilCenterModel(const PupilModelConfig& config) :
	d_ptr(std::make_unique<PupilCenterModelImpl>(config))
{}


PupilCenterModel::~PupilCenterModel()
{
	try {
		std::lock_guard<std::mutex> lock(impl().mutex);
		impl().session.reset();
	}
	catch (...) {
	}
}


bool eye_align::PupilCenterModel::load(void)
{
	auto& state = impl();
	std::lock_guard<std::mutex> lock(state.mutex);
	if (state.loadAttempted) {
		return state.loaded;
	}
	state.loadAttempted = true;

	const std::wstring path = cpp_util::atow(state.config.modelPath);
	const std::string pathForLog = absolutePathForLog(path);
	const auto start = Clock::now();
	try {
		Ort::SessionOptions options;
		options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
		options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
		options.SetInterOpNumThreads(1);
		options.SetIntraOpNumThreads(std::max(1, state.config.intraOpThreads));
		// Without this the intra-op pool spin-waits between runs and burns a core at idle.
		options.AddConfigEntry(kOrtSessionOptionsConfigAllowIntraOpSpinning, "0");
		options.SetLogSeverityLevel(static_cast<int>(ORT_LOGGING_LEVEL_WARNING));

		auto session = std::make_unique<Ort::Session>(ortEnvironment(), path.c_str(), options);

		// Check the model against the contract this class implements.
		Ort::AllocatorWithDefaultOptions allocator;
		if (session->GetInputCount() != 1) {
			LogE() << "PupilCenterModel: " << pathForLog << " has " << session->GetInputCount() << " inputs, expected 1";
			return false;
		}
		const auto inputName = session->GetInputNameAllocated(0, allocator);
		if (std::strcmp(inputName.get(), INPUT_NAME) != 0) {
			LogE() << "PupilCenterModel: " << pathForLog << " input is '" << inputName.get() << "', expected '" << INPUT_NAME << "'";
			return false;
		}
		const auto shape = session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
		if (shape.size() != 4 || (shape[1] > 0 && shape[1] != 1)) {
			LogE() << "PupilCenterModel: " << pathForLog << " input is not a single-channel NCHW tensor";
			return false;
		}
		const int inputHeight = shape[2] > 0 ? static_cast<int>(shape[2]) : DEFAULT_INPUT_HEIGHT;
		const int inputWidth = shape[3] > 0 ? static_cast<int>(shape[3]) : DEFAULT_INPUT_WIDTH;

		bool hasCenter = false, hasPresence = false;
		for (size_t i = 0; i < session->GetOutputCount(); ++i) {
			const auto outputName = session->GetOutputNameAllocated(i, allocator);
			hasCenter = hasCenter || std::strcmp(outputName.get(), OUTPUT_CENTER_NAME) == 0;
			hasPresence = hasPresence || std::strcmp(outputName.get(), OUTPUT_PRESENCE_NAME) == 0;
		}
		if (!hasCenter || !hasPresence) {
			LogE() << "PupilCenterModel: " << pathForLog << " lacks the '" << OUTPUT_CENTER_NAME << "' / '" << OUTPUT_PRESENCE_NAME << "' outputs";
			return false;
		}

		state.memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
		state.inputWidth = inputWidth;
		state.inputHeight = inputHeight;
		state.input.assign(static_cast<size_t>(inputWidth) * inputHeight, 0.0f);
		state.session = std::move(session);
		state.loaded = true;

		LogI() << "PupilCenterModel loaded " << pathForLog << ", input " << inputWidth << "x" << inputHeight
			<< ", intra-op threads " << std::max(1, state.config.intraOpThreads) << ", " << elapsedMs(start) << " ms";
		return true;
	}
	catch (const Ort::Exception& e) {
		LogE() << "PupilCenterModel failed to load " << pathForLog << ": " << e.what();
	}
	catch (const std::exception& e) {
		LogE() << "PupilCenterModel failed to load " << pathForLog << ": " << e.what();
	}
	return false;
}

bool eye_align::PupilCenterModel::isLoaded(void) const
{
	std::lock_guard<std::mutex> lock(impl().mutex);
	return impl().loaded;
}

const PupilModelConfig& eye_align::PupilCenterModel::getConfig(void) const
{
	return impl().config;
}

void eye_align::PupilCenterModel::setPresenceThreshold(float threshold)
{
	impl().presenceThreshold = threshold;
}

float eye_align::PupilCenterModel::getPresenceThreshold(void) const
{
	return impl().presenceThreshold;
}

int eye_align::PupilCenterModel::getInputWidth(void) const
{
	return impl().inputWidth;
}

int eye_align::PupilCenterModel::getInputHeight(void) const
{
	return impl().inputHeight;
}

bool eye_align::PupilCenterModel::infer(const std::uint8_t* gray, int width, int height, int stride,
	float& outNormX, float& outNormY, float& outPresence)
{
	if (gray == nullptr || width <= 0 || height <= 0) {
		return false;
	}
	if (stride <= 0) {
		stride = width;
	}
	if (stride < width) {
		return false;
	}

	auto& state = impl();
	std::lock_guard<std::mutex> lock(state.mutex);
	if (!state.loaded || !state.session) {
		return false;
	}

	const auto start = Clock::now();
	try {
		if (!state.resampler.matches(width, height, state.inputWidth, state.inputHeight)) {
			state.resampler.configure(width, height, state.inputWidth, state.inputHeight);
		}
		if (!state.resampler.apply(gray, stride, state.input.data(), INPUT_MEAN, INPUT_STD)) {
			return false;
		}

		const std::array<int64_t, 4> shape{ 1, 1, state.inputHeight, state.inputWidth };
		Ort::Value inputTensor = Ort::Value::CreateTensor<float>(state.memoryInfo, state.input.data(), state.input.size(), shape.data(), shape.size());

		const char* const inputNames[] = { INPUT_NAME };
		// The heat map output is deliberately not requested.
		const char* const outputNames[] = { OUTPUT_CENTER_NAME, OUTPUT_PRESENCE_NAME };
		auto outputs = state.session->Run(Ort::RunOptions{ nullptr }, inputNames, &inputTensor, 1, outputNames, 2);

		if (outputs.size() != 2 || !outputs[0].IsTensor() || !outputs[1].IsTensor()
			|| outputs[0].GetTensorTypeAndShapeInfo().GetElementCount() < 2
			|| outputs[1].GetTensorTypeAndShapeInfo().GetElementCount() < 1) {
			if (!state.runFailureLogged) {
				LogE() << "PupilCenterModel: unexpected output tensors";
				state.runFailureLogged = true;
			}
			return false;
		}
		const float* center = outputs[0].GetTensorData<float>();
		outNormX = center[0];
		outNormY = center[1];
		outPresence = outputs[1].GetTensorData<float>()[0];

		state.recordTiming(elapsedMs(start));
		return true;
	}
	catch (const Ort::Exception& e) {
		if (!state.runFailureLogged) {
			LogE() << "PupilCenterModel inference failed: " << e.what();
			state.runFailureLogged = true;
		}
	}
	catch (const std::exception& e) {
		if (!state.runFailureLogged) {
			LogE() << "PupilCenterModel inference failed: " << e.what();
			state.runFailureLogged = true;
		}
	}
	return false;
}

PupilCenterModel::PupilCenterModelImpl& eye_align::PupilCenterModel::impl(void) const
{
	return *d_ptr;
}
