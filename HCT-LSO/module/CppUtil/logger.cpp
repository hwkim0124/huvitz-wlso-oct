#include "pch.h"
#include "Logger.h"
#include "StrFormat.h"

#include <fstream>
#include <sstream>
#include <ostream>
#include <thread>
#include <Windows.h>
#include <ctime>
#include <chrono>


#include <boost/log/trivial.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

using namespace cpp_util;
using namespace std;
using namespace boost;


// Register a global logger
BOOST_LOG_GLOBAL_LOGGER(logger, boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>)


// Just a helper macro used by the macros below.
#define LOG(severity) BOOST_LOG_SEV(logger::get(), boost::log::trivial::severity)


#include <boost/log/core/core.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>


namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;


BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", logging::trivial::severity_level)


BOOST_LOG_GLOBAL_LOGGER_INIT(logger, src::severity_logger_mt) {
	src::severity_logger_mt<boost::log::trivial::severity_level> logger;

	// Add attributes
	logger.add_attribute("LineID", attrs::counter<unsigned int>(1));     // lines are sequentially numbered
	logger.add_attribute("TimeStamp", attrs::local_clock());             // each log line gets a timestamp
	return logger;
}

// Log macros
#define LOG_TRACE   LOG(trace)
#define LOG_DEBUG   LOG(debug)
#define LOG_INFO    LOG(info)
#define LOG_WARNING LOG(warning)
#define LOG_ERROR   LOG(error)
#define LOG_FATAL   LOG(fatal)


struct Logger::LoggerImpl
{
	bool outputDebug;
	bool outputFile;
	bool initiated;

	string logDirPath;
	string logFileName;
	string logFilePath;
	string logFileTag;

	char cbuff[LOG_LINE_BUFF_SIZE];

	log::trivial::severity_level level;
	bool logSrcLine;
	bool logTimeTag;
	bool logFileKeep;

