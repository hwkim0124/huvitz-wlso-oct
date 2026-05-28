#pragma once

#include "WsoDomain2.h"

namespace wso_domain
{
	class WSODOMAIN_DLL_API SystemArchive
	{

	public:
		SystemArchive();
		virtual ~SystemArchive();

		SystemArchive(const SystemArchive& rhs) = delete;
		SystemArchive& operator=(const SystemArchive& rhs) = delete;

		static SystemArchive* getInstance(void);
		static std::mutex singleMutex_;

	public:
		void initializeSystemArchive(void);
		void clearSystemArchive(void);

		void writeSloPreviewLastResult(unsigned int seq_num, float quality, float intMean);
		void readSloPreviewLastResult(unsigned int& seq_num, float& quality, float& intMean);

		void clearSloScanResult(void);

	private:
		struct SystemArchiveImpl;
		std::unique_ptr<SystemArchiveImpl> d_ptr;
		SystemArchiveImpl& impl(void) const;
	};
}
