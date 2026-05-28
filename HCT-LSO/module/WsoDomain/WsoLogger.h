#pragma once

#include "WsoDomain2.h"
#include "CallbackFuncs.h"



namespace wso_domain
{
	enum WsoLogMsgLevel : unsigned short {
		LOG_MSG_TRACE = 0,
		LOG_MSG_DEBUG = 1,
		LOG_MSG_INFO = 2,
		LOG_MSG_WARNING = 3,
		LOG_MSG_ERROR = 4,
		LOG_MSG_FATAL = 5
	};

	class WSODOMAIN_DLL_API WsoLogger
	{
	public:
		WsoLogger();
		virtual ~WsoLogger();

		WsoLogger(const WsoLogger& rhs) = delete;
		WsoLogger& operator=(const WsoLogger& rhs) = delete;

		static WsoLogger* getInstance(void);
		static std::mutex singleMutex_;

	public:
		void setLogMsgCallback(WsoLogMsgCallback clb);
		void runLogMsgCallback(std::string msg, int level);

		void setLogMsgLevel(WsoLogMsgLevel level);
		void writeMsg(std::string msg, WsoLogMsgLevel level);
		void info(std::string msg) { writeMsg(msg, LOG_MSG_INFO); }
		void trace(std::string msg) { writeMsg(msg, LOG_MSG_TRACE); }
		void debug(std::string msg) { writeMsg(msg, LOG_MSG_DEBUG); }
		void error(std::string msg) { writeMsg(msg, LOG_MSG_ERROR); }
		void warn(std::string msg) { writeMsg(msg, LOG_MSG_WARNING); }
		void fatal(std::string msg) { writeMsg(msg, LOG_MSG_FATAL); }

	private:
		struct WsoLoggerImpl;
		std::unique_ptr<WsoLoggerImpl> d_ptr;
		WsoLoggerImpl& impl(void) const;
	};

	void WSODOMAIN_DLL_API WsoLogTrace(std::string text);
	void WSODOMAIN_DLL_API WsoLogInfo(std::string text);
	void WSODOMAIN_DLL_API WsoLogDebug(std::string text);
	void WSODOMAIN_DLL_API WsoLogError(std::string text);
	void WSODOMAIN_DLL_API WsoLogWarn(std::string text);
	void WSODOMAIN_DLL_API WsoLogFatal(std::string text);
}

