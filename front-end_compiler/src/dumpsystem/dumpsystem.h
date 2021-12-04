#ifndef DUMP_SYSTEM_H
#define DUMP_SYSTEM_H

#include <stdio.h>
#include <string.h>

const int DUMPSYSTEM_DEFAULT_STREAM = 1;

#define ASSERT$(CONDITION__, ERROR__, ACTION__)                                       \
    do                                                                                \
    {                                                                                 \
        if(!(CONDITION__))                                                            \
        {                                                                             \
            fprintf(stderr, "ERROR: %s\n", #ERROR__);                                 \
                                                                                      \
            FILE* stream__ = dumpsystem_get_stream_(DUMPSYSTEM_DEFAULT_STREAM);       \
            if(stream__ != nullptr)                                                   \
            {                                                                         \
                fprintf(stream__, "<span class = \"error\"> ERROR: %s\n"              \
                                "at %s:%d:%s </span>\n",                              \
                         #ERROR__, __FILE__, __LINE__, __PRETTY_FUNCTION__);          \
                fflush(stream__);                                                     \
            }                                                                         \
                                                                                      \
            {ACTION__}                                                                \
        }                                                                             \
    } while(0)                                                                        \

#define ASSERT_RET$(CONDITION__, ERROR__)             \
    ASSERT$(CONDITION__, ERROR__, return ERROR__; )   \

#define PASS$(CONDITION__, ACTION__)                                                  \
    do                                                                                \
    {                                                                                 \
        if(!(CONDITION__))                                                            \
        {                                                                             \
            FILE* stream__ = dumpsystem_get_stream_(DUMPSYSTEM_DEFAULT_STREAM);       \
            if(stream__ != nullptr)                                                   \
            {                                                                         \
                fprintf(stream__, "<span class = \"error\"> at %s:%d:%s </span>\n",   \
                        __FILE__, __LINE__, __PRETTY_FUNCTION__);                     \
                fflush(stream__);                                                     \
            }                                                                         \
                                                                                      \
            {ACTION__}                                                                \
        }                                                                             \
    } while(0)                                                                        \

#define LOG$(MESSAGE__, ...)                                                          \
    {                                                                                 \
        FILE* stream__ = dumpsystem_get_stream_(DUMPSYSTEM_DEFAULT_STREAM);           \
        if(stream__ != nullptr)                                                       \
        {                                                                             \
            fprintf(stream__, "<span class = \"title\"> LOG: \"" MESSAGE__ "\"\n"     \
                            "at %s:%d:%s</span>\n",                                   \
                    ##__VA_ARGS__, __FILE__, __LINE__, __PRETTY_FUNCTION__);          \
            fflush(stream__);                                                         \
        }                                                                             \
    }                                                                                 \

#define SET_FILE(DESCRNAME, filename)   \
    DESCR_##DESCRNAME,                  \

enum dumpsystem_descriptors_
{
    DESCR_stderr = 0,

    #include "dumpsystem_files.inc"

    DESCR_END
};
#undef SET_FILE

#define dumpsystem_get_stream(DESCRNAME) dumpsystem_get_stream_(DESCR_##DESCRNAME)

FILE* dumpsystem_get_stream_(int descriptor);

#endif // DUMP_SYSTEM_H
