#ifndef NNG_DOWNLOADER_H
#define NNG_DOWNLOADER_H

#include <cstdint>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <future>
#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <cstring>
#include <algorithm>
#include <sys/stat.h>
#include <iomanip>
#include <shared_mutex>

#ifdef WIN32
#ifdef NNG_SDK_EXPORTS
#ifdef __cplusplus
#define NNG_API(x) extern "C" __declspec(dllexport) x
#else
#define NNG_API(x) __declspec(dllexport) x
#endif
#else
#ifdef __cplusplus
#define NNG_API(x) extern "C" __declspec(dllimport) x
#else
#define NNG_API(x) __declspec(dllimport) x
#endif
#endif
#else
#ifdef __cplusplus
#define NNG_API(x) extern "C" __attribute__((visibility("default"))) x
#else
#define NNG_API(x) __attribute__((visibility("default"))) x
#endif
#endif

// 任务状态码
typedef enum nng_dl_task_state_code_t {
    NNG_DL_TASK_STATUS_UNKOWN            = 0,  // 未知状态
    NNG_DL_TASK_STATUS_START_WAITING     = 3,  // 开始等待
    NNG_DL_TASK_STATUS_START_PENDING     = 4,  // 开始挂起
    NNG_DL_TASK_STATUS_STARTED           = 5,  // 已启动
    NNG_DL_TASK_STATUS_STOP_PENDING      = 6,  // 停止挂起
    NNG_DL_TASK_STATUS_STOPED            = 7,  // 已停止
    NNG_DL_TASK_STATUS_SUCCEEDED         = 8,  // 成功
    NNG_DL_TASK_STATUS_FAILED            = 9,  // 失败
    NNG_DL_TASK_STATUS_PAUSED            = 10, // 已暂停
} nng_dl_task_status;

// 任务token状态
typedef enum nng_dl_task_token_err_t {
    NNG_DL_TASK_TOKEN_NORMAL = 0,        // 正常
    NNG_DL_TASK_TOKEN_EXPIRED,           // 令牌过期
    NNG_DL_TASK_TOKEN_SESSION_EXPIRED,   // 会话过期
    NNG_DL_TASK_TOKEN_OTHERS,            // 其他错误
} nng_dl_task_token_err;

typedef struct nng_dl_create_p2sp_info_t {
    const char* save_name;              // 保存名称
    const char* save_path;              // 保存路径
    const char* url;                    // URL
} nng_dl_create_p2sp_info;

typedef struct nng_dl_file_item_t {
    std::string save_name;              // 文件名
    std::string save_path;              // 文件保存路径
    std::string  url;                   // 任务URL
    std::string hash;                   // 文件哈希值
    uint64_t file_size;           // 文件总大小
    std::vector<uint64_t> chunk_task_ids;  // 文件分片任务列表
    uint32_t chunk_count;            // 文件分片数量
    uint32_t finish_chunk;          // 已下载完成分片数量
} nng_dl_file_item;


typedef struct nng_dl_files_info_t {
    uint32_t file_count; // 文件数量
    nng_dl_file_item* file_list; // 文件列表
} nng_dl_files_info;

typedef struct nng_dl_create_batch_info_t {
    const char* task_name; // 任务名称
    uint32_t max_concurrent; // 最大并发数，默认20
    nng_dl_files_info* batch_files;
} nng_dl_create_batch_info;

typedef struct nng_dl_init_param_t {
    const char* app_id;                    // 应用ID
    const char* app_version;              // 应用版本
    const char* cfg_path;                 // 配置文件路径
    uint8_t     save_tasks;               // 是否保存任务
} nng_dl_init_param;

