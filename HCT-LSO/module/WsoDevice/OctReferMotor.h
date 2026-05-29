#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;
	class CSliderCtrl;
	class CEdit;

	class WSODEVICE_DLL_API OctReferMotor : public StepMotor
	{
	public:
		OctReferMotor();
		OctReferMotor(MainBoard* board);
		virtual ~OctReferMotor();

		OctReferMotor(OctReferMotor&& rhs);
		OctReferMotor& operator=(OctReferMotor&& rhs);
		OctReferMotor(const OctReferMotor& rhs);
		OctReferMotor& operator=(const OctReferMotor& rhs);

	public:
		virtual bool initializeOctReferMotor(bool ready = true);
		virtual bool updatePositionToOrigin(int mode = 0) override;
		virtual bool setCurrentPositionAsOrigin(int mode) override;

		int getPositionOfRetinaOrigin(void) const;
		int getPositionOfCorneaOrigin(void) const;
		void setPositionOfRetinaOrigin(int pos);
		void setPositionOfCorneaOrigin(int pos);

		void setCurrentPositionAsRetinaOrigin(void);
		void setCurrentPositionAsCorneaOrigin(void);

		bool updatePositionToRetinaOrigin(void);
		bool updatePositionToCorneaOrigin(void);
		bool updatePositionToUpperEnd(void);
		bool updatePositionToLowerEnd(void);

		bool isEndOfLowerPosition(bool inRange = true) const;
		bool isEndOfUpperPosition(bool inRange = true) const;
		bool isAtLowerSideByOrigin(bool isCornea = false) const;
		bool isAtUpperSideByOrigin(bool isCornea = false) const;


		virtual void loadParamsFromProfile(void);
		virtual void saveParamsToProfile(void);

	private:
		struct OctReferMotorImpl;
		std::unique_ptr<OctReferMotorImpl> d_ptr;
		OctReferMotorImpl& impl(void) const;
	};
}
