#pragma once
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#ifdef ANDROID
#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "LorieNative"
#endif

#ifndef LOG
#define LOG(prio, ...) __android_log_print(prio, LOG_TAG, __VA_ARGS__)
#endif

#define LOGI(...) LOG(ANDROID_LOG_INFO, __VA_ARGS__)
#define LOGW(...) LOG(ANDROID_LOG_WARN, __VA_ARGS__)
#define LOGD(...) LOG(ANDROID_LOG_DEBUG, __VA_ARGS__)
#define LOGV(...) LOG(ANDROID_LOG_VERBOSE, __VA_ARGS__)
#define LOGE(...) LOG(ANDROID_LOG_ERROR, __VA_ARGS__)
#define LOGF(...) LOG(ANDROID_LOG_FATAL, __VA_ARGS__)

#endif
#pragma clang diagnostic pop