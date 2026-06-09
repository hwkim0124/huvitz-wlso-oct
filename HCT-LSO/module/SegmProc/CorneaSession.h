#pragma once

#include "SegmProc.h"
#include "SegmSession.h"


namespace segm_proc
{
	class SEGMPROC_DLL_API CorneaSession : public SegmSession
	{
	public:
		CorneaSession();
		virtual ~CorneaSession();

	public:
		bool processDewarping(void);

	protected:
		virtual bool initialize(void) override;

		bool executeDewarping(void);
		bool employDewarping(void);

	private:
		struct CorneaSessionImpl;
		static std::unique_ptr<CorneaSessionImpl> d_ptr;
		static CorneaSessionImpl& getImpl(void);
	};
}
