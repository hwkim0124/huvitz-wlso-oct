#include "pch.h"
#include "RetParam2.h"
#include "EnfaceChart.h"

using namespace ret_param;
using namespace cv;


struct EnfaceChart::EnfaceChartImpl
{
	const EnfaceData* enfaceData;

	EyeSide eyeSide;
	float chartPosX;
	float chartPosY;

	EnfaceChartImpl() : enfaceData(nullptr), chartPosX(0.0f), chartPosY(0.0f), eyeSide(EyeSide::OD)
	{
	}
};


EnfaceChart::EnfaceChart() :
	d_ptr(make_unique<EnfaceChartImpl>())
{
}


ret_param::EnfaceChart::~EnfaceChart() = default;
ret_param::EnfaceChart::EnfaceChart(EnfaceChart && rhs) = default;
EnfaceChart & ret_param::EnfaceChart::operator=(EnfaceChart && rhs) = default;


ret_param::EnfaceChart::EnfaceChart(const EnfaceChart & rhs)
	: d_ptr(make_unique<EnfaceChartImpl>(*rhs.d_ptr))
{
}


EnfaceChart & ret_param::EnfaceChart::operator=(const EnfaceChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void ret_param::EnfaceChart::setChartPosition(float xmm, float ymm)
{
	d_ptr->chartPosX = xmm;
	d_ptr->chartPosY = ymm;
	return;
}


void ret_param::EnfaceChart::setEyeSide(EyeSide side)
{
	d_ptr->eyeSide = side;
	return;
}


void ret_param::EnfaceChart::setEnfaceData(const EnfaceData * data)
{
	d_ptr->enfaceData = data;
	return;
}


EyeSide ret_param::EnfaceChart::eyeSide(void) const
{
	return d_ptr->eyeSide;
}


float ret_param::EnfaceChart::positionX(void) const
{
	return d_ptr->chartPosX;
}


float ret_param::EnfaceChart::positionY(void) const
{
	return d_ptr->chartPosY;
}


const EnfaceData * ret_param::EnfaceChart::enfaceData(void)
{
	return d_ptr->enfaceData;
}


bool ret_param::EnfaceChart::isEmpty(void) const
{
	return (d_ptr->enfaceData == nullptr);
}


bool ret_param::EnfaceChart::updateContent(void)
{
	return false;
}


bool ret_param::EnfaceChart::updateContent(const std::vector<float>& thicks)
{
	return false;
}