typedef enum nng_dl_error_t {
    NNG_DL_ERROR_SUCCESS                         = 0,  // 成功
    NNG_DL_ERROR_FAILED                          = 1,  // 失败
    NNG_DL_ERROR_ALREADY_INIT                    = 9101,  // 已经初始化
    NNG_DL_ERROR_SDK_NOT_INIT                    = 9102,  // SDK未初始化
    NNG_DL_ERROR_TASK_ALREADY_EXIST              = 9103,  // 任务已存在
    NNG_DL_ERROR_TASK_NOT_EXIST                  = 9104,  // 任务不存在
    NNG_DL_ERROR_TASK_ALREADY_STOPPED            = 9105,  // 任务已停止
    NNG_DL_ERROR_TASK_ALREADY_RUNNING            = 9106,  // 任务已在运行
    NNG_DL_ERROR_TASK_NOT_START                  = 9107,  // 任务未启动
    NNG_DL_ERROR_TASK_STILL_RUNNING              = 9108,  // 任务仍在运行
    NNG_DL_ERROR_FILE_EXISTED                    = 9109,  // 文件已存在
    NNG_DL_ERROR_DISK_FULL                       = 9110,  // 磁盘已满
    NNG_DL_ERROR_TOO_MUCH_TASK                   = 9111,  // 任务过多
    NNG_DL_ERROR_PARAM_ERROR                     = 9112,  // 参数错误
    NNG_DL_ERROR_SCHEMA_NOT_SUPPORT              = 9113,  // 不支持的模式
    NNG_DL_ERROR_DYNAMIC_PARAM_FAIL              = 9114,  // 动态参数设置失败
    NNG_DL_ERROR_CONTINUE_NO_NAME                = 9115,  // 继续时没有名称
    NNG_DL_ERROR_APPNAME_APPKEY_ERROR            = 9116,  // 应用名和应用密钥错误
    NNG_DL_ERROR_CREATE_THREAD_ERROR             = 9117,  // 创建线程错误
    NNG_DL_ERROR_TASK_FINISH                     = 9118,  // 任务已完成
    NNG_DL_ERROR_TASK_NOT_RUNNING                = 9119,  // 任务未运行
    NNG_DL_ERROR_TASK_NOT_IDLE                   = 9120,  // 任务未空闲
    NNG_DL_ERROR_TASK_TYPE_NOT_SUPPORT           = 9121,  // 不支持的任务类型
    NNG_DL_ERROR_ADD_RESOURCE_ERROR              = 9122,  // 添加资源错误
    NNG_DL_ERROR_FUNCTION_NOT_SUPPORT            = 9123,  // 不支持的功能
    NNG_DL_ERROR_ALREADY_HAS_FILENAME            = 9124,  // 已经有文件名
    NNG_DL_ERROR_FILE_NAME_TOO_LONG              = 9125,  // 文件名过长
    NNG_DL_ERROR_ONE_PATH_LEVEL_NAME_TOO_LONG    = 9126,  // 路径层级名称过长
    NNG_DL_ERROR_FULL_PATH_NAME_TOO_LONG         = 9127,  // 完整路径名称过长
    NNG_DL_ERROR_FULL_PATH_NAME_OCCUPIED         = 9128,  // 完整路径名称已被占用
    NNG_DL_ERROR_TASK_NO_FILE_NAME               = 9129,  // 任务没有文件名
    NNG_DL_ERROR_NOT_WIFI_MODE                   = 9130,  // 不是Wi-Fi模式
    NNG_DL_ERROR_SPEED_LIMIT_TO_SMALL            = 9131,  // 速度限制过小
    NNG_DL_ERROR_TASK_CONTROL_STRATEGY           = 9501,  // 任务控制策略错误
    NNG_DL_ERROR_URL_IS_TOO_LONG                 = 9502,  // URL过长
    NNG_DL_ERROR_FILE_DELETE_FAIL                = 9503,  // 删除文件失败
    NNG_DL_ERROR_FILE_NOT_EXIST                  = 9504,  // 文件不存在
    NNG_DL_ERROR_INFO_NAME_NOT_SUPPORT           = 9505,  // 不支持的信息名称
    NNG_DL_ERROR_MEMORY_TOO_SMALL                = 9601,  // 内存太小
    NNG_DL_ERROR_AUTH_TOKEN_VERIFY_FAILED        = 9602,  // 验证令牌失败
    NNG_DL_ERROR_AUTH_SCOPE_VERIFY_FAILED        = 9603,  // 验证范围失败
    NNG_DL_ERROR_AUTH_SESSION_ID_VERIFY_FAILED   = 9604,  // 验证会话ID失败
    NNG_DL_ERROR_AUTH_SESSION_ID_EXPIRED         = 9605,  // 会话ID已过期
    NNG_DL_ERROR_AUTH_RES_HAS_NO_QUOTA           = 9606,  // 资源没有配额
    NNG_DL_ERROR_INSUFFICIENT_DISK_SPACE         = 111085,  // 磁盘空间不足
    NNG_DL_ERROR_OPEN_FILE_ERR                   = 111128,  // 打开文件错误
    NNG_DL_ERROR_NO_DATA_PIPE                    = 111136,  // 没有数据管道
    NNG_DL_ERROR_RESTRICTION                     = 111151,  // 限制
    NNG_DL_ERROR_ACCOUNT_EXCEPTION               = 111152,  // 账户异常
    NNG_DL_ERROR_RESTRICTION_AREA                = 111153,  // 限制区域
    NNG_DL_ERROR_COPYRIGHT_BLOCKING              = 111154,  // 版权阻止
    NNG_DL_ERROR_TYPE2_BLOCKING                  = 111155,  // 类型2阻止
    NNG_DL_ERROR_TYPE3_BLOCKING                  = 111156,  // 类型3阻止
    NNG_DL_ERROR_LONG_TIME_NO_RECV_DATA          = 111176,  // 长时间没有接收数据
    NNG_DL_ERROR_TIME_OUT                        = 119212,  // 超时

    NNG_DL_ERROR_TASK_STATUS_ERR                 = 999999,  // 任务状态错误
} nng_dl_error;

