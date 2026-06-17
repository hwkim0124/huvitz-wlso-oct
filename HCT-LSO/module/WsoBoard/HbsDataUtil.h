#pragma once

#include "WsoBoard.h"
#include "HbsDefines.h"

namespace wso_board
{
	class WSOBOARD_DLL_API HbsDataUtil
	{
	public:
		static unsigned int getCalibBlockDataSize(int blockIdx);
	};
}