//
// Created by Right on 25/3/13 星期四 11:27.
//

#pragma once
#ifndef NNGAME_NNGAMEUSER_H
#define NNGAME_NNGAMEUSER_H

#include "nngamedef.h"

#ifdef __cplusplus
extern "C" {
#endif



NNGAME_API int NNGame_UserLogin(int64_t userid,const char* token);

NNGAME_API int NNGame_UserLogout(int64_t userid);

#ifdef __cplusplus
}
#endif

#endif //NNGAME_NNGAMEUSER_H
