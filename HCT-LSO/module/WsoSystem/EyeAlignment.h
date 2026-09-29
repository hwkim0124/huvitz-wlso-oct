#pragma once

#include "WsoSystem2.h"

#include <memory>
#include <mutex>


namespace wso_system
{
	// Facade over eye_align::EyeAligner for the WsoSystem export layer: wires the
	// cornea-camera frame sink, pushes the persisted configuration, and guards the
	// start against a running OCT auto-optimisation.
	class WSOSYSTEM_DLL_API EyeAlignment
	{
	public:
		EyeAlignment();
		virtual ~EyeAlignment();

		EyeAlignment(const EyeAlignment& rhs) = delete;
		EyeAlignment& operator=(const EyeAlignment& rhs) = delete;

		static EyeAlignment* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool initializeEyeAlignment(void);
		void releaseEyeAlignment(void);
		bool isInitialized(void) const;

		// Copies SystemConfig's AlignmentSettings into the aligner.
		bool applyConfiguration(void);

		EyeAlignResult startAlignment(EyeAlignMode mode, EyeSide eyeSide, EyeAlignEventOccurred callback);
		bool cancelAlignment(void);
		bool holdAlignment(void);
		bool resumeAlignment(void);
		bool isHeld(void) const;
		bool isAligning(void) const;
		EyeAlignMode getMode(void) const;
		EyeAlignPhase getPhase(void) const;
		EyeAlignResult getResult(void) const;
		bool fetchStatus(EyeAlignStatus* status) const;

		bool isPupilDetected(IrCameraId camId) const;
		bool getPupilResult(IrCameraId camId, PupilDetectResult* result) const;
		bool detectPupilInImage(const std::uint8_t* gray, int width, int height, IrCameraId camId, PupilDetectResult* result) const;

		bool setPupilOverride(IrCameraId camId, const PupilDetectResult& result);
		bool clearPupilOverride(IrCameraId camId);

	private:
		struct EyeAlignmentImpl;
		std::unique_ptr<EyeAlignmentImpl> d_ptr;
		EyeAlignmentImpl& impl(void) const;
	};
}
