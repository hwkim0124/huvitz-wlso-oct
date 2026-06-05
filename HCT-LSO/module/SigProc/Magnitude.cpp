#include "pch.h"
#include "Magnitude.h"

#include <array>

using namespace sig_proc;


struct Magnitude::MagnitudeImpl
{
	float* frameData;
	vector<int> magnHist;
	vector<int> histBgdIndex;
	vector<int> histSigIndex;
	vector<float> histBgdValue;
	vector<float> histSigValue;
	vector<float> snrRatios;

	array<float, 4096> axialData;
	vector<array<float, 4096>> axialDataBuff = vector<array<float, 4096>>(100);
	int axialDataAvgSize = 1;
	int axialDataCount = 0;
	int axialDataIndex = 0;

	int imageRows;
	int imageCols;
	int frameRows;
	int frameCols;
	int dataRows;
	int dataCols;

	int dataMulti;
	int dataIndex;
	int rowOffset;
	int colOffset;

	int peakRangeOffset[2];
	int peakTopIdx;
	int peakHalfIdx[2];
	int peakTermIdx[2];
	float peakTopVal;
	float peakFwhm;
	float peakFloor;

	float maxPeakTopVal;
	float maxPeakSnrRatio;
	float avgPeakSnrRatio;
	float avgPeakFwhm;
	int frameCount;

	float signalVal;
	float noiseVal;
	float peakSnrRatio;
	float noiseFixed = 0.0f;

	int histBinSize;
	int histBinUnit;

	MagnitudeImpl() : frameData(nullptr), 
		frameRows(0), frameCols(0),
		dataRows(0), dataCols(0), dataIndex(-1), dataMulti(1),
		rowOffset(0), colOffset(0), imageRows(0), imageCols(0), 
		peakRangeOffset{ 0 }, peakTopIdx(0), peakHalfIdx{ 0 }, peakTermIdx{ 0 },
		peakTopVal(0.0f), peakFwhm(0.0f), 
		maxPeakTopVal(0.0f), maxPeakSnrRatio(0.0f), avgPeakFwhm(0.0f), frameCount(0),
		signalVal(0.0f), noiseVal(0.0f), peakSnrRatio(0.0f), avgPeakSnrRatio(0.0f),
		histBinSize(256)
	{
	}
};


Magnitude::Magnitude() : 
	d_ptr(make_unique<MagnitudeImpl>())
{
	setAxialDataAverageSize(1);
}


Magnitude::~Magnitude()
{
}


sig_proc::Magnitude::Magnitude(Magnitude && rhs) = default;
Magnitude & sig_proc::Magnitude::operator=(Magnitude && rhs) = default;


bool sig_proc::Magnitude::buildHistogram(int rowStart, int colStart, int rowSize, int colSize, int binSize, int binUnit)
{
	if (getImpl().frameData == nullptr || getImpl().frameRows <= 0 || getImpl().frameCols <= 0) {
		return false;
	}
	if ((rowStart + rowSize) >= getImpl().frameRows ||
		(colStart + colSize) >= getImpl().frameCols) {
		return false;
	}

	//if (colSize != getImpl().imageCols) {
		getImpl().magnHist = vector<int>(colSize * binSize, 0);
	//}

	float data;
	int ibin;

	// Accumulate histogram bins along column major index.
	for (int c = colStart, ix = 0; c < (colStart + colSize); c++, ix++) {
		for (int r = rowStart, iy = 0; r < (rowStart + rowSize); r++, iy++) {
			data = *(getImpl().frameData + r * getImpl().frameCols + c);
			ibin = (int)(data / binUnit);
			ibin = (ibin >= binSize ? binSize - 1 : ibin);
			ibin = (ibin < 0 ? 0 : ibin);
			getImpl().magnHist[ix * binSize + ibin] += 1;
		}
	}

	getImpl().rowOffset = rowStart;
	getImpl().colOffset = colStart;
	getImpl().imageRows = rowSize;
	getImpl().imageCols = colSize;

	getImpl().histBinSize = binSize;
	getImpl().histBinUnit = binUnit;
	return true;
}


