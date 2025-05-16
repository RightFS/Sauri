//
// Created by Right on 25/3/25 星期二 14:42.
//
#include <gtest/gtest.h>
#include <fstream>
#include "../../src/downloader/downloader.h"

std::unordered_map<std::string, std::pair<std::string, std::string>> m_task_info; // 任务信息

std::vector<std::pair<std::string, std::string>> readConfigFile(const std::string& configFilename) {
    std::vector<std::pair<std::string, std::string>> tasks;
    std::ifstream configFile(configFilename);
    if (!configFile) {
        std::cerr << "Error: Unable to open config file" << std::endl;
        return tasks;
    }

    std::string line;
    while (getline(configFile, line)) {
        line.erase(remove(line.begin(), line.end(), '\r'), line.end());
        line.erase(remove(line.begin(), line.end(), '\n'), line.end());
        if (line.substr(0, 4) == "get ") {
            std::istringstream iss(line.substr(4));
            std::string filename, save_path, url;
            if (iss >> filename >> save_path >> url) {
                tasks.emplace_back(filename, save_path);
                // 保存任务信息到全局 unordered_map
                m_task_info[filename] = make_pair(save_path, url);
            }
        }
    }

    configFile.close();
    return tasks;
}

TEST(DownloadTest, test1) {
    using namespace std;
    nng_dl_init_param param = {
            "app_id", "app_version", "cfg_path", 1
    };

    // 初始化
    int32_t ret = NNGDownloader::init(&param);
    ASSERT_EQ(ret,NNG_DL_ERROR_SUCCESS);

    vector<pair<string, string>> tasks = readConfigFile("config.txt");
    ASSERT_EQ(tasks.empty(),false);

    uint64_t task_id = 1;

    cout<<"测试创建服务器任务"<<endl;
    for (const auto& task : tasks) {
        if (!task.first.empty() && !task.second.empty()) {
            // 从全局 unordered_map 中获取任务信息
            auto it = m_task_info.find(task.first);
            if (it != m_task_info.end()) {
                string filename = task.first;
                string save_path = it->second.first;
                string url = it->second.second;

                nng_dl_file_item create_info = {
                        task.first.c_str(), // save_name
                        save_path.c_str(),  // save_path
                        url.c_str()         // url
                };
                ret = NNGDownloader::create_server_task(&create_info, &task_id);
                ASSERT_EQ(ret,NNG_DL_ERROR_SUCCESS);
            }
        }
    }

    //cout<<"测试创建批量任务\n";
    ////构造批量任务信息
    //nng_dl_files_info batch_files;
    //batch_files.file_count = tasks.size();
    //batch_files.file_list = new nng_dl_file_item[batch_files.file_count];

    //for (uint32_t i = 0; i < batch_files.file_count; ++i) {
    //    const auto& task = tasks[i];
    //    auto it = m_task_info.find(task.first);
    //    if (it != m_task_info.end()) {
    //        batch_files.file_list[i] = {
    //            task.first.c_str(),       // save_name
    //            it->second.first.c_str(), // save_path
    //            it->second.second.c_str() // url
    //        };
    //    }
    //}

    //nng_dl_create_batch_info create_batch_info = {
    //    "Batch Task", // task_name
    //    20,           // max_concurrent
    //    &batch_files  // batch_files
    //};

    //ret = NNGDownloader::create_batch_task(&create_batch_info, &task_id);
    //if (ret != NNG_DL_ERROR_SUCCESS) {
    //    cerr << "Error: Create batch task failed: " << ret << endl;
    //    delete[] batch_files.file_list;
    //    NNGDownloader::uninit();
    //    return -1;
    //}

    //delete[] batch_files.file_list;

    cout<<"1\n";
    NNGDownloader::set_download_speed_limit(1024*0.2);
    cout<<"2\n";
    NNGDownloader::schedule_and_start_tasks();
    cout<<"99\n";
    NNGDownloader::pause_task(3);
    // NNGDownloader::delete_task(3,0);
    NNGDownloader::schedule_and_start_tasks();

    // 等待任务完成
    this_thread::sleep_for(chrono::seconds(2));
    // 反初始化
    NNGDownloader::uninit();
}


// 测试夹具
class DownloadTest : public ::testing::Test {
protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
};


TEST_F(DownloadTest, test2) {
    EXPECT_EQ(1, 1);
}
