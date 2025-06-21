#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <sstream>
#include <iomanip>
#include <type_traits>

enum class LogLevel
{
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    LOG_ERROR = 3,
    FATAL = 4
};

class Logger
{
public:
    static Logger &Instance();

    void SetLogLevel(LogLevel level);
    void SetLogFile(const std::string &filename);
    void EnableConsole(bool enable);

    void Debug(const std::string &message);
    void Info(const std::string &message);
    void Warning(const std::string &message);
    void Error(const std::string &message);
    void Fatal(const std::string &message);

    void Log(LogLevel level, const std::string &message);

    // Format helpers
    template <typename... Args>
    void DebugFmt(const std::string &format, Args... args)
    {
        Log(LogLevel::DEBUG, FormatString(format, args...));
    }

    template <typename... Args>
    void InfoFmt(const std::string &format, Args... args)
    {
        Log(LogLevel::INFO, FormatString(format, args...));
    }

    template <typename... Args>
    void WarningFmt(const std::string &format, Args... args)
    {
        Log(LogLevel::WARNING, FormatString(format, args...));
    }

    template <typename... Args>
    void ErrorFmt(const std::string &format, Args... args)
    {
        Log(LogLevel::LOG_ERROR, FormatString(format, args...));
    }

private:
    Logger();
    ~Logger();

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    // Base case
    std::string FormatString(const std::string &format) { return format; }

    // Floating-point specialization for {:.2f}
    template <typename T, typename... Args>
    typename std::enable_if<std::is_floating_point<T>::value, std::string>::type
    FormatString(const std::string &format, T value, Args... args)
    {
        size_t pos = format.find("{:.2f}");
        if (pos != std::string::npos)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << value;
            std::string result = format.substr(0, pos) + oss.str() + format.substr(pos + 6);
            return FormatString(result, args...);
        }
        // Fallback to {} if {:.2f} not found
        pos = format.find("{}");
        if (pos != std::string::npos)
        {
            std::ostringstream oss;
            oss << value;
            std::string result = format.substr(0, pos) + oss.str() + format.substr(pos + 2);
            return FormatString(result, args...);
        }
        return format;
    }

    // General case for non-floating-point types
    template <typename T, typename... Args>
    typename std::enable_if<!std::is_floating_point<T>::value, std::string>::type
    FormatString(const std::string &format, T value, Args... args)
    {
        size_t pos = format.find("{}");
        if (pos != std::string::npos)
        {
            std::ostringstream oss;
            oss << value;
            std::string result = format.substr(0, pos) + oss.str() + format.substr(pos + 2);
            return FormatString(result, args...);
        }
        return format;
    }

    // Overload for const char*
    std::string FormatString(const std::string &format, const char *value)
    {
        size_t pos = format.find("{}");
        if (pos == std::string::npos)
            return format;
        std::ostringstream oss;
        oss << value;
        std::string result = format.substr(0, pos) + oss.str() + format.substr(pos + 2);
        return result;
    }

    std::string GetTimestamp();
    std::string GetLevelString(LogLevel level);
    void WriteToFile(const std::string &message);
    void WriteToConsole(const std::string &message);

    LogLevel m_currentLevel;
    std::string m_logFile;
    std::ofstream m_fileStream;
    bool m_consoleEnabled;
    std::mutex m_mutex;
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::Instance().Debug(msg)
#define LOG_INFO(msg) Logger::Instance().Info(msg)
#define LOG_WARNING(msg) Logger::Instance().Warning(msg)
#define LOG_ERROR(msg) Logger::Instance().Error(msg)
#define LOG_FATAL(msg) Logger::Instance().Fatal(msg)

#define LOG_DEBUG_FMT(fmt, ...) Logger::Instance().DebugFmt(fmt, ##__VA_ARGS__)
#define LOG_INFO_FMT(fmt, ...) Logger::Instance().InfoFmt(fmt, ##__VA_ARGS__)
#define LOG_WARNING_FMT(fmt, ...) Logger::Instance().WarningFmt(fmt, ##__VA_ARGS__)
#define LOG_ERROR_FMT(fmt, ...) Logger::Instance().ErrorFmt(fmt, ##__VA_ARGS__)
