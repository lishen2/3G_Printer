#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stm32f10x.h"
#include "utils.h"
#include "ringbuf.h"
#include "usart_io.h"
#include "com_msghandler.h"
#include "com_gsm.h"
#include "timer_list.h"
#include "printer.h"

/* 最大报文长度 */
#define COM_MAX_PACKET_LENGTH 8192

/* 接收数据最大等待时间 30秒 */
#define COM_MAXWAIT_TIME  30 * HZ

/* 命令 */
#define COM_CMDSTR_HEARTBEAT1  "\x12\x13"   /* 心跳 */
#define COM_CMDSTR_HEARTBEAT2  "@#^"        /* 心跳 */
#define COM_CMDSTR_PRINT       "P"          /* 执行打印操作 */
#define COM_CMDSTR_OK          "OK"         /* 服务器返回OK的状态 */

/* 通信命令token */
enum ComCmdToken{
    COM_CMDTOKEN_HEARTBEAT1 = 0,
	COM_CMDTOKEN_HEARTBEAT2,
    COM_CMDTOKEN_PRINT,
    COM_CMDTOKEN_OK,
    COM_CMDTOKEN_UNKNOWN = 100,
};

/* 需要和token一一对应 */
const char* g_cmdStr[] = {
    COM_CMDSTR_HEARTBEAT1,
	COM_CMDSTR_HEARTBEAT2,
    COM_CMDSTR_PRINT,
    COM_CMDSTR_OK,
};

static char g_msgBuf[COM_MAX_PACKET_LENGTH];

/* 解析命令 */
static int _parserCmd(char* msgBuf, char** pparg)
{
    char *sp, *cmd, *arg;
    int token, i;

    cmd = msgBuf;
    
    sp = strchr(msgBuf, ':');
    if (NULL == sp){
        arg = NULL;
    } else {
        *sp = '\0';
        arg = ++sp;
    }
    *pparg = arg;

    token = COM_CMDTOKEN_UNKNOWN;
    for(i = 0; i < sizeof(g_cmdStr)/sizeof(char*); ++i){
        if (0 == strcmp(cmd, g_cmdStr[i])){
            token = i;
			break;
        }
    }

    return token;
}

static int _cmdDoPrint(char* arg)
{
    int length;
    int i;
    int ret;
    unsigned char cbuf;
	unsigned char status;
    int timestamp;

    if (NULL == arg){
        return ERROR_FAILED;
    }

    /* 读取长度 */ 
    length = atoi(arg);

	if (length > COM_MAX_PACKET_LENGTH || length <= 0){
		 printf("Print data length error [%u].\r\n", length);
		 return 1;
	 }

	 /* 打时间戳，开始接收数据 */
	timestamp = g_jiffies + COM_MAXWAIT_TIME;
	for (i = 0; i < length;){
		 ret = USARTIO_RecvChar(GSM_USART_PORT, &cbuf);
		 if (ERROR_SUCCESS == ret){
			 g_msgBuf[i++] = cbuf;
		 }
	
		 /* 如果等待超时，不完整也退出 */
		 if (time_after(g_jiffies, timestamp)){
			 break;
		 }
	}
	
	/* 接收失败，退出 */
	if (i != length){
		printf("Recving data timeout.\r\n");
		return 2;
	}

	/* 发送到打印机之前，首先判断是否有纸，如果缺纸直接退出，就不再向打印机发送了 */
	status = PRINT_GetPrintStatus();
	if (0 == status){
		/* 返回 和打印通讯失败 */
		printf("Query print status timeout.\r\n");
		return 3;
	} 
	if (0 != (PRINT_STATUS_N1_OUTLINE & status)){
		/* 返回 打印机缺纸 */
		printf("Paper run out.\r\n");
		return 4;
	}

	/* 如果接收成功，发送到打印机 */
	USARTIO_SendData(PRINT_USART_PORT, (unsigned char*)g_msgBuf, i);

	return ERROR_SUCCESS;
}

static void _cmdPrint(char* arg)
{
	char errmsg[16];
	int ret;

	ret = _cmdDoPrint(arg);
	if (ERROR_SUCCESS == ret){
		USARTIO_SendString(GSM_USART_PORT, "OK\n");
	} else if (ret > 0) {
		snprintf(errmsg, sizeof(errmsg), "ERROR:%d\n", ret);
		errmsg[sizeof(errmsg) - 1] = 0;
		USARTIO_SendString(GSM_USART_PORT, (unsigned char*)errmsg);
	}

    return;
}

/* 处理消息 */
enum COM_ReciveResault COM_HandleMessage(void)
{
    int ret;
    int token = COM_CMDTOKEN_UNKNOWN;
    enum COM_ReciveResault resault;
    char* arg;
    
    while(1){
        ret = USARTIO_ReadLine(GSM_USART_PORT, (unsigned char*)g_msgBuf, sizeof(g_msgBuf));
        if (ERROR_SUCCESS != ret){
            break;		
        }

        token = _parserCmd(g_msgBuf, &arg);

        switch(token)
        {
            case COM_CMDTOKEN_HEARTBEAT1:
			case COM_CMDTOKEN_HEARTBEAT2:
            {
                resault = COM_REC_RESAULT_HEARTBEAT;
                break;
            }
            case COM_CMDTOKEN_PRINT:
            {
                _cmdPrint(arg);
                resault = COM_REC_RESAULT_OK;
                break;
            }
            case COM_CMDTOKEN_OK:
            {
                resault = COM_REC_RESAULT_OK;
                break;
            }
            default:
            {
                resault = COM_REC_RESAULT_NONE;
                break;
            }
        };//switch
    };//for

    return resault;
}

