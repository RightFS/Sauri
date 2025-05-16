//
// Created by Administrator on 2025/4/16.
//

#ifndef NNGAME_ERROR_H
#define NNGAME_ERROR_H

enum LNGResult {
    LNG_ERR_UNKNOW = -1,  ///< 未知错误, 值为-1
    LNG_SUCC = 0,         ///< 接口调用成功, 值为0

    // library error
    LNG_ERR_LIBRARY_NOT_FOUND = 10001,     ///< 库未找到, 值为10001
    LNG_ERR_LIBRARY_INIT,                  ///< 库初始化失败, 值为10002
    LNG_ERR_LIBRARY_ALREADY_INIT,          ///< 库已初始化, 值为10003
    LNG_ERR_LIBRARY_NOT_INIT,              ///< 库未初始化, 值为10004
    LNG_ERR_LIBRARY_INVALID_PARAM_SCRIPT,  ///< 库参数无效, 值为10005
    LNG_ERR_LIBRARY_INVALID_PARAM_APPID,   ///< 库参数无效, 值为10006

    // database error
    LNG_ERR_DB_SCRIPT_NOT_EXITS = 20001,  ///< 数据库脚本不存在, 值为20000
    LNG_ERR_DB_SCRIPT_ALREADY_EXITS,      ///< 数据库脚本已存在, 值为20001
    LNG_ERR_DB_SCRIPT_INSERT_FAIL,        ///< 数据库脚本插入失败, 值为20002
    LNG_ERR_DB_METADATA_NOT_EXITS,        ///< 数据库元数据不存在, 值为20003

    // script error
    LNG_ERR_SCRIPT_ENGINE_INIT = 30001,  ///< 脚本引擎初始化失败, 值为20001

    // launch error
    LNG_ERR_LUNCH_PATH_EMPTY = 40001,  ///< 启动路径为空, 值为40001
    LNG_ERR_LUNCH_NOT_RUNING,          ///< 未运行, 值为40002

    // executor error
    LNG_ERR_SCRIPT_TERMINATED = 50001,  ///< 脚本终止, 值为50001

};

#endif  // NNGAME_ERROR_H
