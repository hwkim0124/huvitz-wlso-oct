#pragma once

#include "WsoDevice2.h"
#include "BoardComponent.h"

#include <memory>
#include <string>

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API SldLaserDriver
	{
	public:
		SldLaserDriver();
		virtual ~SldLaserDriver();

		SldLaserDriver(SldLaserDriver&& rhs) noexcept;
		SldLaserDriver& operator=(SldLaserDriver&& rhs) noexcept;

		// Prevent copy construction and assignment. 
		SldLaserDriver(const SldLaserDriver& rhs) = delete;
		SldLaserDriver& operator=(const SldLaserDriver& rhs) = delete;

	public:
		void initializeSldLaserDriver(MainBoard* mainboard);
		bool isInitialized(void);

		bool getOctSldStatusParam(OctSldStatusParam* status);
		bool getOctSldCalibParam(OctSldCalibParam* calib);
		bool reloadOctSldStatus(void);

		bool updateSystemProfile(void);
		bool reloadSystemProfile(void);

		bool saveSldCalibration(void);
		bool loadSldCalibration(void);

		float getSldCurrent(void);
		float getIpdCurrent(void);
		float getTemperature(void);
		int getExternalPd(void);
		std::string getSerialName(void);

	protected:
		MainBoard* getMainBoard(void) const;

	private:
		struct SldLaserDriverImpl;
		std::unique_ptr<SldLaserDriverImpl> d_ptr;
		SldLaserDriverImpl& impl(void) const;
	};
}

