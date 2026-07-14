#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API OctAntLensMotor : public StepMotor
	{
	public:
		OctAntLensMotor();
		OctAntLensMotor(MainBoard* board);
		virtual ~OctAntLensMotor();

		OctAntLensMotor(OctAntLensMotor&& rhs);
		OctAntLensMotor& operator=(OctAntLensMotor&& rhs);
		OctAntLensMotor(const OctAntLensMotor& rhs);
		OctAntLensMotor& operator=(const OctAntLensMotor& rhs);

	public:
		bool initializeOctAntLensMotor(void);

		virtual bool updatePositionToOrigin(int mode) override;
		virtual bool setCurrentPositionAsOrigin(int mode) override;

		bool updatePositionToMirrorIn(void);
		bool updatePositionToMirrorOut(void);
		void setCurrentPositionAsMirrorIn(void);
		void setCurrentPositionAsMirrorOut(void);

		virtual int getPositionOfMirrorIn(void) const;
		virtual int getPositionOfMirrorOut(void) const;
		virtual void setPositionOfMirrorIn(int pos);
		virtual void setPositionOfMirrorOut(int pos);

		virtual bool loadCalibParamFromProfile(void) override;
		virtual bool saveCalibParamToProfile(void) override;

	private:
		struct OctAntLensMotorImpl;
		std::unique_ptr<OctAntLensMotorImpl> d_ptr;
		OctAntLensMotorImpl& impl(void) const;
	};
}

