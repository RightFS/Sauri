//
// Created by Right on 25/3/13 星期四 11:51.
//

#pragma once
#ifndef NNGAME_NNGAMEDOWNLOADER_H
#define NNGAME_NNGAMEDOWNLOADER_H
#include "nngamedef.h"

#ifdef __cplusplus
extern "C" {
#endif


NNGAME_API int NNGame_DownloaderGetTasks(int64_t userid, int64_t* taskids, int64_t* taskids_size);




#ifdef __cplusplus
}
#endif

enum class DownloadStatus {
    PENDING,    // 等待下载
    QUEUED,     // 已在队列中
    DOWNLOADING,// 下载中
    PAUSED,     // 已暂停
    COMPLETED,  // 已完成
    FAILED,     // 失败
    CANCELED    // 已取消
};

enum class TaskPriority {
    LOW,
    NORMAL,
    HIGH
};

struct DownloadProgress {
    uint64_t downloadedBytes;   // 已下载字节数
    uint64_t totalBytes;        // 文件总字节数
    double progress;            // 下载进度 (0-1)
    uint64_t speed;             // 当前下载速度 (bytes/s)
    double estimatedTimeLeft;   // 估计剩余时间 (秒)
};
class NNGameDownloader {
public:
    virtual int GetTasks(int64_t userid, int64_t* taskids, int64_t* taskids_size) = 0;
};

class IDownloadTask {
public:
    virtual ~IDownloadTask() = default;

    // 基本控制
    virtual void start() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void cancel() = 0;

    // 获取任务ID
    virtual int getId() const = 0;

    // 获取游戏名称
    virtual std::string getGameName() const = 0;

    // 获取URL
    virtual std::string getUrl() const = 0;

    // 获取保存路径
    virtual std::string getSavePath() const = 0;

    // 获取当前状态
    virtual DownloadStatus getStatus() const = 0;

    // 获取当前进度
    virtual DownloadProgress getProgress() const = 0;

    // 设置优先级
    virtual void setPriority(TaskPriority priority) = 0;
    virtual TaskPriority getPriority() const = 0;

    // 获取错误信息
    virtual std::string getErrorMessage() const = 0;

    // 设置回调函数
    virtual void setProgressCallback(std::function<void(const DownloadProgress&)> callback) = 0;
    virtual void setCompletionCallback(std::function<void()> callback) = 0;
    virtual void setFailureCallback(std::function<void(const std::string&)> callback) = 0;

    // 支持断点续传
    virtual bool supportsResume() const = 0;

    // 检验下载内容 (哈希验证)
    virtual bool verifyIntegrity(const std::string& expectedHash) = 0;
};

class IDownloadManager {
public:
    virtual ~IDownloadManager() = default;

    // 添加新的下载任务，返回任务ID
    virtual int addDownloadTask(const std::string& url,
                                const std::string& savePath,
                                const std::string& gameName,
                                uint64_t totalSize = 0) = 0;

    // 通过任务ID获取下载任务
    virtual std::shared_ptr<IDownloadTask> getTask(int taskId) const = 0;

    // 获取所有下载任务
    virtual std::vector<std::shared_ptr<IDownloadTask>> getAllTasks() const = 0;

    // 暂停所有下载
    virtual void pauseAll() = 0;

    // 恢复所有下载
    virtual void resumeAll() = 0;

    // 取消所有下载
    virtual void cancelAll() = 0;

    // 设置全局下载速度限制 (bytes/s, 0表示无限制)
    virtual void setGlobalSpeedLimit(uint64_t bytesPerSecond) = 0;

    // 获取当前总下载速度
    virtual uint64_t getCurrentTotalSpeed() const = 0;

    // 设置最大并行下载数量
    virtual void setMaxConcurrentDownloads(int count) = 0;

    // 设置下载任务优先级变更回调
    virtual void setOnPriorityChangedCallback(std::function<void(int taskId, int newPriority)> callback) = 0;

    // 设置存储空间检查器
    virtual void setStorageChecker(std::function<bool(const std::string& path, uint64_t requiredSpace)> checker) = 0;
};

class IDownloadQueue {
public:
    virtual ~IDownloadQueue() = default;

    // 添加任务到队列
    virtual void addTask(std::shared_ptr<IDownloadTask> task) = 0;

    // 从队列中移除任务
    virtual void removeTask(int taskId) = 0;

    // 获取下一个应该执行的任务
    virtual std::shared_ptr<IDownloadTask> getNextTask() = 0;

    // 获取所有排队中的任务
    virtual std::vector<std::shared_ptr<IDownloadTask>> getAllQueuedTasks() const = 0;

    // 调整任务优先级
    virtual void reorderTask(int taskId, int newPosition) = 0;

