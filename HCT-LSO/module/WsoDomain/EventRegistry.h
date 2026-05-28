#pragma once

#include "WsoDomain2.h"
#include "EventFuncs.h"


namespace wso_domain
{
	class WSODOMAIN_DLL_API EventRegistry
	{
	public:
		EventRegistry();
		virtual ~EventRegistry();

		EventRegistry(const EventRegistry& rhs) = delete;
		EventRegistry& operator=(const EventRegistry& rhs) = delete;

		static EventRegistry* getInstance(void);
		static std::mutex singleMutex_;

	public:
		void setOctPreviewBufferGrabbed(OctPreviewBufferGrabbedEvent clb = nullptr);
		void setOctMeasureBufferGrabbed(OctMeasureBufferGrabbedEvent clb = nullptr);
		void setOctEnfaceBufferGrabbed(OctEnfaceBufferGrabbedEvent clb = nullptr);

		void setOctEnfaceImageProcessed(OctEnfaceImageProcessedEvent clb = nullptr);
		void setOctPreviewImageProcessed(OctPreviewImageProcessedEvent clb = nullptr);
		void setOctMeasureImageProcessed(OctMeasureImageProcessedEvent clb = nullptr);
		void setOctProtocolPatternAcquired(OctProtocolPatternAcquiredEvent clb = nullptr);
		void setOctProtocolMeasureCompleted(OctProtocolMeasureCompletedEvent clb = nullptr);

		void runOctPreviewBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index);
		void runOctMeasureBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index);
		void runOctEnfaceBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index);

		void runOctEnfaceImageProcessed(unsigned char* bits, int width, int height);
		void runOctPreviewImageProcessed(unsigned char* bits, int width, int height, float qindex, float sn_ratio, int ref_point, int image_index);
		void runOctMeasureImageProcessed(unsigned char* bits, int width, int height, float qindex, float sn_ratio, int ref_point, int image_index, unsigned char* lateral);
		void runOctProtocolPatternAcquired(bool result);
		void runOctProtocolMeasureCompleted(bool result);


	private:
		struct EventRegistryImpl;
		std::unique_ptr<EventRegistryImpl> d_ptr;
		EventRegistryImpl& impl(void) const;
	};
}

