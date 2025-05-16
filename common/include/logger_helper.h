//
// Created by Right on 25/5/16 星期五 14:57.
//

#pragma once

#include <easylogging++.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <chrono>

INITIALIZE_EASYLOGGINGPP

class LoggerHelper {
public:
    static void Initialize(int maxDaysToKeep = 7, const std::string& logRootPath = "logs") {
        s_logRootPath = logRootPath;
        s_maxDaysToKeep = maxDaysToKeep;
        s_dateFormat = "%Y-%m-%d";
        s_timeFormat = "%M/%d %H:%m:%s";
        s_fullFormat = "%datetime{" + s_timeFormat + "} %msg";

        // Create log directory if it doesn't exist
        std::filesystem::create_directories(s_logRootPath);

        // Configure logger
        ConfigureLogger();

        // Set up log rotation
        el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
        el::Helpers::installLogDispatchCallback<LogRotationDispatcher>("LogRotationDispatcher");
        auto* dispatcher = el::Helpers::logDispatchCallback<LogRotationDispatcher>("LogRotationDispatcher");
        dispatcher->setEnabled(true);

        // Clean old logs on startup
        CleanOldLogs();
    }

private:
    static std::string s_logRootPath;
    static int s_maxDaysToKeep;
    static std::string s_dateFormat;
    static std::string s_timeFormat;
    static std::string s_fullFormat;
    static std::string s_currentDate;

    static void ConfigureLogger() {
        el::base::SubsecondPrecision logSsPrec(3);
        s_currentDate = el::base::utils::DateTime::getDateTime(s_dateFormat.c_str(), &logSsPrec);

        el::Configurations defaultConf;
        defaultConf.setToDefault();

        // Global settings
        defaultConf.setGlobally(el::ConfigurationType::Format, s_fullFormat.c_str());
        defaultConf.setGlobally(el::ConfigurationType::Enabled, "true");
        defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
        defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "true");
        defaultConf.setGlobally(el::ConfigurationType::SubsecondPrecision, "3");
        defaultConf.setGlobally(el::ConfigurationType::PerformanceTracking, "false");
        defaultConf.setGlobally(el::ConfigurationType::LogFlushThreshold, "0");
        defaultConf.setGlobally(el::ConfigurationType::MaxLogFileSize, "512000000"); // 500MB

        // Create folder for current date
        std::string dateFolder = s_logRootPath + "/" + s_currentDate;
        std::filesystem::create_directories(dateFolder);

        // Configure different log levels
        defaultConf.set(el::Level::Info, el::ConfigurationType::Filename, (dateFolder + "/info.log").c_str());
        defaultConf.set(el::Level::Trace, el::ConfigurationType::Filename, (dateFolder + "/trace.log").c_str());
        defaultConf.set(el::Level::Debug, el::ConfigurationType::Filename, (dateFolder + "/debug.log").c_str());
        defaultConf.set(el::Level::Warning, el::ConfigurationType::Filename, (dateFolder + "/warning.log").c_str());
        defaultConf.set(el::Level::Error, el::ConfigurationType::Filename, (dateFolder + "/error.log").c_str());
        defaultConf.set(el::Level::Fatal, el::ConfigurationType::Filename, (dateFolder + "/fatal.log").c_str());

        // Specific settings
        defaultConf.set(el::Level::Trace, el::ConfigurationType::ToStandardOutput, "false");
        defaultConf.set(el::Level::Trace, el::ConfigurationType::Format, "[%datetime] %msg");

        el::Loggers::reconfigureAllLoggers(defaultConf);
    }

    static void CleanOldLogs() {
        try {
            if (!std::filesystem::exists(s_logRootPath)) {
                return;
            }

            std::vector<std::filesystem::path> dateDirs;
            for (const auto& entry : std::filesystem::directory_iterator(s_logRootPath)) {
                if (entry.is_directory()) {
                    dateDirs.push_back(entry.path());
                }
            }

            // If we have more directories than max days to keep
            if (dateDirs.size() > static_cast<size_t>(s_maxDaysToKeep)) {
                // Sort by name (which should be dates in YYYY-MM-DD format)
                std::sort(dateDirs.begin(), dateDirs.end());

                // Delete oldest directories
                size_t dirsToDelete = dateDirs.size() - s_maxDaysToKeep;
                for (size_t i = 0; i < dirsToDelete; i++) {
                    std::filesystem::remove_all(dateDirs[i]);
                }
            }
        } catch (const std::exception& e) {
            // Don't throw from here, just log the error
            std::cerr << "Error cleaning old logs: " << e.what() << std::endl;
        }
    }

    class LogRotationDispatcher : public el::LogDispatchCallback {
    protected:
        void handle(const el::LogDispatchData* data) noexcept override {
            m_data = data;
            dispatch(m_data->logMessage()->logger()->logBuilder()->build(
                    m_data->logMessage(),
                    m_data->dispatchAction() == el::base::DispatchAction::NormalLog));
        }

    private:
        const el::LogDispatchData* m_data{};

        void dispatch(el::base::type::string_t&& logLine) noexcept {
            try {
                el::base::SubsecondPrecision ssPrec(3);
                std::string currentDate = el::base::utils::DateTime::getDateTime(s_dateFormat.c_str(), &ssPrec);

                // Check if date has changed
                if (currentDate != s_currentDate) {
                    s_currentDate = currentDate;
                    ConfigureLogger();
                    CleanOldLogs();
                }
            } catch (...) {
                // Silently fail in case of any error
            }
        }
    };
};

// Initialize static members
std::string LoggerHelper::s_logRootPath = "logs";
int LoggerHelper::s_maxDaysToKeep = 7;
std::string LoggerHelper::s_dateFormat = "%Y-%m-%d";
std::string LoggerHelper::s_timeFormat = "%M/%d %H:%m:%s";
std::string LoggerHelper::s_fullFormat = "%datetime{%M/%d %H:%m:%s} %msg";
std::string LoggerHelper::s_currentDate;

// Usage example
inline void InitializeLogger(int daysToKeep = 7, const std::string& logPath = "logs") {
    LoggerHelper::Initialize(daysToKeep, logPath);
}