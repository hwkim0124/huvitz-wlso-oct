#include "pch.h"
#include "OctDataSetup.h"
#include "OctDataDefines.h"

using namespace wso_domain;



struct OctDataSetup::OctDataSetupImpl
{
	double corneaPixResol;
	double retinaPixResol;
	double inAirPixResol;

	OctDataSetupImpl()
	{
		corneaPixResol = CORNEA_SCAN_PIXEL_RESOLUTION;
		retinaPixResol = RETINA_SCAN_PIXEL_RESOLUTION;
		inAirPixResol = INAIR_SCAN_PIXEL_RESOLUTION;
	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<OctDataSetup::OctDataSetupImpl> OctDataSetup::d_ptr(new OctDataSetupImpl());



wso_domain::OctDataSetup::OctDataSetup()
{
}


wso_domain::OctDataSetup::~OctDataSetup()
{
}


double wso_domain::OctDataSetup::getRetinaScanAxialResolution(void)
{
	return impl().retinaPixResol;
}

double wso_domain::OctDataSetup::getCorneaScanAxialResolution(void)
{
	return impl().corneaPixResol;
}

double wso_domain::OctDataSetup::getInAirScanAxialResolution(void)
{
	return impl().inAirPixResol;
}

void wso_domain::OctDataSetup::setRetinaScanAxialResolution(double resol)
{
	impl().retinaPixResol = ((resol < 0.0 || resol > 10.0) ? RETINA_SCAN_PIXEL_RESOLUTION : resol);
}

void wso_domain::OctDataSetup::setCorneaScanAxialResolution(double resol)
{
	impl().corneaPixResol = ((resol < 0.0 || resol > 10.0) ? CORNEA_SCAN_PIXEL_RESOLUTION : resol);
}

void wso_domain::OctDataSetup::setInAirScanAxialResolution(double resol)
{
	impl().inAirPixResol = ((resol < 0.0 || resol > 10.0) ? INAIR_SCAN_PIXEL_RESOLUTION : resol);
}


OctDataSetup::OctDataSetupImpl& wso_domain::OctDataSetup::impl(void)
{
	return *d_ptr;
}
