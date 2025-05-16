#include "downloader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <functional>
#include <algorithm>
#include <unordered_map>
#include <sys/stat.h>
#include <iomanip>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

// 静态成员初始化
atomic<bool> NNGDownloader::g_sdk_initialized(false);
shared_mutex NNGDownloader::g_task_mutex; // 修改为读写锁
unordered_map<uint64_t, nng_dl_task_state> NNGDownloader::g_task_states;
unordered_map<uint64_t, nng_dl_file_item> NNGDownloader::g_task_info;
uint64_t NNGDownloader::g_next_task_id = 1;
atomic<uint32_t> NNGDownloader::g_concurrent_task_count(0);
uint32_t NNGDownloader::g_max_concurrent_task_count = 20;
uint32_t NNGDownloader::g_download_speed_limit = 1024*1000; // 单位是KB/s，默认为不限速1000MB/s
atomic<bool> NNGDownloader::g_upload_switch(false);
uint32_t NNGDownloader::g_upload_speed_limit = 100;
NNGDownloader::CoroutinePool* NNGDownloader::g_coroutine_pool = nullptr;
queue<uint64_t> NNGDownloader::g_pending_tasks;
atomic<bool> NNGDownloader::m_progressEnabled(false);

// 写回调函数
size_t NNGDownloader::WriteCallback(void* contents, size_t size, size_t nmemb, string* s) {
    size_t newLength = size * nmemb;
    s->append((char*)contents, newLength);
    return newLength;
}

uint32_t NNGDownloader::ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow) {
    static std::chrono::steady_clock::time_point last_update_time = std::chrono::steady_clock::now();
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_update_time).count();

    if (elapsed_ms < 100) {
        return 0;
    }
    last_update_time = current_time;

    uint64_t task_id = *(uint64_t*)clientp;

    unique_lock<shared_mutex> lock(g_task_mutex); // 写操作使用独占锁

    auto it = g_task_states.find(task_id);
    if (it != g_task_states.end()) {
        it->second.downloaded_size = static_cast<uint64_t>(dlnow);
        it->second.total_size = static_cast<uint64_t>(dltotal);

        auto it_task_info = g_task_info.find(task_id);
        if (it_task_info != g_task_info.end() && !it_task_info->second.chunk_task_ids.empty()) {
            uint64_t original_task_id = it_task_info->second.chunk_task_ids[0];
            auto original_it = g_task_states.find(original_task_id);
            if (original_it != g_task_states.end()) {
                uint64_t total_downloaded = 0;
                uint64_t total_size = 0;

                for (uint64_t chunk_task_id : it_task_info->second.chunk_task_ids) {
                    auto chunk_it = g_task_states.find(chunk_task_id);
                    if (chunk_it != g_task_states.end()) {
                        total_downloaded += chunk_it->second.downloaded_size;
                        total_size += chunk_it->second.total_size;
                    }
                }

                original_it->second.downloaded_size = total_downloaded;
                original_it->second.total_size = total_size;
            }
        }
    }
    return 0;
}