    // 根据优先级重新排序队列
    virtual void reorderByPriority() = 0;

    // 清空队列
    virtual void clear() = 0;

    // 获取队列中的任务数
    virtual size_t size() const = 0;

    // 检查队列是否为空
    virtual bool isEmpty() const = 0;
};

// 网络请求头部
struct HttpHeader {
    std::string name;
    std::string value;
};

// 代理设置
struct ProxySettings {
    std::string host;
    int port;
    std::string username;
    std::string password;
    bool enabled;
};

class INetworkTransfer {
public:
    virtual ~INetworkTransfer() = default;

    // 初始化传输
    virtual bool initialize(const std::string& url, const std::string& savePath) = 0;

    // 开始传输
    virtual bool start() = 0;

    // 暂停传输
    virtual bool pause() = 0;

    // 恢复传输
    virtual bool resume() = 0;

    // 取消传输
    virtual bool cancel() = 0;

    // 设置请求头
    virtual void setHeaders(const std::vector<HttpHeader>& headers) = 0;

    // 设置代理
    virtual void setProxy(const ProxySettings& proxy) = 0;

    // 设置分片下载 (多线程下载同一文件)
    virtual void setChunkedDownload(int numChunks) = 0;

    // 设置速度限制 (bytes/s, 0表示无限制)
    virtual void setSpeedLimit(uint64_t bytesPerSecond) = 0;

    // 获取当前速度
    virtual uint64_t getCurrentSpeed() const = 0;

    // 获取已下载的字节数
    virtual uint64_t getDownloadedBytes() const = 0;

    // 获取总字节数
    virtual uint64_t getTotalBytes() const = 0;

    // 检查是否支持断点续传
    virtual bool isResumeSupported() const = 0;

    // 设置进度回调函数
    virtual void setProgressCallback(std::function<void(uint64_t downloaded, uint64_t total)> callback) = 0;

    // 设置完成回调函数
    virtual void setCompletionCallback(std::function<void()> callback) = 0;

    // 设置错误回调函数
    virtual void setErrorCallback(std::function<void(const std::string& error)> callback) = 0;

    // 获取最后的错误信息
    virtual std::string getLastError() const = 0;
};

class IConfigManager {
public:
    virtual ~IConfigManager() = default;

    // 下载设置
    virtual void setDefaultDownloadLocation(const std::string& location) = 0;
    virtual std::string getDefaultDownloadLocation() const = 0;

    virtual void setMaxConcurrentDownloads(int count) = 0;
    virtual int getMaxConcurrentDownloads() const = 0;

    virtual void setGlobalSpeedLimit(uint64_t bytesPerSecond) = 0;
    virtual uint64_t getGlobalSpeedLimit() const = 0;

    // 网络设置
    virtual void setProxySettings(const ProxySettings& settings) = 0;
    virtual ProxySettings getProxySettings() const = 0;

    virtual void setDefaultHeaders(const std::vector<HttpHeader>& headers) = 0;
    virtual std::vector<HttpHeader> getDefaultHeaders() const = 0;

    // 下载行为设置
    virtual void setAutoResumeDownloads(bool enabled) = 0;
    virtual bool getAutoResumeDownloads() const = 0;

    virtual void setDefaultChunkCount(int count) = 0;
    virtual int getDefaultChunkCount() const = 0;

    // 保存和加载配置
    virtual bool saveConfig() = 0;
    virtual bool loadConfig() = 0;
};

enum class DownloadEvent {
    TASK_ADDED,
    TASK_STARTED,
    TASK_PAUSED,
    TASK_RESUMED,
    TASK_COMPLETED,
    TASK_FAILED,
    TASK_CANCELED,
    TASK_PROGRESS_UPDATED,
    QUEUE_CHANGED,
    SPEED_LIMIT_CHANGED,
    NETWORK_ERROR,
    STORAGE_ERROR
};

struct DownloadEventData {
    DownloadEvent eventType;
    int taskId;
    std::shared_ptr<IDownloadTask> task;
    DownloadProgress progress;
    std::string errorMessage;
    // 其他事件相关数据...
};

class IDownloadEventNotifier {
public:
    virtual ~IDownloadEventNotifier() = default;

    // 注册事件监听器
    virtual void registerListener(DownloadEvent event, std::function<void(const DownloadEventData&)> listener) = 0;

    // 移除事件监听器
    virtual void unregisterListener(DownloadEvent event) = 0;

    // 触发事件 (内部使用)
    virtual void notifyEvent(const DownloadEventData& eventData) = 0;

    // 移除所有监听器
    virtual void clearListeners() = 0;
};
#endif //NNGAME_NNGAMEDOWNLOADER_H
