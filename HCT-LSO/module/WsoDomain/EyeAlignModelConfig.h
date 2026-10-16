#pragma once

#include <cstdint>
#include <string>

// Pupil-centre network settings of the EyeAlign detector.
//
// NATIVE ONLY. Unlike the structs in EyeAlignDefines.h this type holds a std::string and
// never crosses the P/Invoke boundary: it is read from / written to the <Detector> element
// of the "AlignmentSetting" group in WsoConfig.xml and handed to EyeAligner::setModelConfig().

namespace wso_domain
{
	struct PupilModelConfig
	{
		// ONNX model file. A relative path resolves against the working directory, like
		// WsoConfig.xml itself. The build copies the model next to the executables.
		std::string modelPath = "models/pupil_track_engine.onnx";

		// The model's "presence" output (probability that a pupil is visible) must reach
		// this value for a detection to count as found. 0.9 (rather than the 0.5 the model
		// was exported with) keeps marginal, low-confidence centres out of the control loop.
		float presenceThreshold = 0.9f;

		// ONNX Runtime intra-op thread count. The network is small; a couple of threads
		// halve the latency while leaving the other cores to acquisition and processing.
		int32_t intraOpThreads = 2;
	};
}
