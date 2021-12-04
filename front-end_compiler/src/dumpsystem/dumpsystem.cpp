#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dumpsystem.h"

static const size_t TXT_TIME_CAP   = 512;
static const size_t DUMPSTREAM_CAP = 16;

static const char LOG_INTRO[] =
R"(
<html>
    <head>
        <title>
            Log
        </title>
        <style>
            .ok {color: springgreen;font-weight: bold;}
            .error{color: red;font-weight: bold;}
            .log{color: #C5D0E6;}
            .title{color: #E59E1F;text-align: center;font-weight: bold;}
        </style>
    </head>
    <body bgcolor = "#2F353B">
        <pre class = "log">
----------------------------------------------------------------------------------------------------
Dumpsystem activated this stream
)";

static const char LOG_OUTRO[] = 
R"(
End of log
Dumpsystem disactivated this stream
----------------------------------------------------------------------------------------------------
        </pre>
    </body>
</html>
)";

struct Dumpstream
{
    FILE* stream = nullptr;

    int state = -1;
};

static struct Dumpstreams
{
    Dumpstream arr[DUMPSTREAM_CAP] = {};
} DMP;

static void dumpsystem_close_streams_();

#define SET_FILE(DESCRNAME, FILENAME)                                                  \
    case DESCR_##DESCRNAME:                                                            \
    {                                                                                  \
        Dumpstream* dmpstr = &DMP.arr[DESCR_##DESCRNAME];                              \
                                                                                       \
        if(dmpstr->state == -1)                                                        \
        {                                                                              \
            dmpstr->stream = fopen(FILENAME, "w");                                     \
                                                                                       \
            if(dmpstr->stream == nullptr)                                              \
            {                                                                          \
                dmpstr->state = -2;                                                    \
                perror("Cannot open `" #FILENAME "` for dump");                        \
                return nullptr;                                                        \
            }                                                                          \
                                                                                       \
            dmpstr->state = 1;                                                         \
            time_t t_time = time(nullptr);                                             \
            strftime(txt_time, TXT_TIME_CAP, "%H:%M:%S %d.%m.%Y", localtime(&t_time)); \
            fprintf(dmpstr->stream, "%s (%s)\n\n", LOG_INTRO, txt_time);               \
        }                                                                              \
                                                                                       \
        return dmpstr->stream;                                                         \
    }                                                                                  \

FILE* dumpsystem_get_stream_(int descriptor)
{
    static bool first_call = 1;

    char txt_time[TXT_TIME_CAP] = "";

    if(first_call)
    {
        first_call = 0;
        atexit(&dumpsystem_close_streams_);
    }
    
    switch(descriptor)
    {
        case DESCR_stderr:
            return stderr;

        #include "dumpsystem_files.inc"

        default:
            return stderr;
    }
}
#undef SET_FILE

static void dumpsystem_close_streams_()
{
    for(int iter = DESCR_stderr + 1; iter < DESCR_END; iter++)
    {
        Dumpstream* temp = &DMP.arr[iter];

        if(temp->state == 1)
        {
            fprintf(temp->stream, "%s", LOG_OUTRO);

            if(fclose(temp->stream) != 0)
                perror("One of dumpfiles closed unsuccesfully");
        }
    }
}
