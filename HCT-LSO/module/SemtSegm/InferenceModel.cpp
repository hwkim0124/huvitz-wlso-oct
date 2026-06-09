#include "pch.h"
#include "SemtSegm2.h"
#include "InferenceModel.h"
#include "InferenceInput.h"
#include "InferenceResult.h"

#include <inference_engine.hpp>

using namespace semt_segm;
using namespace cv;
using namespace InferenceEngine;


#if defined(ENABLE_UNICODE_PATH_SUPPORT) && defined(_WIN32)
	#define tcout std::wcout
	#define file_name_t std::wstring
	#define imread_t imreadW
	#define ClassificationResult_t ClassificationResultW
#else
	#define tcout std::cout
	#define file_name_t std::string
	#define imread_t cv::imread
	#define ClassificationResult_t ClassificationResult
#endif


#if defined(ENABLE_UNICODE_PATH_SUPPORT) && defined(_WIN32)
std::string simpleConvert(const std::wstring & wstr) {
	std::string str;
	for (auto && wc : wstr)
		str += static_cast<char>(wc);
	return str;
}
#endif 


struct InferenceModel::InferenceModelImpl
{
	Core ieCore;
	CNNNetwork network;
	ExecutableNetwork executable_network;

	InferenceEngine::InputsDataMap input_info;
	InferenceEngine::OutputsDataMap output_info;

	bool initialized = false;

	InferenceModelImpl() 
	{
	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<InferenceModel::InferenceModelImpl> InferenceModel::d_ptr(new InferenceModelImpl());


InferenceModel::InferenceModel()
{
}


InferenceModel::~InferenceModel()
{
}

bool semt_segm::InferenceModel::initializeNetwork()
{
	if (isInitialized()) {
		return true;
	}

	getImpl().initialized = false;

	try {
		std::vector<std::string> availableDevices = getImpl().ieCore.GetAvailableDevices();

		const file_name_t input_model{ L"saved_model.xml" };

		CNNNetwork network = getImpl().ieCore.ReadNetwork(input_model);

		/** Take information about all topology inputs **/
		InferenceEngine::InputsDataMap input_info = network.getInputsInfo();
		/** Take information about all topology outputs **/
		InferenceEngine::OutputsDataMap output_info = network.getOutputsInfo();

		if (network.getOutputsInfo().size() != 1) {
			throw std::logic_error("Sample supports topologies with 1 output only");
		}
		if (network.getInputsInfo().size() != 1) {
			throw std::logic_error("Sample supports topologies with 1 input only");
		}

		/** Iterate over all input info**/
		for (auto &item : input_info) {
			auto input_data = item.second;
			input_data->setPrecision(InferenceEngine::Precision::FP32);
			input_data->setLayout(InferenceEngine::Layout::NCHW);
			// input_data->getPreProcess().setResizeAlgorithm(InferenceEngine::RESIZE_BILINEAR);
			// input_data->getPreProcess().setColorFormat(InferenceEngine::ColorFormat::RGB);
		}

		/** Iterate over all output info**/
		for (auto &item : output_info) {
			auto output_data = item.second;
			output_data->setPrecision(InferenceEngine::Precision::FP32);
			// output_data->setLayout(InferenceEngine::Layout::NC);
		}

		getImpl().executable_network = getImpl().ieCore.LoadNetwork(network, "CPU");
		getImpl().input_info = move(input_info);
		getImpl().output_info = move(output_info);
		LogD() << "Inference model initialized";
	}
	catch (InferenceEngine::details::InferenceEngineException &iee) {
		LogD() << "Inference model initialization failed, " << iee.what();
		return false;
	}
	catch (exception e) {
		LogD() << "Inference model initialization failed, " << e.what();
		return false;
	}

	getImpl().initialized = true;
	return true;
}

bool semt_segm::InferenceModel::isInitialized() 
{
	return getImpl().initialized;
}

bool semt_segm::InferenceModel::requestPrediction(const OcularImage* image)
{
	if (!isInitialized()) {
		return false;
	}

	// Mat matImg = image->getCvMatConst();
	Mat matImg = cv::Mat::zeros(256, 256, CV_32F);

	auto infer_request = getImpl().executable_network.CreateInferRequest();

	/** Iterate over all input blobs **/
	for (auto & item : getImpl().input_info) {
		auto input_name = item.first;
		auto input_data = item.second;

		/** Get input blob **/
		auto input = infer_request.GetBlob(input_name);

		auto desc = input->getTensorDesc();
		auto dims = desc.getDims();
		auto layout = desc.getLayout();

		size_t channels = matImg.channels();
		size_t height = matImg.size().height;
		size_t width = matImg.size().width;

		size_t strideH = matImg.step.buf[0];
		size_t strideW = matImg.step.buf[1];
		size_t elemSize = matImg.elemSize();
		auto data = reinterpret_cast<float*>(matImg.data);

		bool is_dense =
			strideW == channels * elemSize &&
			strideH == channels * width * elemSize;

		if (!is_dense) THROW_IE_EXCEPTION
			<< "Doesn't support conversion from not dense cv::Mat";

		InferenceEngine::TensorDesc tDesc(InferenceEngine::Precision::FP32,
										{ 1, channels, height, width },
										InferenceEngine::Layout::NCHW);

		/** Fill input tensor with planes. First b channel, then g and r channels **/
		// assuming input precision was asked to be U8 in prev step
		input = InferenceEngine::make_shared_blob<float>(tDesc, data);
		// input->allocate();
		infer_request.SetBlob(input_name, input);
		break;
	}

	infer_request.Infer();

	for (auto &item : getImpl().output_info) {
		auto output_name = item.first;
		auto output = infer_request.GetBlob(output_name);
		auto desc = output->getTensorDesc();
		{
			auto const memLocker = output->cbuffer(); // use const memory locker
													  // output_buffer is valid as long as the lifetime of memLocker
			const float *output_buffer = memLocker.as<const float *>();
			/** output_buffer[] - accessing output blob data **/

			
		}
		break;
	}

	return true;
}

bool semt_segm::InferenceModel::requestPrediction(const InferenceInput& input, InferenceResult& result, bool maskLabel)
{
	if (!isInitialized()) {
		return false;
	}

	auto infer_request = getImpl().executable_network.CreateInferRequest();

	/** Iterate over all input blobs **/
	for (auto & item : getImpl().input_info) {
		auto input_name = item.first;
		auto input_data = item.second;

		/** Get input blob **/
		auto input_blob = infer_request.GetBlob(input_name);

		auto desc = input_blob->getTensorDesc();
		auto dims = desc.getDims();
		auto layout = desc.getLayout();

		auto blob = input.getInputBlob();
		infer_request.SetBlob(input_name, blob);
		break;
	}

	infer_request.Infer();

	for (auto &item : getImpl().output_info) {
		auto output_name = item.first;
		auto output = infer_request.GetBlob(output_name);
		auto desc = output->getTensorDesc();
		auto dims = desc.getDims();

		{
			auto const memLocker = output->cbuffer(); // use const memory locker
			
			// output_buffer is valid as long as the lifetime of memLocker
			const float *output_buffer = memLocker.as<const float *>();
			/** output_buffer[] - accessing output blob data **/

			// NCHW
			int width = (int)dims[3];
			int height = (int)dims[2];
			int channels = (int)dims[1];
			result.postprocess(output_buffer, height, width, channels, maskLabel);
		}
		break;
	}
	return true;
}


InferenceModel::InferenceModelImpl & semt_segm::InferenceModel::getImpl(void)
{
	return *d_ptr;
}
