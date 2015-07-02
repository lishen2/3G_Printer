#include <stdio.h>
#include "assert.h"
#include "conf_parser.h"

/* 配置文件中键值对的最大长度 */
#define CONF_KEY_MAXSIZE      32
#define CONF_VALUE_MAXSIZE    64

/* 状态和当前的键值 */
static int  g_state;
static char g_key[CONF_KEY_MAXSIZE + 1]; 
static char g_value[CONF_VALUE_MAXSIZE + 1];
static char g_keyPos = 0;
static char g_valuePos = 0;
static CONF_HDL_CB g_handler = NULL;

#define CONF_COPYKEY(c) \
    if (g_keyPos < sizeof(g_key)){\
        g_key[g_keyPos] = c;\
        g_keyPos++;\
    }

#define CONF_COPYVALUE(c) \
    if (g_valuePos < sizeof(g_value)){\
        g_value[g_valuePos] = c;\
        g_valuePos++;\
    }

#define CONF_RESETPOS(c)\
    g_keyPos = 0;\
    g_valuePos = 0;

/* 状态机解析消息时的状态 */
enum CONFParserState
{
    CONF_STATE_START,
    CONF_STATE_COMMENT,
    CONF_STATE_KEY,
    CONF_STATE_VALUE,
};

static void _stateValue(char c)
{
    switch(c)
    {
        case '\n':
        {   
            //写入结束符
            CONF_COPYKEY('\0');
            g_key[sizeof(g_key) - 1] = '\0'; //在缓冲区用光的情况下，在最后填入结束符
            CONF_COPYVALUE('\0');
            g_value[sizeof(g_value) - 1] = '\0'; //在缓冲区用光的情况下，在最后填入结束符            
            g_handler(g_key, g_value);
			ASSERT(NULL != g_handler);
            g_state = CONF_STATE_START;
            break;
        }
        default:
        {
            CONF_COPYVALUE(c);
            break;
        }
    }
}

static void _stateKey(char c)
{
    switch(c)
    {
        case '=':
        {
            g_state = CONF_STATE_VALUE;
            break;
        }
        case '\n':
        {         
            g_state = CONF_STATE_START;
            break;
        }
        default:
        {
            CONF_COPYKEY(c);
            break;
        }
    }
}

static void _stateComment(char c)
{
    switch(c)
    {
        case '\n':
        {         
            g_state = CONF_STATE_START;
            break;
        }
        default:
        {
        }        
    }

    return;
}

static void _stateStart(char c)
{
    switch(c)
    {
        case '#':
        {
            g_state = CONF_STATE_COMMENT;
            break;
        }
        case '\n':
        {            
            g_state = CONF_STATE_START;
            break;
        }
        default:
        {
            CONF_COPYKEY(c);            
            g_state = CONF_STATE_KEY;
            break;
        }
    }

    return;
}

static void _handleChar(char c)
{
    /* 跳过\r和空白字符 */
    if ('\r' == c || ' ' == c || '\t' == c){
        return;
    }

    switch (g_state)
    {
        case CONF_STATE_START:
        {
            _stateStart(c);
            break;
        }
        case CONF_STATE_COMMENT:
        {
            _stateComment(c);
            break;
        }
        case CONF_STATE_KEY:
        {
            _stateKey(c);
            break;
        }
        case CONF_STATE_VALUE:
        {
            _stateValue(c);
            break;
        }
        default:
        {
            break;
        }
    }//switch

    /* 行切换的时候，重置读取和写入位置 */
    if ('\n' == c){
        CONF_RESETPOS();
    }

    return;
}

void CONF_ParserBuf(char* buf, int size)
{
    int i;

    for (i = 0; i < size; ++i){
        _handleChar(buf[i]);
    }

    return;
}

/* 设置处理键值对的回调函数 */
void CONF_SetHandler(CONF_HDL_CB cb)
{
	g_handler = cb;
	
	return;	
}

