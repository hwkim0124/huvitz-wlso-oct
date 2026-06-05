#pragma once

#include "OctPattern2.h"
#include "PatternScan.h"

#include <memory>


namespace oct_pattern
{
	class OCTPATTERN_DLL_API EnfaceScan : public PatternScan
	{
	public:
		EnfaceScan();
		virtual ~EnfaceScan();

		EnfaceScan(EnfaceScan&& rhs);
		EnfaceScan& operator=(EnfaceScan&& rhs);
		EnfaceScan(const EnfaceScan& rhs);
		EnfaceScan& operator=(const EnfaceScan& rhs);

	public:
		virtual bool buildPattern(void);

	protected:
		void buildEnfaceCube(bool isVert);

	private:
		struct EnfaceScanImpl;
		std::unique_ptr<EnfaceScanImpl> d_ptr;
		EnfaceScanImpl& getImpl(void) const;
	};
}
