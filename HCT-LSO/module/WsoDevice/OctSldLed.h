#pragma once

#include "WsoDevice2.h"
#include "BoardComponent.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API OctSldLed : public BoardComponent
	{
	public:
		OctSldLed();
		OctSldLed(MainBoard* board);
		virtual ~OctSldLed();

		OctSldLed(OctSldLed&& rhs);
		OctSldLed& operator=(OctSldLed&& rhs);
		OctSldLed(const OctSldLed& rhs) = delete;
		OctSldLed& operator=(const OctSldLed& rhs) = delete;

	public:
		bool initializeOctSldLed(void) ;

		bool writeCalibParam_BoardToMemory(void);
		bool fetchCalibParam_MemoryToBoard(void);

		virtual bool loadCalibParamFromProfile(void) override;
		virtual bool saveCalibParamToProfile(void) override;

		bool isLaserOn(void);
		bool turnLaserOn(void);
		bool turnLaserOff(void);
		bool control(int value);

		const char* getName(void) const;
		LaserType getType(void) const;

		std::uint16_t getHighCode(void);
		std::uint16_t getLowCode1(void);
		std::uint16_t getLowCode2(void);
		std::uint16_t getRsiCode(void);

		bool setHighCode(std::uint16_t code);
		bool setLowCode1(std::uint16_t code);
		bool setLowCode2(std::uint16_t code);
		bool setRsiCode(std::uint16_t code);

	private:
		struct OctSldLedImpl;
		std::unique_ptr<OctSldLedImpl> d_ptr;
		OctSldLedImpl& impl(void) const;
	};
}