typedef struct nng_dl_task_traffic_info_t {
    uint64_t origin_size;  // 原始大小
    uint64_t p2p_size;     // 对等网络大小
    uint64_t p2s_size;     // 对等单播大小
    uint64_t dcdn_size;    // 分布式内容分发网络大小
} nng_dl_task_traffic_info;

typedef struct nng_dl_task_state_t {
    uint64_t speed;             // 速度
    uint64_t total_size;        // 总大小
    uint64_t downloaded_size;   // 已下载大小
    uint8_t  state_code;        // 状态码
    uint32_t task_err_code;     // 任务错误码
    uint32_t task_token_err;    // 任务令牌错误
    uint32_t priority;          // 任务优先级，值越小优先级越高
} nng_dl_task_state;

class NNGDownloader {
public:
    static int32_t init(const nng_dl_init_param* param);  // 初始化
    static int32_t uninit();  // 反初始化

    static int32_t login(const char* login_token, char* session_id);  // 登录

    static int32_t get_unfinished_tasks(uint64_t* task_id_array, uint32_t* count);  // 获取未完成任务
    static int32_t get_finished_tasks(uint64_t* task_id_array, uint32_t* count);  // 获取已完成任务

    static int32_t create_server_task(const nng_dl_file_item* create_info, uint64_t* task_id);  // 创建服务器任务
    static int32_t create_batch_task(const nng_dl_create_batch_info* create_info, uint64_t* task_id);  // 创建批量任务
    // static int32_t create_p2sp_task(const nng_dl_create_p2sp_info* create_info, uint64_t* task_id);  // 创建P2SP任务

    static int32_t set_task_token(uint64_t task_id, const char* task_token);  // 设置任务令牌

    static void    schedule_and_start_tasks();//任务排序并开始任务
    static int32_t execute_task(uint64_t task_id);  //执行任务
    static int32_t pause_task(uint64_t task_id);  // 暂停任务
    static int32_t stop_task(uint64_t task_id);  // 停止任务
    static int32_t delete_task(uint64_t task_id, uint8_t delete_file_flag);  // 删除任务及文件,delete_file_flag: 0-不删除文件，1-删除文件

