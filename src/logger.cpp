#include "logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdarg>
#include <cstdio>

Logger::Logger()
    : m_currentLevel(LogLevel::INFO), m_consoleEnabled(true)
{
}

Logger::~Logger()
{
    if (m_fileStream.is_open())
    {
        m_fileStream.close();
    }
}

Logger &Logger::Instance()
{
    static Logger instance;
    return instance;
}

void Logger::SetLogLevel(LogLevel level)
{
    m_currentLevel = level;
}

void Logger::SetLogFile(const std::string &filename)
{
    if (m_fileStream.is_open())
    {
        m_fileStream.close();
    }

    m_logFile = filename;
    if (!filename.empty())
    {
        m_fileStream.open(filename, std::ios::app);
        if (!m_fileStream.is_open())
        {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
    }
}

void Logger::EnableConsole(bool enable)
{
    m_consoleEnabled = enable;
}

void Logger::Debug(const std::string &message)
{
    Log(LogLevel::DEBUG, message);
}

void Logger::Info(const std::string &message)
{
    Log(LogLevel::INFO, message);
}

void Logger::Warning(const std::string &message)
{
    Log(LogLevel::WARNING, message);
}

void Logger::Error(const std::string &message)
{
    Log(LogLevel::LOG_ERROR, message);
}

void Logger::Fatal(const std::string &message)
{
    Log(LogLevel::FATAL, message);
}

void Logger::Log(LogLevel level, const std::string &message)
{
    if (level < m_currentLevel)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string timestamp = GetTimestamp();
    std::string levelStr = GetLevelString(level);
    std::string fullMessage = timestamp + " [" + levelStr + "] " + message + "\n";

    WriteToFile(fullMessage);
    WriteToConsole(fullMessage);
}

std::string Logger::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::GetLevelString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO ";
    case LogLevel::WARNING:
        return "WARN ";
    case LogLevel::LOG_ERROR:
        return "ERROR";
    case LogLevel::FATAL:
        return "FATAL";
    default:
        return "UNKN ";
    }
}

void Logger::WriteToFile(const std::string &message)
{
    if (m_fileStream.is_open())
    {
        m_fileStream << message;
        m_fileStream.flush();
    }
}

void Logger::WriteToConsole(const std::string &message)
{
    if (m_consoleEnabled)
    {
        std::cout << message;
        std::cout.flush();
    }
}