// 计算MD5
string NNGDownloader::calculateMD5(const string& filePath) {
    u8string u8Path(filePath.begin(), filePath.end());
    filesystem::path fsPath(u8Path);

    ifstream file(fsPath, ios::binary);
    if (!file) {
        cerr << "Error: Unable to open file for MD5 calculation" << endl;
        return "";
    }

    EVP_MD_CTX* md5Context = EVP_MD_CTX_new();
    EVP_DigestInit_ex(md5Context, EVP_md5(), nullptr);

    char buffer[1024];
    while (file.read(buffer, sizeof(buffer))) {
        EVP_DigestUpdate(md5Context, buffer, file.gcount());
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength;
    EVP_DigestFinal_ex(md5Context, digest, &digestLength);
    EVP_MD_CTX_free(md5Context);

    stringstream ss;
    for (unsigned int i = 0; i < digestLength; ++i) {
        ss << hex << setw(2) << setfill('0') << (int)digest[i];
    }

    return ss.str();
}

// 初始化SDK
int32_t NNGDownloader::init(const nng_dl_init_param* param) {
    if (g_sdk_initialized) {
        return NNG_DL_ERROR_ALREADY_INIT;
    }

    if (param == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    g_coroutine_pool = new CoroutinePool(g_max_concurrent_task_count);

    g_sdk_initialized = true;
    return NNG_DL_ERROR_SUCCESS;
}

// 反初始化SDK
int32_t NNGDownloader::uninit() {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    vector<uint64_t> task_ids;
    {
        unique_lock<shared_mutex> lock(g_task_mutex); // 写操作使用独占锁
        for (const auto& pair : g_task_states) {
            task_ids.push_back(pair.first);
        }
    }

    for (auto task_id : task_ids) {
        stop_task(task_id);
    }

    delete g_coroutine_pool;
    g_coroutine_pool = nullptr;

    g_sdk_initialized = false;
    g_task_states.clear();
    g_task_info.clear();
    g_next_task_id = 1;
    g_concurrent_task_count = 0;

    curl_global_cleanup();

    return NNG_DL_ERROR_SUCCESS;
}

// 登录
int32_t NNGDownloader::login(const char* login_token, char* session_id) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    if (login_token == nullptr || session_id == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    strcpy(session_id, "mock_session_id_12345");
    return NNG_DL_ERROR_SUCCESS;
}

// 获取未完成任务
int32_t NNGDownloader::get_unfinished_tasks(uint64_t* task_id_array, uint32_t* count) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    if (task_id_array == nullptr || count == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    vector<uint64_t> unfinished_tasks;
    {
        shared_lock<shared_mutex> lock(g_task_mutex); // 读操作使用共享锁
        for (const auto& pair : g_task_states) {
            if (pair.second.state_code != NNG_DL_TASK_STATUS_SUCCEEDED &&
                pair.second.state_code != NNG_DL_TASK_STATUS_FAILED &&
                pair.second.state_code != NNG_DL_TASK_STATUS_STOPED) {
                unfinished_tasks.push_back(pair.first);
            }
        }
    }

    uint32_t task_count = static_cast<uint32_t>(unfinished_tasks.size());
    if (*count < task_count) {
        *count = task_count;
        return NNG_DL_ERROR_SUCCESS;
    }

    memcpy(task_id_array, unfinished_tasks.data(), task_count * sizeof(uint64_t));
    *count = task_count;
    return NNG_DL_ERROR_SUCCESS;
}

// 获取已完成任务
int32_t NNGDownloader::get_finished_tasks(uint64_t* task_id_array, uint32_t* count) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    if (task_id_array == nullptr || count == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    vector<uint64_t> finished_tasks;
    {
        shared_lock<shared_mutex> lock(g_task_mutex); // 读操作使用共享锁
        for (const auto& pair : g_task_states) {
            if (pair.second.state_code == NNG_DL_TASK_STATUS_SUCCEEDED ||
                pair.second.state_code == NNG_DL_TASK_STATUS_FAILED ||
                pair.second.state_code == NNG_DL_TASK_STATUS_STOPED) {
                finished_tasks.push_back(pair.first);
            }
        }
    }

    uint32_t task_count = static_cast<uint32_t>(finished_tasks.size());
    if (*count < task_count) {
        *count = task_count;
        return NNG_DL_ERROR_SUCCESS;
    }

    memcpy(task_id_array, finished_tasks.data(), task_count * sizeof(uint64_t));
    *count = task_count;
    return NNG_DL_ERROR_SUCCESS;
}

// 创建服务器任务
int32_t NNGDownloader::create_server_task(const nng_dl_file_item* create_info, uint64_t* task_id) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    if (create_info == nullptr || task_id == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    {
        unique_lock<shared_mutex> lock(g_task_mutex); // 写操作使用独占锁

        for (const auto& pair : g_task_states) {
            auto it = g_task_info.find(pair.first);
            if (it != g_task_info.end() && it->second.save_name == create_info->save_name) {
                if (pair.second.state_code != NNG_DL_TASK_STATUS_FAILED) {
                    return NNG_DL_ERROR_TASK_ALREADY_EXIST;
                }
            }
        }

        *task_id = g_next_task_id++;
        g_task_states[*task_id] = {10*1024, 0, 0, NNG_DL_TASK_STATUS_START_PENDING, 0, 0, 100};
        g_task_info[*task_id] = *create_info;
    }

    {
        unique_lock<shared_mutex> lock(g_task_mutex); // 写操作使用独占锁
        g_pending_tasks.push(*task_id);
    }

    return NNG_DL_ERROR_SUCCESS;
}

// 创建批量任务
int32_t NNGDownloader::create_batch_task(const nng_dl_create_batch_info* create_info, uint64_t* task_id) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    if (create_info == nullptr || task_id == nullptr || create_info->batch_files == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    {
        unique_lock<shared_mutex> lock(g_task_mutex); // 写操作使用独占锁

        for (uint32_t i = 0; i < create_info->batch_files->file_count; ++i) {
            const nng_dl_file_item* file_item = &create_info->batch_files->file_list[i];
            for (const auto& pair : g_task_states) {
                auto it = g_task_info.find(pair.first);
                if (it != g_task_info.end() && it->second.save_name == file_item->save_name) {
                    if (pair.second.state_code != NNG_DL_TASK_STATUS_FAILED) {
                        return NNG_DL_ERROR_TASK_ALREADY_EXIST;
                    }
                }
            }
        }

        for (uint32_t i = 0; i < create_info->batch_files->file_count; ++i) {
            const nng_dl_file_item* file_item = &create_info->batch_files->file_list[i];
            uint64_t file_task_id = g_next_task_id++;
            g_task_states[file_task_id] = {10*1024, 0, 0, NNG_DL_TASK_STATUS_START_PENDING, 0, 0, 100};
            g_task_info[file_task_id] = *file_item;
        }
    }

    {
        unique_lock<shared_mutex> lock(g_task_mutex); // 写操作使用独占锁
        for (uint32_t i = 0; i < create_info->batch_files->file_count; ++i) {
            uint64_t sub_task_id = g_next_task_id - create_info->batch_files->file_count + i;
            g_pending_tasks.push(sub_task_id);
        }
    }

    return NNG_DL_ERROR_SUCCESS;
}

// 设置任务令牌
int32_t NNGDownloader::set_task_token(uint64_t task_id, const char* task_token) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    if (task_token == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    unique_lock<shared_mutex> lock(g_task_mutex); // 写操作使用独占锁

    auto it = g_task_states.find(task_id);
    if (it == g_task_states.end()) {
        return NNG_DL_ERROR_TASK_NOT_EXIST;
    }

    return NNG_DL_ERROR_SUCCESS;
}

// 响应头处理函数
size_t NNGDownloader::HeaderCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    HeaderInfo* info = static_cast<HeaderInfo*>(userdata);
    string header(reinterpret_cast<const char*>(ptr), size * nmemb);

    if (header.size() >= 2 && header[header.size()-2] == '\r' && header[header.size()-1] == '\n') {
        header = header.substr(0, header.size()-2);
    }
    
    if (header.empty()) {
        return size * nmemb;
    }
    
    size_t colonPos = header.find(':');
    if (colonPos != string::npos) {
        string fieldName = header.substr(0, colonPos);
        string fieldValue = header.substr(colonPos + 1);
        
        fieldName.erase(0, fieldName.find_first_not_of(" \t"));
        fieldName.erase(fieldName.find_last_not_of(" \t") + 1);
        fieldValue.erase(0, fieldValue.find_first_not_of(" \t"));
        fieldValue.erase(fieldValue.find_last_not_of(" \t") + 1);
        
        string fieldNameLower = fieldName;
        transform(fieldNameLower.begin(), fieldNameLower.end(), fieldNameLower.begin(), ::tolower);
        
        if (fieldNameLower == "content-length") {
            try {
                info->file_size = stoull(fieldValue);
            } catch (const std::exception& e) {
                cerr << "Error: Unable to convert Content-Length value to integer" << endl;
            }
        }
        
        else if (fieldNameLower == "x-file-md5") {
            info->x_file_md5 = fieldValue;
        }
    }

    return size * nmemb;
}

// 下载文件函数 - 主函数，负责决定是否分片下载
void NNGDownloader::downloadFile(const nng_dl_file_item& file, uint64_t task_id) {
    uint64_t file_size = 0;
    string file_hash;
    {
        CURL* curl = curl_easy_init();
        if (!curl) {
            cerr << "Error: 无法初始化CURL获取文件大小" << endl;
            return;
        }
        string url = "http://192.168.88.188:8848/download_endpoint?filename=" + file.save_name;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); 
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L); 

        HeaderInfo headerInfo;
        headerInfo.file_size = 0; 
        headerInfo.x_file_md5 = ""; 
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerInfo);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "Error: 无法获取文件大小: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            return;
        }

        curl_easy_cleanup(curl);

        file_size = headerInfo.file_size;
        file_hash = headerInfo.x_file_md5;

        if (file_size == 0) {
            cerr << "Error: 获取到的文件大小为0" << endl;
            return;
        }
    }

    constexpr uint64_t chunk_size_threshold = 512 * 1024; 
    constexpr uint64_t chunk_size = 512 * 1024; 
    
    uint64_t num_chunks = (file_size + chunk_size - 1) / chunk_size;
    if (num_chunks == 0) num_chunks = 1; 
    
    // 检查是否为断点续传，并且所有分片已完成但尚未合并
    vector<uint64_t> chunk_task_ids;
    bool all_chunks_exist = true;
    bool all_chunks_done = true;
    bool file_needs_merge = false;
    
    {
        shared_lock<shared_mutex> lock(g_task_mutex);
        auto it = g_task_info.find(task_id);
        if (it != g_task_info.end() && !it->second.chunk_task_ids.empty()) {
            chunk_task_ids = it->second.chunk_task_ids;
            
            // 检查所有分片是否都存在并已完成
            for (uint64_t chunk_id : chunk_task_ids) {
                auto chunk_state_it = g_task_states.find(chunk_id);
                if (chunk_state_it == g_task_states.end()) {
                    all_chunks_exist = false;
                    break;
                }
                
                if (chunk_state_it->second.state_code != NNG_DL_TASK_STATUS_SUCCEEDED) {
                    all_chunks_done = false;
                    break;
                }
            }
            
            // 检查原文件是否需要合并
            string original_filepath = (filesystem::path(file.save_path) / file.save_name).string();
            if (!filesystem::exists(original_filepath) || 
                (file_hash != "" && calculateMD5(original_filepath) != file_hash)) {
                file_needs_merge = true;
            }
        } else {
            all_chunks_exist = false;
        }
    }
    
    // 如果所有分片都存在且已完成，无论原文件状态如何，都确保执行合并
    if (all_chunks_exist && all_chunks_done && chunk_task_ids.size() > 1) {
        cout << "检测到所有分片已下载完成，准备合并..." << endl;
        mergeChunks(file, chunk_task_ids, file_hash);
        
        {
            unique_lock<shared_mutex> lock(g_task_mutex);
            // 更新主任务状态
            auto it = g_task_states.find(task_id);
            if (it != g_task_states.end()) {
                it->second.state_code = NNG_DL_TASK_STATUS_SUCCEEDED;
                it->second.downloaded_size = file_size;
                it->second.total_size = file_size;
            }
            
            // 清理分片任务信息
            for (uint64_t chunk_task_id : chunk_task_ids) {
                g_task_states.erase(chunk_task_id);
                g_task_info.erase(chunk_task_id);
            }
        }
        
        return;  // 直接返回，不需要重新下载
    }
    
    // 如果不是所有分片都存在或已完成，则创建或继续下载分片
    if (!all_chunks_exist) {
        chunk_task_ids.clear();
        
        // 创建分片任务的代码（原有代码）
        unique_lock<shared_mutex> lock(g_task_mutex); 
        
        for (uint64_t i = 0; i < num_chunks; ++i) {
            nng_dl_file_item chunk_file;
            
            if (num_chunks == 1) {
                chunk_file.save_name = file.save_name;
            } else {
                chunk_file.save_name = file.save_name + ".chunk" + to_string(i);
            }
            
            chunk_file.save_path = file.save_path;
            chunk_file.url = file.url;
            chunk_file.hash = ""; 
            chunk_file.file_size = (i == num_chunks - 1) ? 
                                   (file_size - i * chunk_size) : chunk_size;
            chunk_file.chunk_count = 0; 
            chunk_file.chunk_task_ids.clear(); 
            chunk_file.finish_chunk = 0; // 初始化已下载完成分片数量
            
            uint64_t chunk_task_id = g_next_task_id++;
            g_task_states[chunk_task_id] = {
                chunk_file.file_size, 
                0, 
                0, 
                NNG_DL_TASK_STATUS_START_PENDING, 
                0, 
                0, 
                100
            };
            g_task_info[chunk_task_id] = chunk_file;
            chunk_task_ids.push_back(chunk_task_id);
        }
        
        auto it = g_task_info.find(task_id);
        if (it != g_task_info.end()) {
            it->second.chunk_count = num_chunks;
            it->second.chunk_task_ids = chunk_task_ids;
            it->second.finish_chunk = 0; // 初始化已下载完成分片数量
        }
    }
    
    for (size_t i = 0; i < chunk_task_ids.size(); ++i) {
        uint64_t chunk_task_id = chunk_task_ids[i];
        nng_dl_file_item chunk_file;
        
        {
            shared_lock<shared_mutex> lock(g_task_mutex); 
            auto it = g_task_info.find(chunk_task_id);
            if (it == g_task_info.end()) {
                cerr << "Error: 找不到分片任务信息" << endl;
                continue;
            }
            chunk_file = it->second;
        }
        
        uint64_t start_byte = i * chunk_size;
        uint64_t end_byte = min((i + 1) * chunk_size - 1, file_size - 1);
        
        g_coroutine_pool->submitTask([chunk_task_id, chunk_file, start_byte, end_byte, file_size] {
            downloadFileInChunks(chunk_file, chunk_task_id, start_byte, end_byte, file_size);
        });
    }
    
     
    // 等待所有分片完成
    while (true) {
        bool all_chunks_done = true;
        {
            shared_lock<shared_mutex> lock(g_task_mutex); 
            for (uint64_t chunk_task_id : chunk_task_ids) {
                auto it = g_task_states.find(chunk_task_id);
                if (it == g_task_states.end() || it->second.state_code != NNG_DL_TASK_STATUS_SUCCEEDED) {
                    all_chunks_done = false;
                    break;
                }
            }
        }
        if (all_chunks_done) {
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    
    // 所有分片已完成，无需再检查父任务状态，直接判断是否需要合并
    if (num_chunks > 1) {
        // 总是执行合并操作，确保文件被正确合并
        mergeChunks(file, chunk_task_ids, file_hash);
        
        {
            unique_lock<shared_mutex> lock(g_task_mutex); 
            // 清理分片任务信息
            for (uint64_t chunk_task_id : chunk_task_ids) {
                g_task_states.erase(chunk_task_id);
                g_task_info.erase(chunk_task_id);
            }
        }
    }else {
        if (!file_hash.empty()) {
            string filepath = (filesystem::path(file.save_path) / file.save_name).string();
            string localMD5 = calculateMD5(filepath);
            if (!localMD5.empty() && localMD5 != file_hash) {
                cerr << "Error: MD5校验失败，文件可能损坏" << endl;
                
                unique_lock<shared_mutex> lock(g_task_mutex); 
                auto it = g_task_states.find(task_id);
                if (it != g_task_states.end()) {
                    it->second.state_code = NNG_DL_TASK_STATUS_FAILED;
                }
                return;
            }
        }
    }
    
    {
        unique_lock<shared_mutex> lock(g_task_mutex); 
        auto it = g_task_states.find(task_id);
        if (it != g_task_states.end()) {
            it->second.state_code = NNG_DL_TASK_STATUS_SUCCEEDED;
            it->second.downloaded_size = file_size;
            it->second.total_size = file_size;
        }
    }
}

// 下载文件分片
void NNGDownloader::downloadFileInChunks(const nng_dl_file_item& chunk_file, uint64_t task_id, uint64_t start_byte, uint64_t end_byte, uint64_t file_size) {
    string original_filename = chunk_file.save_name.substr(0, chunk_file.save_name.find(".chunk"));

    string sPath = chunk_file.save_path;
    filesystem::path dirPath(u8string(sPath.begin(), sPath.end()));
    filesystem::path filepath = dirPath / chunk_file.save_name;

    uint64_t downloaded_size = 0;
    if (filesystem::exists(filepath)) {
        downloaded_size = filesystem::file_size(filepath);
    }

     // 文件已经下载完成
     if (downloaded_size >= end_byte - start_byte + 1) {
        unique_lock<shared_mutex> lock(g_task_mutex); 
        auto it = g_task_states.find(task_id);
        if (it != g_task_states.end()) {
            it->second.state_code = NNG_DL_TASK_STATUS_SUCCEEDED;
            it->second.downloaded_size = downloaded_size;
            it->second.total_size = end_byte - start_byte + 1;  // 确保设置正确的总大小
        }
        
        // 更新父任务的已下载分片数量
        auto parent_it = g_task_info.find(task_id);
        if (parent_it != g_task_info.end()) {
            parent_it->second.finish_chunk++;
        }
        return;
    }

    try {
        if (!filesystem::exists(dirPath)) {
            filesystem::create_directories(dirPath);
        }
    } catch (const std::exception& e) {
        cerr << "Error: 无法创建目录: " << e.what() << endl;
        return;
    }

    FILE* fp = nullptr;
    if (downloaded_size > 0) {
        fp = fopen(filepath.string().c_str(), "ab");  
    } else {
        fp = fopen(filepath.string().c_str(), "wb");  
    }
    
    if (!fp) {
        cerr << "Error: 无法打开文件 " << filepath << " 进行写入" << endl;
        return;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        cerr << "Error: 无法初始化CURL" << endl;
        fclose(fp);
        return;
    }

    string url = chunk_file.url.empty() ? 
                "http://192.168.88.188:8848/download_endpoint?filename=" + original_filename : 
                chunk_file.url;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);  
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);        

    struct curl_slist* headers = nullptr;
    uint64_t request_start = start_byte + downloaded_size;
    if (request_start <= end_byte) {
        string range_header = "Range: bytes=" + to_string(request_start) + "-" + to_string(end_byte);
        
        headers = curl_slist_append(headers, range_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    HeaderInfo headerInfo;
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerInfo);

    curl_off_t download_speed_limit = static_cast<curl_off_t>(g_download_speed_limit) * 1024;
    curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, download_speed_limit);

    uint64_t* task_id_ptr = new uint64_t(task_id);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, task_id_ptr);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L); 
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);       

    CURLcode res = curl_easy_perform(curl);
    
    fclose(fp);
    delete task_id_ptr;

    
    if (headers) {
        curl_slist_free_all(headers);
    }
    
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        cerr << "下载分片失败: " << curl_easy_strerror(res) << endl;
        
        unique_lock<shared_mutex> lock(g_task_mutex); 
        auto it = g_task_states.find(task_id);
        if (it != g_task_states.end()) {
            it->second.state_code = NNG_DL_TASK_STATUS_FAILED;
        }
        return;
    }

    uint64_t expected_size = end_byte - start_byte + 1;
    uint64_t current_size = 0;
    if (filesystem::exists(filepath)) {
        current_size = filesystem::file_size(filepath);
    }

    if (current_size < expected_size) {
        cerr << "分片下载不完整: 预期" << expected_size << "字节，实际" << current_size << "字节" << endl;
        
        unique_lock<shared_mutex> lock(g_task_mutex); 
        auto it = g_task_states.find(task_id);
        if (it != g_task_states.end()) {
            it->second.state_code = NNG_DL_TASK_STATUS_FAILED;
        }
        return;
    }

    {
        unique_lock<shared_mutex> lock(g_task_mutex); 
        auto it = g_task_states.find(task_id);
        if (it != g_task_states.end()) {
            it->second.state_code = NNG_DL_TASK_STATUS_SUCCEEDED;
            it->second.downloaded_size = current_size;
        }
        
        // 更新父任务的已下载分片数量
        auto parent_it = g_task_info.find(task_id);
        if (parent_it != g_task_info.end()) {
            parent_it->second.finish_chunk++;
        }
    }
}

