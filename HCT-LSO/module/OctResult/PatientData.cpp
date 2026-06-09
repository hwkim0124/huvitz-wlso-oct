#include "pch.h"
#include "PatientData.h"

using namespace oct_result;


struct PatientData::PatientDataImpl
{
	OctPatient data;

	PatientDataImpl()
	{
	}
};


PatientData::PatientData() :
	d_ptr(make_unique<PatientDataImpl>())
{
}


oct_result::PatientData::PatientData(OctPatient data) :
	d_ptr(make_unique<PatientDataImpl>())
{
	d_ptr->data = data;
	return;
}


oct_result::PatientData::~PatientData() = default;
oct_result::PatientData::PatientData(PatientData && rhs) = default;
PatientData & oct_result::PatientData::operator=(PatientData && rhs) = default;


oct_result::PatientData::PatientData(const PatientData & rhs)
	: d_ptr(make_unique<PatientDataImpl>(*rhs.d_ptr))
{
}


PatientData & oct_result::PatientData::operator=(const PatientData & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_result::PatientData::setPatient(const OctPatient& data)
{
	d_ptr->data = data;
	return;
}


OctPatient & oct_result::PatientData::getPatient(void) const
{
	return d_ptr->data;
}


PatientData::PatientDataImpl & oct_result::PatientData::getImpl(void) const
{
	return *d_ptr;
}
