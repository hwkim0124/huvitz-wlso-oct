#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;


	class WSODEVICE_DLL_API OctPolarMotor : public StepMotor
	{
	public:
		OctPolarMotor();
		OctPolarMotor(MainBoard* board);
		virtual ~OctPolarMotor();

		OctPolarMotor(OctPolarMotor&& rhs);
		OctPolarMotor& operator=(OctPolarMotor&& rhs);
		OctPolarMotor(const OctPolarMotor& rhs);
		OctPolarMotor& operator=(const OctPolarMotor& rhs);

	public:
		virtual bool initializeOctPolarMotor(bool ready = true);

		virtual bool updatePositionByValue(float value) override;
		virtual bool updatePositionByValueOffset(float offset) override;
		virtual bool updatePositionToOrigin(int mode = 0) override;
		virtual bool setCurrentPositionAsOrigin(int mode = 0) override;
		virtual float getCurrentValueByPosition(void) const override;

		virtual int getPositionAtValue(float value) const override;
		virtual float getValueAtPosition(int pos) const override;

		bool updatePositionByDegree(int degree);
		bool updatePositionByDegreeOffset(int offset);
		bool updatePositionToZeroDegree(void);

		int getPositionOfZeroDegree(void) const;
		void setPositionOfZeroDegree(int pos);
		void setCurrentPositionAsZeroDegree(void);

		float getCurrentDegree(void) const;
		int getPositionsPerDegree(void) const;
		void setPositionsPerDegree(int pos);

		int convertDegreeToPosition(float diopt) const;
		float convertPositionToDegree(int pos) const;
		int getDegreeDirection(void) const;

		virtual void loadParamsFromProfile(void);
		virtual void saveParamsToProfile(void);

	private:
		struct OctPolarMotorImpl;
		std::unique_ptr<OctPolarMotorImpl> d_ptr;
		OctPolarMotorImpl& impl(void) const;
	};
}

