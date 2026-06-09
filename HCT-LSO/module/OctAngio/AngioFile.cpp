#include "pch.h"
#include "AngioFile.h"
#include "AngioDecorr.h"
#include "AngioLayout.h"

#include "OctAngio2.h"

using namespace oct_angio;



struct AngioFile::AngioFileImpl
{
	AngioFileImpl() {

	};
};



AngioFile::AngioFile()
	: d_ptr(make_unique<AngioFileImpl>())
{
}


AngioFile::~AngioFile()
{
}


oct_angio::AngioFile::AngioFile(AngioFile && rhs) = default;
AngioFile & oct_angio::AngioFile::operator=(AngioFile && rhs) = default;


bool oct_angio::AngioFile::saveAngioDataFile(const std::string dirPath, const std::string fileName,
										oct_angio::AngioLayout & layout, oct_angio::AngioDecorr & decorr)
{
	auto header = makeDataHeader(layout, decorr);
	auto dsizes = makeDecorrSizes(layout, decorr);

	std::string path;
	path = (boost::format("%s//%s") % dirPath % fileName).str();

	try {
		std::ofstream file(path, std::ios::out | std::ofstream::binary);
		file.write((char*)(&header[0]), header.size());
		file.write((char*)(&dsizes[0]), dsizes.size() * sizeof(int));

		int lines = layout.numberOfLines();
		auto& differs = decorr.differentials();
		auto& decorrs = decorr.decorrelations();

		for (int i = 0; i < lines; i++) {
			auto table = makeDecorrTable(i, dsizes[i], layout, decorr);
			file.write((char*)(&table.indice[0]), table.indice.size() * sizeof(int));
			file.write((char*)(&table.dc_vals[0]), table.dc_vals.size() * sizeof(unsigned short));
			file.write((char*)(&table.df_vals[0]), table.df_vals.size() * sizeof(unsigned short));
			// int bytes = table.dc_vals.size() * sizeof(unsigned short);
			// LogD() << i << "th data written, size: " << dsizes[i] << ", bytes: " << bytes;
		}

		file.close();
	}
	catch (...) {
		LogD() << "Saving angio data file failed, path: " << path;
		return false;
	}

	LogD() << "Angio data file saved: " << path;
	return true;
}


bool oct_angio::AngioFile::loadAngioDataFile(const std::string dirPath, const std::string fileName, oct_angio::AngioLayout & layout, oct_angio::AngioDecorr & decorr)
{
	std::string path;
	path = (boost::format("%s//%s") % dirPath % fileName).str();

	fstream fs;
	fs.open(path);
	if (fs.fail()) {
		LogD() << "Angio data file2 not found, path: " << path;
		return false;
	}

	try {
		std::ifstream file(path, std::ios::in | std::ifstream::binary);
		if (!file.good()) {
			LogD() << "Openning angio data file2 failed, path: " << path;
			return false;
		}

		std::vector<int> dsizes;

		if (!readDataHeader(file, layout, decorr)) {
			LogD() << "Loading angio data2 failed, invalid header";
			file.close();
			return false;
		}
		if (!readDecorrSizes(file, dsizes, layout, decorr)) {
			LogD() << "Loading angio data2 failed, invalid sizes";
			file.close();
			return false;
		}
		if (!readDecorrTables(file, dsizes, layout, decorr)) {
			LogD() << "Loading angio data2 failed, invalid data";
			file.close();
			return false;
		}
		file.close();
	}
	catch (...) {
		LogD() << "Loading angio data file2 failed, path: " << path;
		return false;
	}

	LogD() << "Angio data file2 loaded: " << path;
	return true;
}


bool oct_angio::AngioFile::readDataHeader(std::ifstream & file, oct_angio::AngioLayout & layout, oct_angio::AngioDecorr & decorr)
{
	int headSize = FILE_HEADER_SIZE;
	auto header = vector<char>(headSize);

	file.read((char*)&header[0], sizeof(char)*headSize);

	int lines = layout.numberOfLines();
	int points = layout.numberOfPoints();
	int repeats = layout.numberOfRepeats();
	bool vertical = layout.isVerticalScan();
	float rangeX = layout.scanRangeX();
	float rangeY = layout.scanRangeY();
	float centerX = layout.foveaCenterX();
	float centerY = layout.foveaCenterY();
	bool isDisc = layout.isDiscScan();

	int idx = 0;
	memcpy(&lines, &header[idx], sizeof(int)); idx += sizeof(int);
	memcpy(&points, &header[idx], sizeof(int)); idx += sizeof(int);
	memcpy(&repeats, &header[idx], sizeof(int)); idx += sizeof(int);
	memcpy(&vertical, &header[idx], sizeof(bool)); idx += sizeof(bool);

	memcpy(&rangeX, &header[idx], sizeof(float)); idx += sizeof(float);
	memcpy(&rangeY, &header[idx], sizeof(float)); idx += sizeof(float);
	memcpy(&centerX, &header[idx], sizeof(float)); idx += sizeof(float);
	memcpy(&centerY, &header[idx], sizeof(float)); idx += sizeof(float);
	memcpy(&isDisc, &header[idx], sizeof(bool)); idx += sizeof(bool);

	if (lines <= 0 || points <= 0 || repeats <= 0 || lines > 2048 || points > 2048 || repeats > 20) {
		return false;
	}
	if (fabs(rangeX) >= 99.0f || fabs(rangeY) >= 99.0f || fabs(centerX) >= 99.0f || fabs(centerY) >= 99.0f) {
		return false;
	}

	layout.setupLayout(lines, points, repeats, vertical);
	layout.setupRange(rangeX, rangeY, centerX, centerY, isDisc);
	return true;
}

