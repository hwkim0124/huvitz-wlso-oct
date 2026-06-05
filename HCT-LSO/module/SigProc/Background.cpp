#include "pch.h"
#include "Background.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <iterator>
#include <format>


using namespace sig_proc;



struct Background::BackgroundImpl
{
	unsigned short data[LINE_CAMERA_CCD_PIXELS];
	bool empty;

	BackgroundImpl() : data { 0 }, empty(true)
	{
	}
};


Background::Background() :
	d_ptr(make_unique<BackgroundImpl>())
{
}


Background::~Background()
{
}


sig_proc::Background::Background(Background && rhs) = default;
Background & sig_proc::Background::operator=(Background && rhs) = default;


void sig_proc::Background::makeBackgroundSpectrum(const unsigned short * data, int width, int height)
{
	if (data == nullptr || width != LINE_CAMERA_CCD_PIXELS) {
		return;
	}

	unsigned int buff[LINE_CAMERA_CCD_PIXELS] = { 0 };
	unsigned short bgrd[LINE_CAMERA_CCD_PIXELS] = { 0 };
	int padd = height / 10;

	int sidx = padd;
	int eidx = height - padd;
	int size = (eidx - sidx + 1);

	// Make an average axial scan profile from resampled data.
	for (int i = 0; i < width; i++) {
		for (int j = sidx; j < eidx; j++) {
			buff[i] += data[i + j * width];
		}
		bgrd[i] = (unsigned short)(buff[i] / size);
	}

	setProfileData(bgrd);
	return;
}


void sig_proc::Background::setProfileData(const unsigned short * data, int size)
{
	memcpy(getImpl().data, data, size * sizeof(unsigned short));
	getImpl().empty = false;
	return;
}


unsigned short * sig_proc::Background::getProfileData(void) const
{
	return getImpl().data;
}


bool sig_proc::Background::isEmpty(void) const
{
	return getImpl().empty;
}


void sig_proc::Background::setEmpty(bool flag) 
{
	getImpl().empty = flag;
	return;
}


void sig_proc::Background::dumpToFile(void)
{
	std::string path = std::format(".//{}", BACKGROUND_FILE_NAME);

	std::ofstream file(path, std::ios::out | std::ofstream::binary);
	file.write((char *)getProfileData(), sizeof(unsigned short)*LINE_CAMERA_CCD_PIXELS);
	file.close();
	return;
}


Background::BackgroundImpl & sig_proc::Background::getImpl(void) const
{
	return *d_ptr;
}