bool sig_proc::Magnitude::updateSignalToNoiseRatios(float sigRatio, float bgdRatio)
{
	if (getImpl().imageCols <= 0 || getImpl().histBinSize <= 0) {
		return false;
	}

	getImpl().snrRatios = vector<float>(getImpl().imageCols, 0.0f);
	getImpl().histBgdIndex = vector<int>(getImpl().imageCols, 0);
	getImpl().histSigIndex = vector<int>(getImpl().imageCols, 0);
	getImpl().histBgdValue = vector<float>(getImpl().imageCols, 0.0f);
	getImpl().histSigValue = vector<float>(getImpl().imageCols, 0.0f);

	int sigSize = (int)(sigRatio * getImpl().imageCols);
	int bgdSize = (int)(bgdRatio * getImpl().imageCols);
	int binSize = getImpl().histBinSize;
	int binUnit = getImpl().histBinUnit;

	int nCols = getImpl().imageCols;
	int nSigs = 0;
	int nBgds = 0;
	float snrRate;

	for (int c = 0; c < nCols; c++) {
		nSigs = nBgds = 0;
		for (int i = 0; i < binSize; i++) {
			nBgds += getImpl().magnHist[c*binSize + i];
			if (nBgds >= bgdSize) {
				getImpl().histBgdIndex[c] = i;
				getImpl().histBgdValue[c] = (float)(i * binUnit);
				break;
			}
		}
		for (int i = binSize - 1; i >= 0; i--) {
			nSigs += getImpl().magnHist[c*binSize + i];
			if (nSigs >= sigSize) {
				getImpl().histSigIndex[c] = i;
				getImpl().histSigValue[c] = (float)(i * binUnit);
				break;
			}
		}

		double sigValue = getImpl().histSigValue[c];
		double bgdValue = getImpl().histBgdValue[c];

		sigValue = (sigValue <= 0.0 ? binUnit : sigValue);
		bgdValue = (bgdValue <= 0.0 ? binUnit : bgdValue);
		snrRate = (float)(20.0f * log10(sigValue / bgdValue));
		getImpl().snrRatios[c] = snrRate;
	}
	return true;
}


int sig_proc::Magnitude::frameCols(void) const
{
	return getImpl().frameCols;
}


int sig_proc::Magnitude::frameRows(void) const
{
	return getImpl().frameRows;
}


int sig_proc::Magnitude::dataCols(void) const
{
	return getImpl().dataCols;
}


int sig_proc::Magnitude::dataRows(void) const
{
	return getImpl().dataRows;
}


int sig_proc::Magnitude::imageCols(void) const
{
	return getImpl().imageCols;
}


int sig_proc::Magnitude::imageRows(void) const
{
	return getImpl().imageRows;
}


int sig_proc::Magnitude::peakTopIndex(void) const
{
	return getImpl().peakTopIdx;
}


float sig_proc::Magnitude::peakTopValue(void) const
{
	return getImpl().peakTopVal;
}


int sig_proc::Magnitude::peakHalfIndex(int index) const
{
	return getImpl().peakHalfIdx[index];
}


int sig_proc::Magnitude::peakTermIndex(int index) const
{
	return getImpl().peakTermIdx[index];
}


float sig_proc::Magnitude::peakFWHM(void) const
{
	return getImpl().peakFwhm;
}


float sig_proc::Magnitude::peakSNR(void) const
{
	return getImpl().peakSnrRatio;
}


float sig_proc::Magnitude::maxPeakSNR(void) const
{
	return getImpl().maxPeakSnrRatio;
}

float sig_proc::Magnitude::meanPeakSNR(void) const
{
	return getImpl().avgPeakSnrRatio;
}


float sig_proc::Magnitude::meanPeakFWHM(void) const
{
	return getImpl().avgPeakFwhm;
}


float * sig_proc::Magnitude::getSnrRatios(void) const
{
	if (getImpl().snrRatios.empty()) {
		return nullptr;
	}
	return &getImpl().snrRatios[0];
}


float sig_proc::Magnitude::signalLevel(void) const
{
	return getImpl().signalVal;
}


float sig_proc::Magnitude::noiseLevel(void) const
{
	return getImpl().noiseVal;
}


Magnitude::MagnitudeImpl & sig_proc::Magnitude::getImpl(void) const
{
	return *d_ptr;
}


