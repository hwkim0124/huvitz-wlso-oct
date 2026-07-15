#pragma once

#include "WsoDevice2.h"
#include "BoardComponent.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API LightLed : public BoardComponent
	{
	public:
		LightLed();
		LightLed(MainBoard* board, LightType type);
		virtual ~LightLed();

		LightLed(LightLed&& rhs);
		LightLed& operator=(LightLed&& rhs);
		LightLed(const LightLed& rhs);
		LightLed& operator=(const LightLed& rhs);

	public:
		virtual bool initializeLightLed(void);

		virtual bool setIntensity(unsigned short value);
		virtual unsigned short getIntensity(void) const;
		virtual bool setLightMode(unsigned short value, bool setBoard = true);
		virtual unsigned short getLightMode(void) const;

		unsigned short getValueSet1(void) const;
		unsigned short getValueSet2(void) const;

		const char* getName(void) const;
		LightType getType(void) const;

		bool isLightOn(void);
		bool turnLightOn(void);
		bool turnLightOff(void);
		bool control(bool flag);

		virtual bool loadCalibParamFromProfile(void) override;
		virtual bool saveCalibParamToProfile(void) override;
		virtual bool loadConfigFromIniFile(void) override;
		virtual bool saveConfigToIniFile(void) override;

		bool updateIntensity(int value);
		bool updateIntensityByOffset(int offset);

	protected:
		std::uint8_t getLightLedId(void) const;
		std::uint16_t getInitialValue(void) const;
		std::uint16_t getInitialMode(void) const;

	private:
		struct LightLedImpl;
		std::unique_ptr<LightLedImpl> d_ptr;
		LightLedImpl& impl(void) const;
	};
}

