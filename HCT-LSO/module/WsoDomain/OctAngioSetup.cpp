#include "pch.h"
#include "OctAngioSetup.h"


using namespace wso_domain;


struct OctAngioSetup::OctAngioSetupImpl {

	float decorrThreshold;
	float motionThreshold;
	float motionOverPoints;
	float motionDistanceRatio;
	int averageOffset;

	int filterOrients;
	float filterSigma;
	float filterDivider;
	float filterWeight;
	float biasFieldSigma;

	OctAngioSetupImpl() {
		initializeOctAngioSetupImpl();
	}

	void initializeOctAngioSetupImpl() {
		decorrThreshold = 0.0f;
		motionThreshold = 1.5f;
		motionOverPoints = 0.5f;
		motionDistanceRatio = 1.15f;
		averageOffset = 3;

		filterOrients = 12;
		filterSigma = 3.0f;
		filterDivider = 1.5f;
		filterWeight = 0.25f;
		biasFieldSigma = 15.0f;
	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<OctAngioSetup::OctAngioSetupImpl> OctAngioSetup::d_ptr(new OctAngioSetupImpl());


OctAngioSetup::OctAngioSetup()
{
}


OctAngioSetup::~OctAngioSetup()
{
}

float wso_domain::OctAngioSetup::getDecorrThreshold(void)
{
	return impl().decorrThreshold;
}

float wso_domain::OctAngioSetup::getMotionThreshold(void)
{
	return impl().motionThreshold;
}

float wso_domain::OctAngioSetup::getMotionOverPoints(void)
{
	return impl().motionOverPoints;
}

float wso_domain::OctAngioSetup::getMotionDistanceRatio(void)
{
	return impl().motionDistanceRatio;
}

int wso_domain::OctAngioSetup::getAverageOffset(void)
{
	return impl().averageOffset;
}

int wso_domain::OctAngioSetup::getFilterOrients(void)
{
	return impl().filterOrients;
}

float wso_domain::OctAngioSetup::getFilterSigma(void)
{
	return impl().filterSigma;
}

float wso_domain::OctAngioSetup::getFilterDivider(void)
{
	return impl().filterDivider;
}

float wso_domain::OctAngioSetup::getFilterWeight(void)
{
	return impl().filterWeight;
}

float wso_domain::OctAngioSetup::getBiasFieldSigma(void)
{
	return impl().biasFieldSigma;
}

void wso_domain::OctAngioSetup::setDecorrThreshold(float val)
{
	impl().decorrThreshold = val;
}

void wso_domain::OctAngioSetup::setMotionThreshold(float val)
{
	impl().motionThreshold = val;
}

void wso_domain::OctAngioSetup::setMotionOverPoints(float val)
{
	impl().motionOverPoints = val;
}

void wso_domain::OctAngioSetup::setMotionDistanceRatio(float val)
{
	impl().motionDistanceRatio = val;
}

void wso_domain::OctAngioSetup::setAverageOffset(int val)
{
	impl().averageOffset = val;
}

void wso_domain::OctAngioSetup::setFilterOrients(int val)
{
	impl().filterOrients = val;
}

void wso_domain::OctAngioSetup::setFilterSigma(float val)
{
	impl().filterSigma = val;
}

void wso_domain::OctAngioSetup::setFilterDivider(float val)
{
	impl().filterDivider = val;
}

void wso_domain::OctAngioSetup::setFilterWeight(float val)
{
	impl().filterWeight = val;
}

void wso_domain::OctAngioSetup::setBiasFieldSigma(float val)
{
	impl().biasFieldSigma = val;
}


OctAngioSetup::OctAngioSetupImpl& wso_domain::OctAngioSetup::impl(void)
{
	return *d_ptr;
}
