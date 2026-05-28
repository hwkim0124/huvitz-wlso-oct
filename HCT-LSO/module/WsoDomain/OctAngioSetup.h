#pragma once

#include "WsoDomain2.h"


namespace wso_domain
{
	class WSODOMAIN_DLL_API OctAngioSetup
	{
	public:
		OctAngioSetup();
		virtual ~OctAngioSetup();

	public:
		static float getDecorrThreshold(void);
		static float getMotionThreshold(void);
		static float getMotionOverPoints(void);
		static float getMotionDistanceRatio(void);

		static int getAverageOffset(void);
		static int getFilterOrients(void);
		static float getFilterSigma(void);
		static float getFilterDivider(void);
		static float getFilterWeight(void);
		static float getBiasFieldSigma(void);

		static void setDecorrThreshold(float val);
		static void setMotionThreshold(float val);
		static void setMotionOverPoints(float val);
		static void setMotionDistanceRatio(float val);

		static void setAverageOffset(int val);
		static void setFilterOrients(int val);
		static void setFilterSigma(float val);
		static void setFilterDivider(float val);
		static void setFilterWeight(float val);
		static void setBiasFieldSigma(float val);


	private:
		struct OctAngioSetupImpl;
		static std::unique_ptr<OctAngioSetupImpl> d_ptr;
		static OctAngioSetupImpl& impl(void);
	};
}