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

/* ����ĳ��� */
#define COM_MAX_PACKET_LENGTH 1024

/* �����������ȴ�ʱ�� 10�� */
#define COM_MAXWAIT_TIME  10 * HZ

/* ���� */
#define COM_CMDSTR_HEARTBEAT   "\x12\x13"    /* ���� */
#define COM_CMDSTR_PRINT       "P"           /* ִ�д�ӡ���� */
#define COM_CMDSTR_OK          "OK"          /* ����������OK��״̬ */

/* ��Ӧ */
#define COM_RESSTR_SUCCESS  "OK\n"       /* ����ִ�гɹ� */
#define COM_RESSTR_ERROR    "ERROR\n"    /* ����ִ��ʧ�� */

/* ���ͳɹ�����Ӧ */
#define COM_RESPONSE_SUCCESS() \
    USARTIO_SendString(GSM_USART_PORT, COM_RESSTR_SUCCESS);

/* ����ʧ�ܵ���Ӧ */
#define COM_RESPONSE_ERROR() \
    USARTIO_SendString(GSM_USART_PORT, COM_RESSTR_ERROR);

/* ͨ������token */
enum ComCmdToken{
    COM_CMDTOKEN_HEARTBEAT = 0,
    COM_CMDTOKEN_PRINT,
    COM_CMDTOKEN_OK,
    COM_CMDTOKEN_UNKNOWN = 100,
};

/* ��Ҫ��tokenһһ��Ӧ */
const char* g_cmdStr[] = {
    COM_CMDSTR_HEARTBEAT,
    COM_CMDSTR_PRINT,
    COM_CMDSTR_OK,
};

static char g_msgBuf[2048];

/* �������� */
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
    int timestamp;

    if (NULL == arg){
        return ERROR_FAILED;
    }

    /* ��ȡ���� */ 
    length = atoi(arg);

	if (length > COM_MAX_PACKET_LENGTH || length <= 0){
		 printf("Print data length error [%u].\r\n", length);
		 return ERROR_FAILED;
	 }
	
	 /* ��ʱ�������ʼ�������� */
	 timestamp = g_jiffies;
	 for (i = 0; i < length;){
		 ret = USARTIO_RecvChar(GSM_USART_PORT, &cbuf);
		 if (ERROR_SUCCESS == ret){
			 g_msgBuf[i++] = cbuf;
		 }
	
		 /* ����ȴ���ʱ��������Ҳ�˳� */
		 if (time_after(g_jiffies, timestamp + COM_MAXWAIT_TIME)){
			 break;
		 }
	 }
	
	 /* ������ճɹ������͵���ӡ�� */
	 if (i == length){
		 USARTIO_SendData(PRINT_USART_PORT, (unsigned char*)g_msgBuf, i);
		 return ERROR_SUCCESS;
	 } else {
		 printf("Recving data timeout.\r\n");
		 return ERROR_FAILED;
	 }
}

static void _cmdPrint(char* arg)
{
	int ret;

	ret = _cmdDoPrint(arg);
	if (ERROR_SUCCESS == ret){
		COM_RESPONSE_SUCCESS();
	} else {
		COM_RESPONSE_ERROR();
	}

    return;
}

/* ������Ϣ */
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
            case COM_CMDTOKEN_HEARTBEAT:
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