	LoggerImpl() : cbuff{ 0 }, level(log::trivial::info),
		logSrcLine(true), logTimeTag(false), logFileKeep(false), outputDebug(true), outputFile(false), initiated(false),
		logDirPath(".//logs"), logFileName("system.log"), logFilePath(".//system.log")
	{
	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<Logger::LoggerImpl> Logger::d_ptr(new LoggerImpl());


Logger::Logger()
{

}


Logger::~Logger()
{
}


bool cpp_util::Logger::initLogger(Logger::LogLevel level, bool srcLine, bool outFile, bool timeTag, bool fileKeep,
	std::string dirPath,
	std::string filePrefix, std::string fileExt)
{
	setLogLevel(level);
	setOutputFile(outFile);
	d_ptr->logSrcLine = srcLine;
	d_ptr->logTimeTag = timeTag;
	d_ptr->logFileKeep = fileKeep;

#ifdef _DEBUG
	setOutputDebug(true);
	// setOutputFile(false);
	d_ptr->logSrcLine = true;
#endif

	if (outFile) {
		if (!createLogFileDirectory(dirPath)) {
			return false;
		}
		if (!makeLogFileName(filePrefix, fileExt, timeTag)) {
			return false;
		}

		removeOldLogFiles(filePrefix, fileExt, LOG_FILE_VALID_DAYS);
	}

	initBoostLog();

	LogD() << "Logger initialized, log level: " << static_cast<int>(level) << ", output file: " << outFile << ", dir path: " << dirPath;
	d_ptr->initiated = true;
	return isInitiated();
}


bool cpp_util::Logger::isInitiated(void)
{
	return getImpl().initiated;
}


void cpp_util::Logger::releaseLogger(void)
{
	logging::core::get()->flush();
	return;
}


void cpp_util::Logger::setLogLevel(Logger::LogLevel level)
{
	if (level == LogLevel::LEVEL_TRACE) {
		getImpl().level = log::trivial::trace;
	}
	else if (level == LogLevel::LEVEL_DEBUG) {
		getImpl().level = log::trivial::debug;
	}
	else if (level == LogLevel::LEVEL_INFO) {
		getImpl().level = log::trivial::info;
	}
	else if (level == LogLevel::LEVEL_WARNING) {
		getImpl().level = log::trivial::warning;
	}
	else if (level == LogLevel::LEVEL_ERROR) {
		getImpl().level = log::trivial::error;
	}
	else if (level == LogLevel::LEVEL_FATAL) {
		getImpl().level = log::trivial::fatal;
	}
	else {
		getImpl().level = log::trivial::info;
	}
	return;
}


void cpp_util::Logger::setOutputDebug(bool flag)
{
	getImpl().outputDebug = flag;
	return;
}


void cpp_util::Logger::setOutputFile(bool flag)
{
	getImpl().outputFile = flag;
	return;
}


bool cpp_util::Logger::isDebugMode(void)
{
	return (d_ptr->level < log::trivial::info);
}


void cpp_util::Logger::write(Logger::LogLevel logLevel, std::string cstr)
{
	switch (logLevel) {
	case Logger::LogLevel::LEVEL_DEBUG:
		LOG_DEBUG << cstr;
		break;
	case Logger::LogLevel::LEVEL_INFO:
		LOG_INFO << cstr;
		break;
	case Logger::LogLevel::LEVEL_WARNING:
		LOG_WARNING << cstr;
		break;
	case Logger::LogLevel::LEVEL_ERROR:
		LOG_ERROR << cstr;
		break;
	case Logger::LogLevel::LEVEL_FATAL:
		LOG_FATAL << cstr;
		break;
	case Logger::LogLevel::LEVEL_TRACE:
	default:
		LOG_TRACE << cstr;
		break;
	}

	if (static_cast<int>(logLevel) >= d_ptr->level) {
		logging::core::get()->flush();
	}

	// If _DEBUG defined, print log record into Visual stduio's output window. 
	if (d_ptr->outputDebug) {
		cstr += "\n";
		OutputDebugStringA(cstr.c_str());
	}
	return;
}


void cpp_util::Logger::trace(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	::vsnprintf_s(d_ptr->cbuff, LOG_LINE_BUFF_SIZE, _TRUNCATE, format, ap);
	va_end(ap);
	write(LogLevel::LEVEL_TRACE, d_ptr->cbuff);
	return;
}


void cpp_util::Logger::trace(std::string cstr)
{
	write(LogLevel::LEVEL_TRACE, cstr);
	return;
}


void cpp_util::Logger::debug(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	::vsnprintf_s(d_ptr->cbuff, LOG_LINE_BUFF_SIZE, _TRUNCATE, format, ap);
	va_end(ap);
	write(LogLevel::LEVEL_DEBUG, d_ptr->cbuff);
	return;
}


void cpp_util::Logger::debug(std::string cstr)
{
	write(LogLevel::LEVEL_DEBUG, cstr);
	return;
}


void cpp_util::Logger::info(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	::vsnprintf_s(d_ptr->cbuff, LOG_LINE_BUFF_SIZE, _TRUNCATE, format, ap);
	va_end(ap);
	write(LogLevel::LEVEL_INFO, d_ptr->cbuff);
	return;
}


void cpp_util::Logger::info(std::string cstr)
{
	write(LogLevel::LEVEL_INFO, cstr);
	return;
}


void cpp_util::Logger::warn(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	::vsnprintf_s(d_ptr->cbuff, LOG_LINE_BUFF_SIZE, _TRUNCATE, format, ap);
	va_end(ap);
	write(LogLevel::LEVEL_WARNING, d_ptr->cbuff);
	return;
}


void cpp_util::Logger::warn(std::string cstr)
{
	write(LogLevel::LEVEL_WARNING, cstr);
	return;
}


void cpp_util::Logger::error(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	::vsnprintf_s(d_ptr->cbuff, LOG_LINE_BUFF_SIZE, _TRUNCATE, format, ap);
	va_end(ap);
	write(LogLevel::LEVEL_ERROR, d_ptr->cbuff);
	return;
}


void cpp_util::Logger::error(std::string cstr)
{
	write(LogLevel::LEVEL_ERROR, cstr);
	return;
}


void cpp_util::Logger::fatal(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	::vsnprintf_s(d_ptr->cbuff, LOG_LINE_BUFF_SIZE, _TRUNCATE, format, ap);
	va_end(ap);
	write(LogLevel::LEVEL_FATAL, d_ptr->cbuff);
	return;
}


void cpp_util::Logger::fatal(std::string cstr)
{
	write(LogLevel::LEVEL_FATAL, cstr);
	return;
}


void cpp_util::Logger::writeLine(const std::wstring& text)
{
	wstring line;
	line = getCurrentTimeTag();
	line += L" ";
	line += text;
	line += L"\r\n";
	return ;
}


bool cpp_util::Logger::createLogFileDirectory(std::string path)
{
	if (CreateDirectory(atow(path).c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {
		getImpl().logDirPath = path;
		return true;
	}
	return false;
}


bool cpp_util::Logger::makeLogFileName(std::string prefix, std::string fileExt, bool timeTag)
{
	if (!timeTag) {
		auto filename = format_string("%s%s", prefix, fileExt);
		getImpl().logFileName = filename;
	}
	else {
		auto datetime = datetime_string();
		auto filename = format_string("%s-%s%s", prefix, datetime, fileExt);

		getImpl().logFileTag = datetime;
		getImpl().logFileName = filename;
	}

	auto filepath = format_string("%s//%s", d_ptr->logDirPath, d_ptr->logFileName);
	return true;
}


void cpp_util::Logger::removeOldLogFiles(std::string prefix, std::string fileExt, int daysToKeep)
{
	auto log_path = d_ptr->logDirPath;
	auto log_name = format_string("%s-*%s", prefix, fileExt);

	string pattern(log_path);
	pattern.append("//").append(log_name);

	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_t = std::chrono::system_clock::to_time_t(now);
	std::time_t keep_t = now_t - (daysToKeep * 24 * 60 * 60);
	auto ttag = date_string(keep_t, "");

	WIN32_FIND_DATAA data;
	HANDLE hFind;

	if ((hFind = FindFirstFileA(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
		do {
			string fileName = data.cFileName;
			string filePath(d_ptr->logDirPath);
			filePath.append("//").append(fileName);

			if (fileName.size() > 13) {
				// Compare 6 digits date.
				auto ftag = fileName.substr(7, 6);
				if (atoi(ftag.c_str()) >= atoi(ttag.c_str())) {
					// LogDebug() << filePath;
				}
				else {
					DeleteFileA(filePath.c_str());
					LogD() << "Log file deleted: " << filePath;
				}
			}

		} while (FindNextFileA(hFind, &data) != 0);
		FindClose(hFind);
	}
	return;
}


void cpp_util::Logger::initBoostLog(void)
{
	typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
	boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

	if (getImpl().outputFile) {
		// add a logfile stream to our sink
		sink->locked_backend()->add_stream(boost::make_shared<std::ofstream>(getImpl().logFilePath));
	}

	// add "console" output stream to our sink
	// sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));

	// specify the format of the log message
	logging::formatter formatter = expr::stream
		<< std::setw(7) << std::setfill('0') << line_id << std::setfill(' ') << " | "
		// << expr::format_date_time(timestamp, "%Y-%m-%d, %H:%M:%S.%f") << " "
		<< expr::format_date_time(timestamp, "%Y-%m-%d, %H:%M:%S") << " "
		<< "[" << logging::trivial::severity << "]"
		<< " - " << expr::smessage;
	sink->set_formatter(formatter);

	// only messages with severity >= SEVERITY_THRESHOLD are written
	sink->set_filter(severity >= getImpl().level);

	// "register" our sink
	logging::core::get()->add_sink(sink);
	return;
}


std::wstring cpp_util::Logger::getCurrentTimeTag(void)
{
	auto datetime = datetime_string();
	auto timetag = format_string("[%s]", datetime);
	return std::wstring(atow(timetag));
}


Logger::LoggerImpl& cpp_util::Logger::getImpl(void)
{
	return *d_ptr;
}

