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
            fprintf(stderr, "\x1b[31;1mERROR:\x1b[0m %s\n", #ERROR__);                \
                                                                                      \
            FILE* stream__ = dumpsystem_get_opened_stream();                          \
            if(stream__ != nullptr)                                                   \
            {                                                                         \
                fprintf(stream__, "<span class = \"error\">ERROR: %s\n</span>"        \
                                  "\t\t\t\tat %s : %d : %s\n",                        \
                         #ERROR__, __FILE__, __LINE__, __PRETTY_FUNCTION__);          \
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
            FILE* stream__ = dumpsystem_get_opened_stream();                          \
            if(stream__ != nullptr)                                                   \
            {                                                                         \
                fprintf(stream__, "\t\t\t\tat %s : %d : %s\n",                        \
                        __FILE__, __LINE__, __PRETTY_FUNCTION__);                     \
            }                                                                         \
                                                                                      \
            {ACTION__}                                                                \
        }                                                                             \
    } while(0)                                                                        \

#define LOG$(MESSAGE__, ...)                                                          \
    {                                                                                 \
        FILE* stream__ = dumpsystem_get_opened_stream();                              \
        if(stream__ != nullptr)                                                       \
        {                                                                             \
            fprintf(stream__, "<span class = \"title\">" MESSAGE__ " %s\n</span>"     \
                              "\t\t\t\tat %s : %d : %s\n",                            \
                    ##__VA_ARGS__, __func__, __FILE__, __LINE__, __PRETTY_FUNCTION__);\
        }                                                                             \
    }                                                                                 \

#define MSG$(MESSAGE__, ...)                                                          \
    {                                                                                 \
        FILE* stream__ = dumpsystem_get_opened_stream();                              \
        if(stream__ != nullptr)                                                       \
        {                                                                             \
            fprintf(stream__, "<span class = \"msg\">" MESSAGE__ " \n</span>",        \
                    ##__VA_ARGS__);                                                   \
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
FILE* dumpsystem_get_opened_stream();

#endif // DUMP_SYSTEM_H
