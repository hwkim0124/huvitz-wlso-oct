#pragma once

#include "CppUtil2.h"

#include <sstream>
#include <thread>
#include <memory>
#include <string>


namespace cpp_util
{
	class CPPUTIL_DLL_API Logger
	{
	public:
		Logger();
		virtual ~Logger();

		enum class LogLevel {
			LEVEL_TRACE = 0, LEVEL_DEBUG, LEVEL_INFO,
			LEVEL_WARNING, LEVEL_ERROR, LEVEL_FATAL
		};

	public:
		static bool initLogger(Logger::LogLevel level = LogLevel::LEVEL_INFO,
			bool srcLine = false, bool outFile = true,
			bool timeTag = true, bool fileKeep = false,
			std::string dirPath = ".//logs",
			std::string filePrefix = "system",
			std::string fileExt = ".log");
		static bool isInitiated(void);
		static void releaseLogger(void);

		static void setLogLevel(Logger::LogLevel level);
		static void setOutputDebug(bool flag);
		static void setOutputFile(bool flag);
		static bool isDebugMode(void);

		static void write(Logger::LogLevel logLevel, std::string cstr);

		static void trace(const char* format, ...);
		static void trace(std::string cstr);
		static void debug(const char* format, ...);
		static void debug(std::string cstr);
		static void info(const char* format, ...);
		static void info(std::string cstr);
		static void warn(const char* format, ...);
		static void warn(std::string cstr);
		static void error(const char* format, ...);
		static void error(std::string cstr);
		static void fatal(const char* format, ...);
		static void fatal(std::string cstr);

		static void writeLine(const std::wstring& text);

	protected:
		static bool createLogFileDirectory(std::string path);
		static bool makeLogFileName(std::string prefix, std::string fileExt, bool timeTag);
		static void removeOldLogFiles(std::string prefix, std::string fileExt, int daysToKeep);

		static void initBoostLog(void);
		static std::wstring getCurrentTimeTag(void);

	private:
		struct LoggerImpl;
		static std::unique_ptr<LoggerImpl> d_ptr;
		static LoggerImpl& getImpl(void);
	};


	struct CPPUTIL_DLL_API LoggerOut
	{
	public:
		LoggerOut() : level(Logger::LogLevel::LEVEL_INFO) {
		}

		LoggerOut(Logger::LogLevel logLevel) : level(logLevel) {
		}

		LoggerOut(Logger::LogLevel logLevel, const char* location, int line) : level(logLevel) {
			ss << location << "(" << line << "): " << std::this_thread::get_id() << " - ";
		}

		virtual ~LoggerOut() {
			Logger::write(this->level, ss.str());
		}

		std::ostringstream& operator()() { return ss; };
		void operator()(const char* msg) { ss << msg; };

	private:
		std::ostringstream ss;
		Logger::LogLevel level;
	};

	constexpr int LOG_LINE_BUFF_SIZE = 8192;
	constexpr int LOG_FILE_VALID_DAYS = 30;

#ifdef _DEBUG
#define LogTrace	cpp_util::LoggerOut(Logger::LogLevel::LEVEL_TRACE, __FILE__, __LINE__)
#define LogDebug	cpp_util::LoggerOut(Logger::LogLevel::LEVEL_DEBUG, __FILE__, __LINE__)
#define LogInfo		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_INFO, __FILE__, __LINE__)
#define LogWarn		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_WARNING, __FILE__, __LINE__)
#define LogError	cpp_util::LoggerOut(Logger::LogLevel::LEVEL_ERROR, __FILE__, __LINE__)
#define LogFatal	cpp_util::LoggerOut(Logger::LogLevel::LEVEL_FATAL, __FILE__, __LINE__)

#define LogT		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_TRACE, __FILE__, __LINE__)
#define LogD		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_DEBUG, __FILE__, __LINE__)
#define LogI		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_INFO, __FILE__, __LINE__)
#define LogW		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_WARNING, __FILE__, __LINE__)
#define LogE		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_ERROR, __FILE__, __LINE__)
#define LogF		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_FATAL, __FILE__, __LINE__)
#else 
#define LogTrace	cpp_util::LoggerOut(Logger::LogLevel::LEVEL_TRACE)
#define LogDebug	cpp_util::LoggerOut(Logger::LogLevel::LEVEL_DEBUG)
#define LogInfo		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_INFO)
#define LogWarn		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_WARNING)
#define LogError	cpp_util::LoggerOut(Logger::LogLevel::LEVEL_ERROR)
#define LogFatal	cpp_util::LoggerOut(Logger::LogLevel::LEVEL_FATAL)

#define LogT		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_TRACE)
#define LogD		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_DEBUG)
#define LogI		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_INFO)
#define LogW		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_WARNING)
#define LogE		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_ERROR)
#define LogF		cpp_util::LoggerOut(Logger::LogLevel::LEVEL_FATAL)
#endif

}

