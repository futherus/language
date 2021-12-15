
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include "Tree.h"

static const char TREE_DUMPFILE[] = "tree_dump.html";

#define PRINT(format, ...) fprintf(stream, format, ##__VA_ARGS__)

/////////////////////////////////////////////////////////////////////////////////////////////

static long DUMP_ITERATION = 0;
static FILE* TEMP_GRAPH_STREAM = nullptr;

static const char GRAPHVIZ_PNG_NAME[]  = "tree_dump/graphviz_dump";
static const char GRAPHVIZ_TEMP_FILE[] = "tree_dump/graphviz_temp.txt";

static const char GRAPHVIZ_INTRO[] =
R"(
digraph G{
    graph [dpi = 100];
    bgcolor = "#2F353B";
    ranksep = 0.5;
    nodesep = 0.75;
    edge[minlen = 3, arrowsize = 2, penwidth = 1.5, color = green];
    node[shape = rectangle, style = "filled", fillcolor = "#C5D0E6", fontsize = 30,
         color = "#C5D0E6", penwidth = 5];
)";

static const char GRAPHVIZ_OUTRO[] = "}\n";

static char* graphviz_png_()
{
    static char filename[FILENAME_MAX] = "";
    sprintf(filename, "%s_%ld.png", GRAPHVIZ_PNG_NAME, DUMP_ITERATION);

    return filename;
}

static void tree_print_node_(Node* node, size_t)
{
    FILE* stream = TEMP_GRAPH_STREAM;

    PRINT("node%p[label = \"%s\", ", node, demangle(&node->tok));

    switch(node->tok.type)
    {
        case TYPE_OP:
            PRINT("shape = diamond, color = red]");
            break;

        case TYPE_EMBED:
            PRINT("shape = pentagon, color = red]");
            break;
        
        case TYPE_KEYWORD: case TYPE_AUX :
            PRINT("shape = parallelogram, color = black]");
            break;
            
        case TYPE_VAR:
            PRINT("shape = box, color = blue]");
            break;
        
        case TYPE_NUMBER:
            PRINT("shape = oval, color = yellow]");
            break;

        case TYPE_FUNC:
            PRINT("shape = octagon, color = blue]");
            break;

        case TYPE_ID:
            PRINT("shape = doubleoctagon, color = red, penwidth = 10]");
            break;

        default: case TYPE_NOTYPE : case TYPE_EOF : 
        {
            assert(0);
        }
    }
    PRINT("\n");
    
    if(node->left  != nullptr)
        PRINT("node%p -> node%p [color = green];\n", node, node->left);
    if(node->right != nullptr)
        PRINT("node%p -> node%p [color = orange];\n", node, node->right);
}
static void tree_graph_dump_(Tree* tree)
{
    FILE* stream = fopen(GRAPHVIZ_TEMP_FILE, "w");
    if(!stream)
    {
        perror(__FILE__": can't open temporary dump file");
        return;
    }
    TEMP_GRAPH_STREAM = stream;

    PRINT("%s", GRAPHVIZ_INTRO);

    tree_visitor(tree, &tree_print_node_);

    PRINT("%s", GRAPHVIZ_OUTRO);

    fclose(stream);

    char sys_cmd[FILENAME_MAX] = "dot tree_dump/graphviz_temp.txt -q -Tpng -o ";
    strcat(sys_cmd, graphviz_png_());

    system(sys_cmd);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static FILE* DUMP_STREAM = nullptr;

static const char HTML_INTRO[] =
R"(
<html>
    <head>
        <title>
            Tree log
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
)";

static const char HTML_OUTRO[] =
R"(
        </pre>
    </body>
</html>
)";

static const char DATA_IS_NULL_MSG[] = "\n                                                                                            "
                                       "\n  DDDDDD      A    TTTTTTTTT    A         IIIII   SSSSS     N    N  U     U  L      L       "
                                       "\n  D     D    A A       T       A A          I    S          NN   N  U     U  L      L       "
                                       "\n  D     D   A   A      T      A   A         I     SSSS      N N  N  U     U  L      L       "
                                       "\n  D     D  AAAAAAA     T     AAAAAAA        I         S     N  N N  U     U  L      L       "
                                       "\n  DDDDDD  A       A    T    A       A     IIIII  SSSSS      N   NN   UUUUU   LLLLLL LLLLLL  "
                                       "\n                                                                                            ";

static const char ERROR_MSG[]        = "\n                                          "
                                       "\n  EEEEEE  RRRR    RRRR     OOOO   RRRR    "
                                       "\n  E       R   R   R   R   O    O  R   R   "
                                       "\n  EEEE    RRRR    RRRR    O    O  RRRR    "
                                       "\n  E       R   R   R   R   O    O  R   R   "
                                       "\n  EEEEEE  R    R  R    R   OOOO   R    R  "
                                       "\n                                          ";

void tree_dump(Tree* tree, const char msg[], tree_err errcode)
{
    DUMP_ITERATION++;

    FILE* stream = DUMP_STREAM;
    if(stream == nullptr)
        return;
    
    PRINT("<span class = \"title\">\n----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------</span>\n");

    if(errcode)
    {
        PRINT("<span class = \"error\"> %s\n", ERROR_MSG);
        PRINT("%s (%d)\n", msg, errcode);
    }
    else    
        PRINT("<span class = \"title\"> %s\n", msg);

    PRINT("</span>\n");
    
    if(!tree)
    {
        PRINT("<span class = \"error\"> %s </span>\n", DATA_IS_NULL_MSG);
        PRINT("<span class = \"title\">\n----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------</span>\n");
        return;
    }

    PRINT("size: %lld\n" "capacity: %lld\n", tree->size, tree->cap);

    if(!tree->root)
    {
        PRINT("<span class = \"error\"> %s </span>\n", DATA_IS_NULL_MSG);
        PRINT("<span class = \"title\">\n----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n</span>");
        return;
    }
    PRINT("\n\n\n");
    tree_graph_dump_(tree);

    PRINT(R"(<img src = ")" "%s" R"(" alt = "Graphical dump" height = 720>)", graphviz_png_());

    PRINT("<span class = \"title\">\n\n----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n</span>");
}

/////////////////////////////////////////////////////////////////////////////////////////////

#undef PRINT

static void close_dumpfile_()
{
    fprintf(DUMP_STREAM, "%s", HTML_OUTRO);

    if(fclose(DUMP_STREAM) != 0)
        perror(__FILE__": file can't be succesfully closed");
}

void tree_dump_init(FILE* dumpstream)
{
    if(dumpstream)
    {
        DUMP_STREAM = dumpstream;
        return;
    }

    if(TREE_DUMPFILE[0] != 0)
    {
        DUMP_STREAM = fopen(TREE_DUMPFILE, "w");


        if(DUMP_STREAM)
        {
            atexit(&close_dumpfile_);
            fprintf(DUMP_STREAM, "%s", HTML_INTRO);
            
            return;
        }
    }

    perror(__FILE__ ": can't open dump file");
    DUMP_STREAM = stderr;

    return;
}
