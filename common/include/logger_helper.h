//
// Created by Right on 25/5/16 14:57.
//

#pragma once

#define WIN32_LEAN_AND_MEAN
#define ELPP_WINSOCK2 1
#include <easylogging++.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <chrono>


class LoggerHelper {
public:
    static void Initialize(int maxDaysToKeep = 7, const std::string& logRootPath = "logs");

private:
    static std::string s_logRootPath;
    static int s_maxDaysToKeep;
    static std::string s_dateFormat;
    static std::string s_timeFormat;
    static std::string s_fullFormat;
    static std::string s_currentDate;

    static void ConfigureLogger();

    static void CleanOldLogs();

    class LogRotationDispatcher : public el::LogDispatchCallback {
    protected:
        void handle(const el::LogDispatchData* data) noexcept override;

    private:
        const el::LogDispatchData* m_data{};

        void dispatch(el::base::type::string_t&& logLine) noexcept;
    };
};



// Usage example
inline void InitializeLogger(int daysToKeep = 7, const std::string& logPath = "logs") {
    LoggerHelper::Initialize(daysToKeep, logPath);
}