    static int32_t get_task_state(uint64_t task_id, nng_dl_task_state* state);  // 获取任务状态

    // 支持的信息名称： "url", "save_path", "save_name", "traffic"
    static int32_t get_task_info(uint64_t task_id, const char* info_name, void* buff, uint32_t* buff_len);  // 获取任务信息

    static int32_t set_concurrent_task_count(uint32_t count);  // 设置并发任务数
    static int32_t set_download_speed_limit(uint32_t speed);  // 设置下载速度限制
    static int32_t set_upload_switch(uint32_t upload_switch);  // 设置上传开关
    static int32_t set_upload_speed_limit(uint32_t speed);  // 设置上传速度限制

    static int32_t version(char* buff, uint32_t* buff_len);  // 获取版本
    static int32_t set_task_priority(uint64_t task_id, uint32_t priority);// 设置任务优先级
    static int32_t get_task_priority(uint64_t task_id, uint32_t* priority); // 获取任务优先级
    static double getOriginalFileProgress(uint64_t task_id); // 获取原始文件下载进度
private:
    static std::atomic<bool> g_sdk_initialized; // SDK是否已初始化
    static std::shared_mutex g_task_mutex; // 任务管理锁
    static std::unordered_map<uint64_t, nng_dl_task_state> g_task_states; // 任务状态
    static std::unordered_map<uint64_t, nng_dl_file_item> g_task_info; // 任务信息
    static uint64_t g_next_task_id; // 下一个任务ID
    static std::atomic<uint32_t> g_concurrent_task_count; // 当前并发任务数
    static uint32_t g_max_concurrent_task_count; // 最大并发任务数
    static uint32_t g_download_speed_limit; // 下载速度限制（0表示不限制）
    static std::atomic<bool> g_upload_switch; // 上传开关
    static uint32_t g_upload_speed_limit; // 上传速度限制（0表示不限制）
    static std::queue<uint64_t> g_pending_tasks;//全局任务队列
    static std::atomic<bool> m_progressEnabled; // 控制进度回调的标志位

    struct HeaderInfo {
        std::string x_file_md5;
        uint64_t file_size;
    };

    class CoroutinePool {
    public:
        CoroutinePool(int maxCoroutineNum) : maxCoroutineNum(maxCoroutineNum), stop(false) {
            for (int i = 0; i < maxCoroutineNum; ++i) {
                coroutines.emplace_back(async(std::launch::async, &CoroutinePool::coroutineRunner, this));
            }
        }

        ~CoroutinePool() {
            stop = true;
            condition.notify_all();
            for (auto& future : coroutines) {
                if (future.valid()) future.wait();
            }
        }

        void submitTask(const std::function<void()>& task) {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks.push(task);
            condition.notify_one();
        }

    private:
        int maxCoroutineNum=1;
        std::vector<std::future<void>> coroutines;
        std::queue<std::function<void()>> tasks;
        std::mutex mutex_;
        std::condition_variable condition;
        bool stop;

        void coroutineRunner() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    condition.wait(lock, [this]() { return stop || !tasks.empty(); });
                    if (stop && tasks.empty()) break;
                    task = tasks.front();
                    tasks.pop();
                }
                task();
            }
        }
    };

    static CoroutinePool* g_coroutine_pool;
    
    static uint32_t ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow);// 进度回调函数
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s);
    static std::string calculateMD5(const std::string& filePath);
    static size_t HeaderCallback(void* ptr, size_t size, size_t nmemb, void* userdata);
   
    static void downloadFile(const nng_dl_file_item& file, uint64_t task_id);// 计算分片信息并启动分片下载
    static void downloadFileInChunks(const nng_dl_file_item& chunk_file, uint64_t task_id, uint64_t start_byte, uint64_t end_byte, uint64_t file_size); // 下载分片函数
    static void mergeChunks(const nng_dl_file_item& original_file, const std::vector<uint64_t>& chunk_task_ids, const std::string& server_hash);// 合并分片函数
};

#endif