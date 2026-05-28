#include "pch.h"
#include "SystemArchive.h"


using namespace wso_domain;

std::mutex SystemArchive::singleMutex_;

struct SystemArchive::SystemArchiveImpl
{
	unsigned int sloPreviewSeqNum;
	float sloPreviewQuality;
	float sloPreviewIntMean;


	SystemArchiveImpl() {
		initializeSystemArchiveImpl();
	}

	void initializeSystemArchiveImpl(void) {
		sloPreviewSeqNum = 0;
		sloPreviewQuality = 0.0f;
		sloPreviewIntMean = 0.0f;
	}
};



wso_domain::SystemArchive::SystemArchive() :
	d_ptr(std::make_unique<SystemArchiveImpl>())
{
}


wso_domain::SystemArchive::~SystemArchive()
{
}


SystemArchive* wso_domain::SystemArchive::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static SystemArchive instance;
	return &instance;
}

void wso_domain::SystemArchive::initializeSystemArchive(void)
{
}

void wso_domain::SystemArchive::clearSystemArchive(void)
{
	impl().initializeSystemArchiveImpl();
}

void wso_domain::SystemArchive::writeSloPreviewLastResult(unsigned int seq_num, float quality, float intMean)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().sloPreviewSeqNum = seq_num;
	impl().sloPreviewQuality = quality;
	impl().sloPreviewIntMean = intMean;
	return;
}

void wso_domain::SystemArchive::readSloPreviewLastResult(unsigned int& seq_num, float& quality, float& intMean)
{
	lock_guard<mutex> lock(singleMutex_);
	seq_num = impl().sloPreviewSeqNum;
	quality = impl().sloPreviewQuality;
	intMean = impl().sloPreviewIntMean;
	return;
}

void wso_domain::SystemArchive::clearSloScanResult(void)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().sloPreviewSeqNum = 0;
	impl().sloPreviewQuality = 0.0f;
	impl().sloPreviewIntMean = 0.0f;
	return;
}

SystemArchive::SystemArchiveImpl& wso_domain::SystemArchive::impl(void) const
{
	return *d_ptr;
}
