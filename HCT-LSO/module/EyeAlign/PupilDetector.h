#pragma once

#include "EyeAlign2.h"


namespace eye_align
{
	class PupilCenterModel;

	// Pupil detector for one cornea IR camera (8-bit grayscale frames).
	//
	// The centre comes from the shared pupil-centre network (PupilCenterModel, ONNX
	// Runtime): the frame is resampled to the network input, the normalised centre is
	// scaled back to image pixels and the network's presence probability becomes the
	// score; "found" is presence >= the configured threshold (0.9 by default). The model has no radius
	// output, so radius stays 0. Without a model attached detect() reports "not found".
	// Detection tuning (model path, threshold) lives in PupilModelConfig; PupilDetectParam
	// only carries the target point and the error-sign mapping.
	class EYEALIGN_DLL_API PupilDetector
	{
	public:
		PupilDetector();
		explicit PupilDetector(IrCameraId cameraId, const PupilDetectParam& param = PupilDetectParam());
		virtual ~PupilDetector();

	public:
		void setCameraId(IrCameraId cameraId);
		IrCameraId getCameraId(void) const;

		void setParam(const PupilDetectParam& param);
		const PupilDetectParam& getParam(void) const;

		// The shared inference model. nullptr detaches (detect() then reports "not found").
		void setModel(std::shared_ptr<PupilCenterModel> model);
		std::shared_ptr<PupilCenterModel> getModel(void) const;

		// Desired pupil centre for a frame of the given size (-1 in the param = centre).
		float getTargetX(int width) const;
		float getTargetY(int height) const;

		// gray: width*height 8-bit pixels, row stride in bytes (0 = width). Never throws.
		PupilDetectResult detect(const std::uint8_t* gray, int width, int height, int stride = 0, int frameSequence = 0) const;

	private:
		IrCameraId cameraId_;
		PupilDetectParam param_;
		std::shared_ptr<PupilCenterModel> model_;
	};
}
