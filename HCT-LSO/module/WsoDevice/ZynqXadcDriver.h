#pragma once


#include "WsoDevice2.h"
#include "BoardComponent.h"

#include <memory>
#include <string>

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API ZynqXadcDriver
	{
	public:
		ZynqXadcDriver();
		virtual ~ZynqXadcDriver();

		ZynqXadcDriver(ZynqXadcDriver&& rhs) noexcept;
		ZynqXadcDriver& operator=(ZynqXadcDriver&& rhs) noexcept;

		// Prevent copy construction and assignment. 
		ZynqXadcDriver(const ZynqXadcDriver& rhs) = delete;
		ZynqXadcDriver& operator=(const ZynqXadcDriver& rhs) = delete;

	public:
		void initializeZynqXadcDriver(MainBoard* mainboard);

		bool fetchZynqXadcStatus(AdcSensorStatus* status);
		bool reloadZynqXadcStatus(void);

		float getCpuTemperature(void) const;
		int getExternalLdMonPd(void) const;
		int getLensHallSensor1(void) const;
		int getLensHallSensor2(void) const;
		int getOdOs(void) const;



	protected:
		MainBoard* getMainBoard(void) const;

	private:
		struct ZynqXadcDriverImpl;
		std::unique_ptr<ZynqXadcDriverImpl> d_ptr;
		ZynqXadcDriverImpl& impl(void) const;
	};
}

