#pragma once

#include "WsoConfig2.h"

#include <memory>


namespace wso_config
{
	class WSOCONFIG_DLL_API OctScanSettings
	{
	public:
		OctScanSettings();
		virtual ~OctScanSettings();

		OctScanSettings(OctScanSettings&& rhs);
		OctScanSettings& operator=(OctScanSettings&& rhs);
		OctScanSettings(const OctScanSettings& rhs);
		OctScanSettings& operator=(const OctScanSettings& rhs);

	public:
		void initializeOctScanSettings(void);
		void resetToDefaultValues(void);

		double getWavelengthParameter(int index) const;
		void getWavelengthParameter(double* param) const;
		double getDispersionParameter(int index) const;
		void getDispersionParameter(double* param) const;
		void setWavelengthParameter(int index, double value);
		void setDispersionParameter(int index, double value);


	protected:
		struct OctScanSettingsImpl;
		std::unique_ptr<OctScanSettingsImpl> d_ptr;
		OctScanSettingsImpl& getImpl(void) const;
	};
}
