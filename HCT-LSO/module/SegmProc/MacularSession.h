#pragma once

#include "SegmProc.h"
#include "SegmSession.h"


namespace segm_proc
{
	class SEGMPROC_DLL_API MacularSession : public SegmSession
	{
	public:
		MacularSession();
		virtual ~MacularSession();

	public:
		virtual bool initialize(void);

	private:
		struct MacularSessionImpl;
		static std::unique_ptr<MacularSessionImpl> d_ptr;
		static MacularSessionImpl& getImpl(void);
	};
}

