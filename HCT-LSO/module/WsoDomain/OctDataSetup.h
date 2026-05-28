#pragma once

#include "WsoDomain2.h"

namespace wso_domain
{
	class WSODOMAIN_DLL_API OctDataSetup
	{
	public:
		OctDataSetup();
		virtual ~OctDataSetup();

	public:
		static double getRetinaScanAxialResolution(void);
		static double getCorneaScanAxialResolution(void);
		static double getInAirScanAxialResolution(void);

		static void setRetinaScanAxialResolution(double resol);
		static void setCorneaScanAxialResolution(double resol);
		static void setInAirScanAxialResolution(double resol);


	private:
		struct OctDataSetupImpl;
		static std::unique_ptr<OctDataSetupImpl> d_ptr;
		static OctDataSetupImpl& impl(void) ;
	};
}
