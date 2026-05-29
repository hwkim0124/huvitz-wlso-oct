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

	class WSODEVICE_DLL_API OctFocusMotor : public StepMotor
	{
	public:
		OctFocusMotor();
		OctFocusMotor(MainBoard* board);
		virtual ~OctFocusMotor();

		OctFocusMotor(OctFocusMotor&& rhs);
		OctFocusMotor& operator=(OctFocusMotor&& rhs);
		OctFocusMotor(const OctFocusMotor& rhs);
		OctFocusMotor& operator=(const OctFocusMotor& rhs);

	public:
		virtual bool initializeOctFocusMotor(bool ready = true) ;	

		virtual bool updatePositionByValue(float value) override;
		virtual bool updatePositionByValueOffset(float offset) override;
		virtual bool updatePositionToOrigin(int mode = 0) override;
		virtual bool setCurrentPositionAsOrigin(int mode = 0) override;
		virtual float getCurrentValueByPosition(void) const override;

		virtual int getZeroDiopterPosition(void) const;
		virtual void setZeroDiopterPosition(int pos);

		virtual int getPositionAtValue(float value) const override;
		virtual float getValueAtPosition(int pos) const override;

		int getPositionsPerDiopter(void) const;
		float getCurrentDiopter(void) const;
		void setPositionsPerDiopter(int pos);
		void setCurrentPositionAsZeroDiopter(void);

		bool updatePositionByDiopter(float diopt);
		bool updatePositionByDiopterOffset(float offset);
		bool updatePositionToZeroDiopter(void);

		int convertDiopterToPosition(float diopt) const;
		float convertPositionToDiopter(int pos) const;
		int getDiopterDirection(void) const;

		bool isEndOfPlusDiopter(void) const;
		bool isEndOfMinusDiopter(void) const;

		virtual void loadParamsFromProfile(void);
		virtual void saveParamsToProfile(void);

	private:
		struct OctFocusMotorImpl;
		std::unique_ptr<OctFocusMotorImpl> d_ptr;
		OctFocusMotorImpl& impl(void) const;
	};
}

