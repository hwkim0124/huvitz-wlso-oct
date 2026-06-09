#pragma once

#include "WsoSystem2.h"

namespace oct_result {
	class ProtocolResult;
}

namespace wso_system
{
	class WSOSYSTEM_DLL_API OctArchive
	{
	public:
		OctArchive();
		virtual ~OctArchive();

		OctArchive(const OctArchive& rhs) = delete;
		OctArchive& operator=(const OctArchive& rhs) = delete;

		static OctArchive* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool initializeOctArchive(void);
		void releaseOctArchive(void);
		bool isInitialized(void) const;

		bool initiateProtocolResult(OctScanProtocol* protocol, PatternPlan* plan);
		void clearProtocolResult(void);

		bool assignPreviewImage(const OctScanSection& section, const OctScanImageDescript& image);
		bool assignMeasureImage(const OctScanSection& section, const OctScanImageDescript& image, int overlap_index);
		bool assignEnfaceImage(const OctEnfaceImageDescript& image);
		bool assignCorneaImage(const OctCorneaImageDescript& image);
		bool assignRetinaImage(const OctRetinaImageDescript& image);

		int getMeasureSectionCount(void) const;
		int getPreviewSectionCount(void) const;
		bool getMeasureSectionImage(int sect_index, int image_index, OctScanImageDescript& image) const;
		bool getPreviewSectionImage(int sect_index, int image_index, OctScanImageDescript& image) const;
		bool getMeasureEnfaceImage(OctEnfaceImageDescript& image) const;
		bool getCorneaCameraImage(OctCorneaImageDescript& image) const;

		bool isProtocolResultValid(void) const;

		oct_result::ProtocolResult* getProtocolResult(void) const;
		unique_ptr<oct_result::ProtocolResult> fetchProtocolResultLast(void);

	protected:
		void onOctEnfaceImageProcessed(unsigned char* bits, int width, int height);
		void onOctPreviewImageProcessed(unsigned char* bits, int width, int height, float qindex, float sn_ratio, int ref_point, int image_index);
		void onOctMeasureImageProcessed(unsigned char* bits, int width, int height, float qindex, float sn_ratio, int ref_point, int image_index, unsigned char* lateral);

		void onOctProtocolPatternAcquired(bool result);
		void onOctProtocolMeasureCompleted(bool result);

	private:
		struct OctArchiveImpl;
		std::unique_ptr<OctArchiveImpl> d_ptr;
		OctArchiveImpl& impl(void) const;

	};
}