// 合并分片函数
void NNGDownloader::mergeChunks(const nng_dl_file_item& original_file, const vector<uint64_t>& chunk_task_ids, const string& server_hash) {
    string original_filename = original_file.save_name;
    string sPath = original_file.save_path;
    filesystem::path dirPath(u8string(sPath.begin(), sPath.end()));
    filesystem::path original_filepath = dirPath / original_filename;

    ofstream original_file_stream(original_filepath, ios::binary);
    if (!original_file_stream) {
        cerr << "Error: Unable to open file " << original_filepath << " for writing" << endl;
        return;
    }

    for (uint64_t chunk_task_id : chunk_task_ids) {
        nng_dl_file_item chunk_file;
        {
            shared_lock<shared_mutex> lock(g_task_mutex); 
            auto it = g_task_info.find(chunk_task_id);
            if (it != g_task_info.end()) {
                chunk_file = it->second;
            }
        }

        string chunk_filename = chunk_file.save_name;
        filesystem::path chunk_filepath = dirPath / chunk_filename;

        ifstream chunk_file_stream(chunk_filepath, ios::binary);
        if (chunk_file_stream) {
            original_file_stream << chunk_file_stream.rdbuf();
            chunk_file_stream.close();
            filesystem::remove(chunk_filepath);
        }
    }

    original_file_stream.close();

    string localMD5 = calculateMD5(original_filepath.string());
    if (!localMD5.empty()) {
        if (!server_hash.empty() && localMD5 != server_hash) {
            cerr << "Error: Download file " << original_filename << " failed! MD5 checksum mismatch" << endl;
            filesystem::remove(original_filepath);
        } else {
            cout << "\nFile downloaded successfully, The file saving path is " << original_filepath << endl;
        }
    } else {
        cerr << "Error: Download file " << original_filename << " failed! MD5 checksum mismatch" << endl;
        filesystem::remove(original_filepath);
    }
}

