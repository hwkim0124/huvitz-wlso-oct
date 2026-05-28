#include "pch.h"
#include "WsoLogger.h"
#include "CallbackRegistry.h"


using namespace wso_domain;

std::mutex WsoLogger::singleMutex_;


struct WsoLogger::WsoLoggerImpl
{
	WsoLogMsgLevel level;

	WsoLoggerImpl() {
		initializeWsoLoggerImpl();
	}

	void initializeWsoLoggerImpl(void) {
		level = LOG_MSG_INFO;
	}
};


wso_domain::WsoLogger::WsoLogger() :
	d_ptr(std::make_unique<WsoLoggerImpl>())
{
}


wso_domain::WsoLogger::~WsoLogger()
{
}


WsoLogger* wso_domain::WsoLogger::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static WsoLogger instance;
	return &instance;
}

void wso_domain::WsoLogger::setLogMsgCallback(WsoLogMsgCallback clb)
{
	if (auto* pinst = CallbackRegistry::getInstance(); pinst) {
		pinst->setWsoLogMsgCallback(clb);
	}
}

void wso_domain::WsoLogger::runLogMsgCallback(std::string msg, int level)
{
	if (auto* pinst = CallbackRegistry::getInstance(); pinst) {
		pinst->runWsoLogMsgCallback(msg, level);
	}
}

void wso_domain::WsoLogger::setLogMsgLevel(WsoLogMsgLevel level)
{
	impl().level = level;
}

void wso_domain::WsoLogger::writeMsg(std::string msg, WsoLogMsgLevel level)
{
	// Output log message. 
	switch (level) {
	case LOG_MSG_TRACE:
		LogTrace() << msg;
		break;
	case LOG_MSG_DEBUG:
		LogDebug() << msg;
		break;
	case LOG_MSG_WARNING:
		LogWarn() << msg;
		break;
	case LOG_MSG_ERROR:
		LogError() << msg;
		break;
	case LOG_MSG_FATAL:
		LogFatal() << msg;
		break;
	case LOG_MSG_INFO:
	default:
		LogInfo() << msg;
		break;
	}

	if (impl().level <= level) {
		// Run system logging callback. 
		getInstance()->runLogMsgCallback(msg, level);
	}
	return;
}

wso_domain::WsoLogger::WsoLoggerImpl& wso_domain::WsoLogger::impl(void) const
{
	return *d_ptr;
}

void WSODOMAIN_DLL_API wso_domain::WsoLogTrace(std::string text)
{
	WsoLogger::getInstance()->trace(text);
}

void WSODOMAIN_DLL_API wso_domain::WsoLogInfo(std::string text)
{
	WsoLogger::getInstance()->info(text);
}

void WSODOMAIN_DLL_API wso_domain::WsoLogDebug(std::string text)
{
	WsoLogger::getInstance()->debug(text);
}

void WSODOMAIN_DLL_API wso_domain::WsoLogError(std::string text)
{
	WsoLogger::getInstance()->error(text);
}

void WSODOMAIN_DLL_API wso_domain::WsoLogWarn(std::string text)
{
	WsoLogger::getInstance()->warn(text);
}

void WSODOMAIN_DLL_API wso_domain::WsoLogFatal(std::string text)
{
	WsoLogger::getInstance()->fatal(text);
}
