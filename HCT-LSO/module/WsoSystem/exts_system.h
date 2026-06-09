#pragma once 

#include "WsoSystem2.h"



namespace wso_system
{
	extern "C"
	{
		bool WSOSYSTEM_DLL_API __stdcall initializeWsoSystem(wso_domain::WsoLogMsgCallback clb, bool trace_mode);
		void WSOSYSTEM_DLL_API __stdcall releaseWsoSystem(void);
		bool WSOSYSTEM_DLL_API __stdcall isWsoSystemInitialized(void);

		void WSOSYSTEM_DLL_API __stdcall connectWsoLogMsgCallback(wso_domain::WsoLogMsgCallback clb);
		void WSOSYSTEM_DLL_API __stdcall releaseWsoLogMsgCallback(void);
	}
}