void sig_proc::Magnitude::setIntensityData(float * data, int width, int height, int wPadd, int hPadd, int multi)
{
	getImpl().frameData = data;
	getImpl().frameCols = width;
	getImpl().frameRows = height;
	getImpl().dataCols = width - wPadd;
	getImpl().dataRows = height - hPadd;
	getImpl().dataMulti = multi;
	getImpl().frameCount++;
	return;
}


void sig_proc::Magnitude::updateDataProfile(void)
{
	int index = getAxialDataIndex();

	setAxialData(index, getImpl().frameData);
	updateAxialDataPeak(index);
	updateAxialDataSignal(index);
	return;
}


void sig_proc::Magnitude::chooseAxialDataIndex(int index)
{
	getImpl().dataIndex = index;
	return;
}


int sig_proc::Magnitude::getAxialDataIndex(void)
{
	int index = getImpl().dataIndex;
	if (index >= 0 && index < getImpl().dataCols) {
	}
	else {
		// In case invalid axial index. 
		index = (getImpl().dataCols / 2);
	}
	return index;
}


void sig_proc::Magnitude::setAxialDataAverageSize(int size)
{
	size = min(max(size, 1), 100);

	getImpl().axialDataAvgSize = size;
	// getImpl().axialDataBuff = vector<array<float, 4096>>(size);
	getImpl().axialDataIndex = 0;
	getImpl().axialDataCount = 0;
	return;
}


void sig_proc::Magnitude::setAxialData(int index, float * data)
{
	if (index < 0 || index >= getImpl().dataCols) {
		return ;
	}

	int avgSize = getImpl().axialDataAvgSize;
	int dataIdx = getImpl().axialDataIndex;
	for (int j = 0; j < getImpl().dataRows; j++) {
		getImpl().axialDataBuff[dataIdx][j] = *(data + j*getImpl().frameCols + index);
	}
	getImpl().axialDataIndex = (dataIdx + 1) % avgSize;


	if (avgSize <= 1) {
		getImpl().axialData = getImpl().axialDataBuff[0];
	}
	else {
		if (++getImpl().axialDataCount > avgSize) {
			getImpl().axialData = array<float, 4096>{ 0 };
			for (int i = 0; i < getImpl().dataRows; i++) {
				for (int j = 0; j < avgSize; j++) {
					getImpl().axialData[i] += getImpl().axialDataBuff[j][i];
				}
				getImpl().axialData[i] /= avgSize;
			}
		}
		else {
			getImpl().axialData = array<float, 4096>{ 0 };
		}
	}

	return;
}


float * sig_proc::Magnitude::getAxialData(int index)
{
	if (index < 0 || index >= getImpl().dataCols) {
		return nullptr;
	}

	/*
	// Copy axial profile from frame data. 
	if (getImpl().axialData.size() != getImpl().dataRows) {
		getImpl().axialData = vector<float>(getImpl().dataRows);
	}

	for (int j = 0; j < getImpl().dataRows; j++) {
		getImpl().axialData[j] = *(getImpl().frameData + j*getImpl().frameCols + index);
	}
	*/
	return &getImpl().axialData[0];
}


void sig_proc::Magnitude::setFixedNoiseValue(float level)
{
	getImpl().noiseFixed = level;
	return;
}


