#pragma once

#include "EyeAlign2.h"


namespace eye_align
{
	// Pupil detector for one cornea IR camera (8-bit grayscale frames).
	//
	// The interface is final; the algorithm is not implemented yet. detect() validates
	// its arguments and reports "not found". Detection-tuning parameters (thresholds,
	// radii, morphology) will arrive together with the algorithm in a struct of their
	// own; PupilDetectParam only carries the target point and the error-sign mapping.
	class EYEALIGN_DLL_API PupilDetector
	{
	public:
		PupilDetector();
		explicit PupilDetector(IrCameraId camId, const PupilDetectParam& param = PupilDetectParam());
		virtual ~PupilDetector();

	public:
		void setCameraId(IrCameraId camId);
		IrCameraId getCameraId(void) const;

		void setParam(const PupilDetectParam& param);
		const PupilDetectParam& getParam(void) const;

		// Desired pupil centre for a frame of the given size (-1 in the param = centre).
		float getTargetX(int width) const;
		float getTargetY(int height) const;

		// gray: width*height 8-bit pixels, row stride in bytes (0 = width). Never throws.
		PupilDetectResult detect(const std::uint8_t* gray, int width, int height, int stride = 0, int frameSeq = 0) const;

	private:
		IrCameraId camId_;
		PupilDetectParam param_;
	};
}
