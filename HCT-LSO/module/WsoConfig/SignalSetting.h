#pragma once

#include "WsoConfig2.h"

#include <memory>


namespace wso_config
{
	class WSOCONFIG_DLL_API SignalSetting
	{
	public:
		SignalSetting();
		virtual ~SignalSetting();

		SignalSetting(SignalSetting&& rhs);
		SignalSetting& operator=(SignalSetting&& rhs);
		SignalSetting(const SignalSetting& rhs);
		SignalSetting& operator=(const SignalSetting& rhs);

	public:
		void initializeSignalSetting(void);
		void resetToDefaultValues(void);

		double getWavelengthParameter(int index) const;
		void getWavelengthParameter(double* param) const;
		double getDispersionParameter(int index) const;
		void getDispersionParameter(double* param) const;
		void setWavelengthParameter(int index, double value);
		void setDispersionParameter(int index, double value);


	protected:
		struct SignalSettingImpl;
		std::unique_ptr<SignalSettingImpl> d_ptr;
		SignalSettingImpl& getImpl(void) const;
	};
}
