#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define LOG_VERBOSE 2
#define LOG_DEBUG 3
#define LOG_INFO 4
#define LOG_WARN 5
#define LOG_ERROR 6
#define LOG_FATAL 7
#define LOG_PROFILER 8

void 
LogInit(void);

void 
LogMessage(int priority, const char *format, ...);

#define LOG(prio, ...) LogMessage(prio, __VA_ARGS__)

#define LOGI(...) LOG(LOG_INFO, __VA_ARGS__)
#define LOGW(...) LOG(LOG_WARN, __VA_ARGS__)
#define LOGD(...) LOG(LOG_DEBUG, __VA_ARGS__)
#define LOGV(...) LOG(LOG_VERBOSE, __VA_ARGS__)
#define LOGE(...) LOG(LOG_ERROR, __VA_ARGS__)
#define LOGF(...) LOG(LOG_FATAL, __VA_ARGS__)
#define LOGP(...) LOG(LOG_PROFILER, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif

#define DBG LOGD("Here! %s %d", __FILE__, __LINE__)
#ifdef __cplusplus
}
#endif