void sig_proc::Magnitude::updateAxialDataPeak(int index)
{
	float* data = getAxialData(index);
	if (data == nullptr) {
		return;
	}

	float maxVal = 0.0f;
	float sumVal = 0.0f;
	int maxIdx = 0;
	int count = 0;

	int start = getImpl().peakRangeOffset[0];
	int close = getImpl().dataRows - getImpl().peakRangeOffset[1];
	for (int j = start; j < close; j++) {
		if (maxVal < data[j]) {
			maxVal = data[j];
			maxIdx = j;
		}
		sumVal += data[j];
		count++;
	}

	if (count == 0) {
		return;
	}

	float meanVal = sumVal / count;
	float halfMax = maxVal / 2.0f;
	float halfVal[2];
	int halfIdx[2] = { 0 };
	int termIdx[2] = { 0 };

	for (int j = maxIdx - 1; j >= start; j--) {
		if (data[j] <= halfMax) {
			halfIdx[0] = j;
			halfVal[0] = data[j];
			for (; j >= 0; j--) {
				termIdx[0] = j;
				if (data[j] <= meanVal) {
					break;
				}
			}
			break;
		}
	}

	for (int j = maxIdx + 1; j <= close; j++) {
		if (data[j] <= halfMax) {
			halfIdx[1] = j;
			halfVal[1] = data[j];
			for (; j < getImpl().dataRows; j++) {
				termIdx[1] = j;
				if (data[j] <= meanVal) {
					break;
				}
			}
			break;
		}
	}

	float rate, dist, fwhm = 0.0f;

	if (halfIdx[0] && halfIdx[1]) {
		rate = (halfMax - halfVal[0]) / (data[halfIdx[0] + 1] - halfVal[0]);
		dist = (maxIdx - halfIdx[0] - rate);
		fwhm = dist;

		rate = (halfMax - halfVal[1]) / (data[halfIdx[1] - 1] - halfVal[1]);
		dist = (halfIdx[1] - maxIdx - rate);
		fwhm += dist;
	}

	getImpl().peakTopIdx = maxIdx;
	getImpl().peakTopVal = maxVal;
	getImpl().peakHalfIdx[0] = halfIdx[0];
	getImpl().peakHalfIdx[1] = halfIdx[1];
	getImpl().peakTermIdx[0] = termIdx[0];
	getImpl().peakTermIdx[1] = termIdx[1];
	getImpl().peakFwhm = fwhm;

	/*
	float sumNoise = 0.0f;
	count = 1;
	for (int j = start; j < close; j++) {
		if (j <= termIdx[0] || j >= termIdx[1]) {
			sumNoise += data[j];
			count++;
		}
	}
	*/
	getImpl().peakFloor = sumVal / count;

	static float sumFwhm = 0.0f;
	sumFwhm += fwhm;
	if (getImpl().frameCount % 5 == 0) {
		getImpl().avgPeakFwhm = sumFwhm / 5.0f;
		sumFwhm = 0.0f;
	}

	if (maxVal > getImpl().maxPeakTopVal) {
		getImpl().maxPeakTopVal = maxVal;
	}
	return;
}


void sig_proc::Magnitude::updateAxialDataSignal(int index)
{
	if (getImpl().peakTopIdx <= 0) {
		return;
	}

	float* data = getAxialData(index);
	if (data == nullptr) {
		return;
	}

	float dsum = 0.0f;
	int dcnt = 0;

	// Noise level from average background.
	int start = getImpl().peakRangeOffset[0];
	int close = getImpl().dataRows - getImpl().peakRangeOffset[1];
	for (int j = start; j < close; j++) {
		if (data[j] <= getImpl().peakFloor) {
			dsum += data[j];
			dcnt++;
		}
	}

	if (dcnt <= 0) {
		return;
	}

	float noise = dsum / dcnt;
	float signal = getImpl().peakTopVal;
	float ratio = 0.0f;

	if (getImpl().noiseFixed > 0.01f) {
		noise = getImpl().noiseFixed;
	}

	if (noise > 0.0f) {
		ratio = (float)(20.0f * log10(signal / (noise)));
	}

	getImpl().noiseVal = noise;
	getImpl().signalVal = signal;
	getImpl().peakSnrRatio = ratio;

	if (ratio > getImpl().maxPeakSnrRatio) {
		getImpl().maxPeakSnrRatio = ratio;
	}

	static float sumRatio = 0.0f;
	sumRatio += ratio;
	if (getImpl().frameCount % 100 == 0) {
		getImpl().avgPeakSnrRatio = sumRatio / 100.0f;
		sumRatio = 0.0f;
	}
	return;
}


void sig_proc::Magnitude::clearAxialDataHistory(void)
{
	getImpl().maxPeakTopVal = 0.0f;
	getImpl().maxPeakSnrRatio = 0.0f;
	getImpl().avgPeakSnrRatio = 0.0f;
	getImpl().frameCount = 0;
	getImpl().avgPeakFwhm = 0.0f;
	return;
}


void sig_proc::Magnitude::setAxialPeakRangeOffset(int offset1, int offset2)
{
	getImpl().peakRangeOffset[0] = offset1;
	getImpl().peakRangeOffset[1] = offset2;
	return;
}


