#pragma once

#include "WsoConfig2.h"

namespace wso_config
{
	class WSOCONFIG_DLL_API SystemCaliber
	{
	public:
		SystemCaliber();
		virtual ~SystemCaliber();

		SystemCaliber(const SystemCaliber& rhs) = delete;
		SystemCaliber& operator=(const SystemCaliber& rhs) = delete;

		static SystemCaliber* getInstance(void);
		static std::mutex singleMutex_;

	public:
		void setupBoardProfile(HbsDataProfile* profile);

		double dispersionParameterToRetina(int index, bool isSet = false, double value = 0);
		double dispersionParameterToCornea(int index, bool isSet = false, double value = 0);
		double spectrometerParameter(int index, bool isSet = false, double value = 0);
		float galvanometerOffsetX(bool isset = false, float value = 0);
		float galvanometerOffsetY(bool isset = false, float value = 0);
		float galvanometerScaleX(bool isset = false, float value = 0);
		float galvanometerScaleY(bool isset = false, float value = 0);
	
	protected:
		HbsDataProfile* getBoardProfile(void) const;

	private:
		struct SystemCalibImpl;
		std::unique_ptr<SystemCalibImpl> d_ptr;
		SystemCalibImpl& impl(void) const;
	};
}
