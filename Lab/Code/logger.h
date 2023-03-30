#ifndef _LOGGER_H
#define _LOGGER_H
/**
 * This is a logger header file. To use it, simply include and remember to define #LOG_ON
 */
#include <assert.h>
#include <stdio.h>
// #define LOG_ON
#ifdef LOG_ON
#define Log(format, ...) printf("\33[1;36m[%s,%d,%s] " format "\33[0m\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define Panic(format, ...)                                                                             \
    do {                                                                                               \
        printf("\33[1;31m[%s,%d,%s] " format "\33[0m\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        assert(0);                                                                                     \
    } while (0)

#define Warn(format, ...)                                                                              \
    do {                                                                                               \
        printf("\33[1;31m[%s,%d,%s] " format "\33[0m\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    } while (0)

#define Panic_on(cond, format, ...)                                                                        \
    do {                                                                                                   \
        if (cond) {                                                                                        \
            printf("\33[1;31m[%s,%d,%s] " format "\33[0m\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
            assert(0);                                                                                     \
        }                                                                                                  \
    } while (0)

#else
#define Log(...)
#define Panic(...)
#define Warn(...)
#define Panic_on(...)
#endif

/* for debug:: insert breakponit */
void breakpoint();

#endif