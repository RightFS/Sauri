//
// Created by Right on 25/3/13 星期四 11:35.
//

#pragma once
#ifndef NNGAME_NNGAMESDK_H
#define NNGAME_NNGAMESDK_H

#include "nngamedef.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * 接口通用回调的定义
 *
 * @param code 值为 LIM_SUCC 表示成功，其他值表示失败。详情请参考 [错误码](https://cloud.tencent.com/document/product/269/1671)
 * @param desc 错误描述字符串
 * @param json_params Json 字符串，不同的接口，Json 字符串不一样
 * @param user_data ImSDK 负责透传的用户自定义数据，未做任何处理
 *
 * @note
 * 所有回调均需判断 code 是否等于 LIM_SUCC，若不等于说明接口调用失败了，具体原因可以看 code 的值以及 desc 描述。详情请参考 [错误码](https://cloud.tencent.com/document/product/269/1671)
 */
typedef void (*NNGameCommCallback)(int32_t code, const char *desc, const char *json_params, const void *user_data);

NNGAME_API int NNGame_Init();
NNGAME_API int NNGame_Uninit();

#if __cplusplus
};
#endif

#endif //NNGAME_NNGAMESDK_H