// 开始任务
int32_t NNGDownloader::execute_task(uint64_t task_id) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    nng_dl_file_item task_info;
    {
        unique_lock<shared_mutex> lock(g_task_mutex); 
        auto it = g_task_states.find(task_id);
        if (it == g_task_states.end()) {
            return NNG_DL_ERROR_TASK_NOT_EXIST;
        }

        if (it->second.state_code == NNG_DL_TASK_STATUS_STARTED || it->second.state_code == NNG_DL_TASK_STATUS_PAUSED) {
            return NNG_DL_ERROR_TASK_ALREADY_RUNNING;
        }

        it->second.state_code = NNG_DL_TASK_STATUS_STARTED;
        g_concurrent_task_count++;
        
        auto file_it = g_task_info.find(task_id);
        if (file_it != g_task_info.end()) {
            task_info = file_it->second;
        } else {
            it->second.state_code = NNG_DL_TASK_STATUS_FAILED;
            g_concurrent_task_count--;
            return NNG_DL_ERROR_TASK_NOT_EXIST;
        }
    }

    g_coroutine_pool->submitTask([task_id, task_info] {
        downloadFile(task_info, task_id);

        g_concurrent_task_count--;
    });

    return NNG_DL_ERROR_SUCCESS;
}

//暂停任务
int32_t NNGDownloader::pause_task(uint64_t task_id) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    unique_lock<shared_mutex> lock(g_task_mutex); 

    auto it = g_task_states.find(task_id);
    if (it == g_task_states.end()) {
        return NNG_DL_ERROR_TASK_NOT_EXIST;
    }

    if (it->second.state_code != NNG_DL_TASK_STATUS_STARTED) {
        return NNG_DL_ERROR_TASK_NOT_RUNNING;
    }

    it->second.state_code = NNG_DL_TASK_STATUS_PAUSED;

    auto it_task_info = g_task_info.find(task_id);
    if (it_task_info != g_task_info.end() && !it_task_info->second.chunk_task_ids.empty()) {
        for (uint64_t chunk_task_id : it_task_info->second.chunk_task_ids) {
            auto chunk_it = g_task_states.find(chunk_task_id);
            if (chunk_it != g_task_states.end() && chunk_it->second.state_code == NNG_DL_TASK_STATUS_STARTED) {
                chunk_it->second.state_code = NNG_DL_TASK_STATUS_PAUSED;

                nng_dl_file_item chunk_task_info = g_task_info[chunk_task_id];
                filesystem::path chunk_filepath = filesystem::path(chunk_task_info.save_path) / chunk_task_info.save_name;
                if (filesystem::exists(chunk_filepath)) {
                    chunk_it->second.downloaded_size = filesystem::file_size(chunk_filepath);
                }
            }
        }
    }

    nng_dl_file_item task_info = g_task_info[task_id];
    filesystem::path filepath = filesystem::path(task_info.save_path) / task_info.save_name;
    if (filesystem::exists(filepath)) {
        it->second.downloaded_size = filesystem::file_size(filepath);
    }

    return NNG_DL_ERROR_SUCCESS;
}

