#ifndef _CONF_PARSER_H_
#define _CONF_PARSER_H_

/* 用于处理键值对的回调 */
typedef void (*CONF_HDL_CB)(char* key, char* value);

/* 设置处理键值对的回调函数 */
void CONF_SetHandler(CONF_HDL_CB cb);

/* 解析缓冲区 */
void CONF_ParserBuf(char* buf, int size);

#endif

