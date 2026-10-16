#pragma once

#include "EyeAlign2.h"


namespace eye_align
{
	// One ONNX Runtime (CPU) session of the pupil-centre network ("UNextPupil"), shared
	// by every PupilDetector and by the offline detection path.
	//
	// Model contract
	//   input  "input"    float32 [1,1,192,256] NCHW: the whole 8-bit frame, resampled with
	//                     an antialiased bilinear filter and normalised (v / 255 - 0.5) / 0.5
	//   output "center"   float32 [1,2]: normalised pupil centre (x, y) in 0..1
	//   output "presence" float32 [1]:   probability that a pupil is visible (sigmoid in graph)
	//   output "heatmap"  float32 [1,1,48,64]: NOT requested by this class
	//
	// infer() is serialised by an internal mutex and never throws. The ONNX Runtime headers
	// stay inside the .cpp (pimpl), so users of this class need no ORT include path.
	class PupilCenterModel
	{
	public:
		explicit PupilCenterModel(const PupilModelConfig& config);
		~PupilCenterModel();

		PupilCenterModel(const PupilCenterModel& rhs) = delete;
		PupilCenterModel& operator=(const PupilCenterModel& rhs) = delete;

	public:
		// Creates the session from config.modelPath. Attempted once per object: a failure
		// is logged with one error line and every later load() / infer() returns false.
		bool load(void);
		bool isLoaded(void) const;

		const PupilModelConfig& getConfig(void) const;

		// The presence threshold may change without a reload.
		void setPresenceThreshold(float threshold);
		float getPresenceThreshold(void) const;

		// Network input size, read from the model at load() (256 x 192).
		int getInputWidth(void) const;
		int getInputHeight(void) const;

		// gray: width*height 8-bit pixels, row stride in bytes (0 = width).
		// outNormX / outNormY: normalised centre 0..1; outPresence: 0..1.
		// Returns false when not loaded, on bad arguments or on any runtime failure.
		bool infer(const std::uint8_t* gray, int width, int height, int stride,
			float& outNormX, float& outNormY, float& outPresence);

	private:
		struct PupilCenterModelImpl;
		std::unique_ptr<PupilCenterModelImpl> d_ptr;
		PupilCenterModelImpl& impl(void) const;
	};
}