// 停止任务
int32_t NNGDownloader::stop_task(uint64_t task_id) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    unique_lock<shared_mutex> lock(g_task_mutex); 

    auto it = g_task_states.find(task_id);
    if (it == g_task_states.end()) {
        return NNG_DL_ERROR_TASK_NOT_EXIST;
    }

    if (it->second.state_code == NNG_DL_TASK_STATUS_STOPED) {
        return NNG_DL_ERROR_TASK_ALREADY_STOPPED;
    }

    it->second.state_code = NNG_DL_TASK_STATUS_STOPED;

    return NNG_DL_ERROR_SUCCESS;
}

int32_t NNGDownloader::delete_task(uint64_t task_id, uint8_t delete_file_flag) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    unique_lock<shared_mutex> lock(g_task_mutex); 

    auto it = g_task_states.find(task_id);
    if (it == g_task_states.end()) {
        return NNG_DL_ERROR_TASK_NOT_EXIST;
    }

    auto it_task_info = g_task_info.find(task_id);
    if (it_task_info != g_task_info.end() && !it_task_info->second.chunk_task_ids.empty()) {
        for (uint64_t chunk_task_id : it_task_info->second.chunk_task_ids) {
            if (delete_file_flag) {
                auto chunk_it_task_info = g_task_info.find(chunk_task_id);
                if (chunk_it_task_info != g_task_info.end()) {
                    nng_dl_file_item chunk_task_info = chunk_it_task_info->second;
                    string chunk_save_path = string(chunk_task_info.save_path);
                    string chunk_save_name = string(chunk_task_info.save_name);
                    filesystem::path chunk_full_path(u8string(chunk_save_path.begin(), chunk_save_path.end()));
                    chunk_full_path /= filesystem::path(u8string(chunk_save_name.begin(), chunk_save_name.end()));
                    filesystem::remove(chunk_full_path);
                }
            }

            g_task_states.erase(chunk_task_id);
            g_task_info.erase(chunk_task_id);
        }
    }

    if (delete_file_flag) {
        nng_dl_file_item task_info = g_task_info[task_id];
        string t_save_path = string(task_info.save_path);
        string t_save_name = string(task_info.save_name);
        filesystem::path full_path(u8string(t_save_path.begin(), t_save_path.end()));
        full_path /= filesystem::path(u8string(t_save_name.begin(), t_save_name.end()));
        filesystem::remove(full_path);
    }

    g_task_states.erase(it);
    g_task_info.erase(task_id);

    return NNG_DL_ERROR_SUCCESS;
}

