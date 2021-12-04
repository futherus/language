#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "../hash.h"
#include "lexer.h"
#include "../dumpsystem/dumpsystem.h"

static void clean_whitespaces_(char* txt, size_t* pos)
{
    assert(txt && pos);

    while(isspace(txt[*pos]))
        (*pos)++;
}

static bool get_number(char* ptr, double* num, int* n_read)
{
    if(isdigit(*ptr))
    {
        sscanf(ptr, "%lg%n", num, n_read);
        return true;
    }

    return false;
}

static void wordlen_(char* ptr, int* n_read)
{
    assert(ptr && n_read);

    if(ispunct(*ptr))
    {
        (*n_read)++;
        return;
    }

    while(isalnum(ptr[*n_read]) || ptr[*n_read] == '_')
        (*n_read)++;
}

static Token TMP_TOK_    = {};
static bool  IS_TMP_TOK_ = false;
static size_t DATA_SZ_   = 0;

#define DEF_KEY(HASH, NAME, STD_NAME, MANGLE)   \
    case(HASH):                                 \
        TMP_TOK_.type = TYPE_KEYWORD;           \
        TMP_TOK_.val.key = TOK_##MANGLE;        \
        IS_TMP_TOK_ = true;                     \
        pos += n_read;                          \
        return LEXER_NOERR;                     \

#define DEF_OP(HASH, NAME, STD_NAME, MANGLE)    \
    case(HASH):                                 \
        TMP_TOK_.type = TYPE_OP;                \
        TMP_TOK_.val.op = TOK_##MANGLE;         \
        IS_TMP_TOK_ = true;                     \
        pos += n_read;                          \
        return LEXER_NOERR;                     \

#define DEF_EMB(HASH, NAME, STD_NAME, MANGLE)   \
    case(HASH):                                 \
        TMP_TOK_.type = TYPE_EMBED;             \
        TMP_TOK_.val.emb = TOK_##MANGLE;        \
        IS_TMP_TOK_ = true;                     \
        pos += n_read;                          \
        return LEXER_NOERR;                     \

static lexer_err lexer_(char* data)
{
    static char*  txt = nullptr;
    static size_t pos = 0;

    ASSERT_RET$(txt || data, LEXER_NULLPTR);

    if(data)
    {
        TMP_TOK_    = {};
        IS_TMP_TOK_ = false;
        txt = data;
        pos = 0;
    }

    clean_whitespaces_(txt, &pos);

    if(pos == DATA_SZ_)
    {
        TMP_TOK_.type = TYPE_EOF;
        TMP_TOK_.val = {};

        return LEXER_NOERR;
    }
    
    int n_read = 0;

    if(get_number(txt + pos, &TMP_TOK_.val.num, &n_read))
    {
        TMP_TOK_.type = TYPE_NUMBER;
        IS_TMP_TOK_   = true;
        pos += n_read;

        return LEXER_NOERR;
    }

    wordlen_(txt + pos, &n_read);

    ASSERT_RET$(n_read > 0, LEXER_UNKNWN_SYMB);

    uint64_t hash = fnv1_64(txt + pos, n_read);
    switch(hash)
    {
        #include "../reserved_operators.inc"
        #include "../reserved_keywords.inc"
        #include "../reserved_embedded.inc"

        default:
        { /*fallthrough*/ }
    }

    TMP_TOK_.type       = TYPE_ID;
    IS_TMP_TOK_         = true;
    TMP_TOK_.val.name   = txt + pos;
    TMP_TOK_.name_len   = n_read;

    pos += n_read;

    return LEXER_NOERR;
}

#undef DEF_OP
#undef DEF_KEY
#undef DEF_EMB

lexer_err lexer_init(char* data, size_t data_sz)
{
    assert(data);
    ASSERT_RET$(data_sz > 0, LEXER_EMPTY_DATA);

    DATA_SZ_ = data_sz;

    return lexer_(data);
}

lexer_err consume(Token* tok)
{
    assert(tok);

    if(!IS_TMP_TOK_)
        PASS$(!lexer_(nullptr), return LEXER_PASS_ERR; );

    *tok = TMP_TOK_;
    IS_TMP_TOK_ = false;

    return LEXER_NOERR;
}

lexer_err peek(Token* tok)
{
    assert(tok);

    if(!IS_TMP_TOK_)
        PASS$(!lexer_(nullptr), return LEXER_PASS_ERR; );

    *tok = TMP_TOK_;

    return LEXER_NOERR;
}

#define DEF_OP(HASH, NAME, STD_NAME, MANGLE)    \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (STD_NAME));      \
        break;                                  \

#define DEF_KEY(HASH, NAME, STD_NAME, MANGLE)   \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (STD_NAME));      \
        break;                                  \
    
#define DEF_EMB(HASH, NAME, STD_NAME, MANGLE)   \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (STD_NAME));      \
        break;                                  \

char* demangle(const Token* tok)
{
    assert(tok);

    static char buffer[1000] = "";

    switch(tok->type)
    {
        case TYPE_NUMBER:
            sprintf(buffer, "%lg", tok->val.num);
            break;

        case TYPE_VAR : case TYPE_FUNC : case TYPE_CONSTANT :
            for(int iter = 0; iter < tok->name_len; iter++)
                buffer[iter] = tok->val.name[iter];

            buffer[tok->name_len] = '\0';
            break;

        case TYPE_OP:
        {
            switch(tok->val.op)
            {
                #include "../reserved_operators.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_KEYWORD:
        {
            switch(tok->val.key)
            {
                #include "../reserved_keywords.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_EMBED:
        {
            switch(tok->val.emb)
            {
                #include "../reserved_embedded.inc"

                default:
                    assert(0);
            }

            break;
        }

        default: case TYPE_NOTYPE : case TYPE_EOF : case TYPE_ID :
            ASSERT$(0, LEXER_NOTFOUND, assert(0); );
            break;
    }

    return buffer;
}

#undef DEF_OP
#undef DEF_KEY
#undef DEF_EMB