bool oct_angio::AngioFile::readDecorrSizes(std::ifstream & file, std::vector<int>& dsizes, oct_angio::AngioLayout & layout, oct_angio::AngioDecorr & decorr)
{
	int bodySize = FILE_DSIZES_SIZE;
	auto sizes = vector<int>(bodySize, 0);

	file.read((char*)&sizes[0], sizeof(int)*bodySize);

	int count = (int)count_if(sizes.begin(), sizes.end(), [](int e) { return e > 0; });
	LogD() << "Decorrelation sizes read, valid files: " << count;
	dsizes = sizes;

	int invalids = (int)count_if(sizes.begin(), sizes.end(), [](int e) { return e < 0 || e > (2048*2048); });
	if (invalids > 0) {
		return false;
	}
	return true;
}

bool oct_angio::AngioFile::readDecorrTables(std::ifstream & file, std::vector<int> dsizes, oct_angio::AngioLayout & layout, oct_angio::AngioDecorr & decorr)
{
	int lines = layout.numberOfLines();
	int points = layout.numberOfPoints();

	auto& differs = decorr.differentials();
	auto& decorrs = decorr.decorrelations();

	decorrs.resize(lines);
	differs.resize(lines);

	vector<int> indice;
	vector<unsigned short> dc_vals;
	vector<unsigned short> df_vals;
	vector<float> dc_data;
	vector<float> df_data;

	for (int i = 0; i < lines; i++) {
		dc_data = vector<float>(FILE_DATA_HEIGHT*points, 0.0f);
		df_data = vector<float>(FILE_DATA_HEIGHT*points, 0.0f);

		int dsize = dsizes[i];
		if (dsize > 0) {
			indice.resize(dsize, 0);
			dc_vals.resize(dsize, 0);
			df_vals.resize(dsize, 0);

			file.read((char*)&indice[0], sizeof(int)*dsize);
			file.read((char*)&dc_vals[0], sizeof(unsigned short)*dsize);
			file.read((char*)&df_vals[0], sizeof(unsigned short)*dsize);

			for (int j = 0; j < dsize; j++) {
				int k = indice[j];
				// LogD() << "Index: " << k << ", " << dc_vals[j] << ", " << df_vals[j];
				dc_data[k] = dc_vals[j] * DECORR_VALUE_MIN;
				df_data[k] = df_vals[j];
			}
		}

		decorrs[i].fromFloat32((const unsigned char*)&dc_data[0], points, FILE_DATA_HEIGHT);
		differs[i].fromFloat32((const unsigned char*)&df_data[0], points, FILE_DATA_HEIGHT);
	}
	return true;
}

std::vector<char> oct_angio::AngioFile::makeDataHeader(oct_angio::AngioLayout& layout, oct_angio::AngioDecorr& decorr)
{
	int headSize = FILE_HEADER_SIZE;
	auto header = vector<char>(headSize);

	int lines = layout.numberOfLines();
	int points = layout.numberOfPoints();
	int repeats = layout.numberOfRepeats();
	bool vertical = layout.isVerticalScan();
	float rangeX = layout.scanRangeX();
	float rangeY = layout.scanRangeY();
	float centerX = layout.foveaCenterX();
	float centerY = layout.foveaCenterY();
	bool isDisc = layout.isDiscScan();

	int idx = 0;
	memcpy(&header[idx], &lines, sizeof(int)); idx += sizeof(int);
	memcpy(&header[idx], &points, sizeof(int)); idx += sizeof(int);
	memcpy(&header[idx], &repeats, sizeof(int)); idx += sizeof(int);
	memcpy(&header[idx], &vertical, sizeof(bool)); idx += sizeof(bool);

	memcpy(&header[idx], &rangeX, sizeof(float)); idx += sizeof(float);
	memcpy(&header[idx], &rangeY, sizeof(float)); idx += sizeof(float);
	memcpy(&header[idx], &centerX, sizeof(float)); idx += sizeof(float);
	memcpy(&header[idx], &centerY, sizeof(float)); idx += sizeof(float);
	memcpy(&header[idx], &isDisc, sizeof(bool)); idx += sizeof(bool);

	return header;
}


std::vector<int> oct_angio::AngioFile::makeDecorrSizes(oct_angio::AngioLayout& layout, oct_angio::AngioDecorr & decorr)
{
	int bodySize = FILE_DSIZES_SIZE;
	auto sizes = vector<int>(bodySize, 0);

	auto& differs = decorr.differentials();
	auto& decorrs = decorr.decorrelations();
	int lines = layout.numberOfLines();

	for (int i = 0; i < lines; i++) {
		auto data = decorrs[i].copyDataInFloats();
		int count = (int) count_if(data.begin(), data.end(), [](float e) { return e >= DECORR_VALUE_MIN; });
		sizes[i] = count;
	}

	return sizes;
}


oct_angio::AngioFile::DecorrTable oct_angio::AngioFile::makeDecorrTable(int index, int dsize, oct_angio::AngioLayout& layout, oct_angio::AngioDecorr& decorr)
{
	auto table = DecorrTable(dsize);
	auto p_decorr = decorr.getDecorrelationData(index);
	auto p_differ = decorr.getDifferentialsData(index);

	int width = layout.numberOfPoints();
	int height = FILE_DATA_HEIGHT;

	int k = 0;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = y * width + x;
			if (p_decorr[idx] >= DECORR_VALUE_MIN) {
				table.indice[k] = idx;
				table.dc_vals[k] = (unsigned short)(p_decorr[idx] * (1.0f / DECORR_VALUE_MIN));
				table.df_vals[k] = (unsigned short)(p_differ[idx]);
				k++;
			}
		}
	}
	return table;
}


AngioFile::AngioFileImpl & oct_angio::AngioFile::getImpl(void) const
{
	return *d_ptr;
}