// 获取任务状态
int32_t NNGDownloader::get_task_state(uint64_t task_id, nng_dl_task_state* state) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    if (state == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    shared_lock<shared_mutex> lock(g_task_mutex); 

    auto it = g_task_states.find(task_id);
    if (it == g_task_states.end()) {
        return NNG_DL_ERROR_TASK_NOT_EXIST;
    }

    *state = it->second;
    return NNG_DL_ERROR_SUCCESS;
}

// 获取任务信息
int32_t NNGDownloader::get_task_info(uint64_t task_id, const char* info_name, void* buff, uint32_t* buff_len) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    if (info_name == nullptr || buff == nullptr || buff_len == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    shared_lock<shared_mutex> lock(g_task_mutex); 

    auto it = g_task_info.find(task_id);
    if (it == g_task_info.end()) {
        return NNG_DL_ERROR_TASK_NOT_EXIST;
    }

    if (strcmp(info_name, "url") == 0) {
        const std::string& url = it->second.url;
        size_t len = url.size() + 1;
        if (*buff_len < len) {
            *buff_len = len;
            return NNG_DL_ERROR_SUCCESS;
        }
        strcpy(static_cast<char*>(buff), url.c_str());
    } else if (strcmp(info_name, "save_path") == 0) {
        const std::string& save_path = it->second.save_path;
        size_t len = save_path.size() + 1;
        if (*buff_len < len) {
            *buff_len = len;
            return NNG_DL_ERROR_SUCCESS;
        }
        strcpy(static_cast<char*>(buff), save_path.c_str());
    } else if (strcmp(info_name, "save_name") == 0) {
        const std::string& save_name = it->second.save_name;
        size_t len = save_name.size() + 1;
        if (*buff_len < len) {
            *buff_len = len;
            return NNG_DL_ERROR_SUCCESS;
        }
        strcpy(static_cast<char*>(buff), save_name.c_str());
    } else if (strcmp(info_name, "hash") == 0) {
        const std::string& hash = it->second.hash;
        size_t len = hash.size() + 1;
        if (*buff_len < len) {
            *buff_len = len;
            return NNG_DL_ERROR_SUCCESS;
        }
        strcpy(static_cast<char*>(buff), hash.c_str());
    } else if (strcmp(info_name, "file_size") == 0) {
        if (*buff_len < sizeof(uint64_t)) {
            *buff_len = sizeof(uint64_t);
            return NNG_DL_ERROR_SUCCESS;
        }
        *((uint64_t*)buff) = it->second.file_size;
    } else if (strcmp(info_name, "chunk_count") == 0) {
        if (*buff_len < sizeof(uint32_t)) {
            *buff_len = sizeof(uint32_t);
            return NNG_DL_ERROR_SUCCESS;
        }
        *((uint32_t*)buff) = it->second.chunk_count;
    } else if (strcmp(info_name, "chunk_task_ids") == 0) {
        if (*buff_len < sizeof(vector<uint64_t>)) {
            *buff_len = sizeof(vector<uint64_t>);
            return NNG_DL_ERROR_SUCCESS;
        }
        vector<uint64_t>* chunk_ids = static_cast<vector<uint64_t>*>(buff);
        *chunk_ids = it->second.chunk_task_ids;
    } else if (strcmp(info_name, "finish_chunk") == 0) {
        if (*buff_len < sizeof(uint32_t)) {
            *buff_len = sizeof(uint32_t);
            return NNG_DL_ERROR_SUCCESS;
        }
        *((uint32_t*)buff) = it->second.finish_chunk;
    } else {
        return NNG_DL_ERROR_INFO_NAME_NOT_SUPPORT;
    }

    return NNG_DL_ERROR_SUCCESS;
}

