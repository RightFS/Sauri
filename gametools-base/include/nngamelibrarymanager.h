//
// Created by Right on 25/3/13 星期四 17:21.
//

#pragma once
#ifndef NNGAME_NNGAMELIBRARYMANAGER_H
#define NNGAME_NNGAMELIBRARYMANAGER_H
#include "nngamedef.h"

#ifdef __cplusplus
extern "C" {
#endif

NNGAME_API int NNGame_LibraryGetApps(int64_t userid, int64_t* appids, int64_t* appids_size);

NNGAME_API int NNGame_LibraryGetAppInfo(int64_t appid, char* appname, int64_t appname_size, char* appicon, int64_t appicon_size);

NNGAME_API int NNGame_LibraryInstallApp(int64_t appid);

NNGAME_API int NNGame_LibraryUninstallApp(int64_t appid);

//launch
NNGAME_API int NNGame_LibraryLaunchApp(int64_t appid);

//kill
NNGAME_API int NNGame_LibraryKillApp(int64_t appid);

#ifdef __cplusplus
}
#endif
#endif //NNGAME_NNGAMELIBRARYMANAGER_H
