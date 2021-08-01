/**
 * @author      : wuzheng 
 * @file        : debug
 * @created     : Monday Mar 01, 2021 11:06:33 CST
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef DEBUG_HOOK
#include  <android/log.h>
#define IO_LOG_DETAIL(io, fmt, ...) fprintf(io, "[%-24s, %4d] : " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define IO_LOG(io, fmt, ...) fprintf(io, fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define LOG(fmt, ...)  __android_log_print(ANDROID_LOG_ERROR, "TTTTT", "[%-24s, %4d]" fmt, __func__, __LINE__, ##__VA_ARGS__)

#else
#define IO_LOG_DETAIL(io, fmt, ...)
#define IO_LOG(io, fmt, ...)
#define LOG(fmt, ...)

#endif
#endif /* end of include guard DEBUG_H */