// 设置并发任务数
int32_t NNGDownloader::set_concurrent_task_count(uint32_t count) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    g_max_concurrent_task_count = count;
    return NNG_DL_ERROR_SUCCESS;
}

// 设置下载速度限制
int32_t NNGDownloader::set_download_speed_limit(uint32_t speed) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    g_download_speed_limit = speed;
    return NNG_DL_ERROR_SUCCESS;
}

// 设置上传开关
int32_t NNGDownloader::set_upload_switch(uint32_t upload_switch) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    g_upload_switch = upload_switch != 0;
    return NNG_DL_ERROR_SUCCESS;
}

// 设置上传速度限制
int32_t NNGDownloader::set_upload_speed_limit(uint32_t speed) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    g_upload_speed_limit = speed;
    return NNG_DL_ERROR_SUCCESS;
}

// 获取版本
int32_t NNGDownloader::version(char* buff, uint32_t* buff_len) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    if (buff == nullptr || buff_len == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    const char* version = "1.0.0";
    size_t len = strlen(version) + 1;
    if (*buff_len < len) {
        *buff_len = len;
        return NNG_DL_ERROR_SUCCESS;
    }
    strcpy(buff, version);
    return NNG_DL_ERROR_SUCCESS;
}

// 设置任务优先级
int32_t NNGDownloader::set_task_priority(uint64_t task_id, uint32_t priority) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    unique_lock<shared_mutex> lock(g_task_mutex); 

    auto it = g_task_states.find(task_id);
    if (it == g_task_states.end()) {
        return NNG_DL_ERROR_TASK_NOT_EXIST;
    }

    it->second.priority = priority;

    auto it_task_info = g_task_info.find(task_id);
    if (it_task_info != g_task_info.end() && !it_task_info->second.chunk_task_ids.empty()) {
        for (uint64_t chunk_task_id : it_task_info->second.chunk_task_ids) {
            auto chunk_it = g_task_states.find(chunk_task_id);
            if (chunk_it != g_task_states.end()) {
                chunk_it->second.priority = priority;
            }
        }
    }

    return NNG_DL_ERROR_SUCCESS;
}

