#pragma once
#include "LayeredBscan.h"

namespace semt_segm
{
	class SEMTSEGM_DLL_API MacularBscan :
		public LayeredBscan
	{
	public:
		MacularBscan();
		virtual ~MacularBscan();

		MacularBscan(MacularBscan&& rhs);
		MacularBscan& operator=(MacularBscan&& rhs);
		MacularBscan(const MacularBscan& rhs) = delete;
		MacularBscan& operator=(const MacularBscan& rhs) = delete;

	private:
		struct MacularBscanImpl;
		std::unique_ptr<MacularBscanImpl> d_ptr;
		MacularBscanImpl& getImpl(void) const;
	};
}