// 获取任务优先级
int32_t NNGDownloader::get_task_priority(uint64_t task_id, uint32_t* priority) {
    if (!g_sdk_initialized) {
        return NNG_DL_ERROR_SDK_NOT_INIT;
    }

    if (priority == nullptr) {
        return NNG_DL_ERROR_PARAM_ERROR;
    }

    shared_lock<shared_mutex> lock(g_task_mutex); 

    auto it = g_task_states.find(task_id);
    if (it == g_task_states.end()) {
        return NNG_DL_ERROR_TASK_NOT_EXIST;
    }

    *priority = it->second.priority;
    return NNG_DL_ERROR_SUCCESS;
}

double NNGDownloader::getOriginalFileProgress(uint64_t task_id) {
    shared_lock<shared_mutex> lock(g_task_mutex); 
    auto it = g_task_states.find(task_id);
    if (it == g_task_states.end()) {
        return 0.0;
    }

    uint64_t total_size = it->second.total_size;
    uint64_t downloaded_size = it->second.downloaded_size;

    if (downloaded_size > total_size) {
        downloaded_size = total_size;
    }

    if (total_size == 0) {
        return 0.0;
    }
    return (static_cast<double>(downloaded_size) / total_size) * 100.0;
}

// 在任务调度时根据优先级进行排序并启动任务
void NNGDownloader::schedule_and_start_tasks() {
    vector<uint64_t> pending_tasks;
    {
        unique_lock<shared_mutex> lock(g_task_mutex); // 使用独占锁（写锁）
        while (!g_pending_tasks.empty()) {
            pending_tasks.push_back(g_pending_tasks.front());
            g_pending_tasks.pop();
        }
    }

    sort(pending_tasks.begin(), pending_tasks.end(), [](uint64_t a, uint64_t b) {
        shared_lock<shared_mutex> lock(g_task_mutex); // 使用共享锁（读锁）
        return g_task_states[a].priority < g_task_states[b].priority;
    });

    for (auto p_task : pending_tasks) {
        if (g_concurrent_task_count >= g_max_concurrent_task_count) {
            unique_lock<shared_mutex> lock(g_task_mutex); // 使用独占锁（写锁）
            g_pending_tasks.push(p_task);
            continue;
        }

        int32_t start_ret = execute_task(p_task);
        if (start_ret != NNG_DL_ERROR_SUCCESS) {
            cerr << "Start task id = " << p_task << " failed: " << start_ret << endl;
            unique_lock<shared_mutex> lock(g_task_mutex); // 使用独占锁（写锁）
            g_pending_tasks.push(p_task);
        }
    }
}
