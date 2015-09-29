#include <string.h>
#include <stdio.h>
#include "stm32f10x.h"
#include "ringbuf.h"
#include "usart_io.h"
#include "utils.h"
#include "com_utils.h"
#include "watchdog.h"

/* 执行命令返回的结果 */
#define COM_UTILS_RESPONSE_OK        "OK"
#define COM_UTILS_RESPONSE_OK_LEN    (sizeof(COM_UTILS_RESPONSE_OK) - 1)
#define COM_UTILS_RESPONSE_ERROR     "ERROR"
#define COM_UTILS_RESPONSE_ERROR_LEN (sizeof(COM_UTILS_RESPONSE_ERROR) - 1)

/* 消耗掉缓冲中所有的数据 */
void CUTILS_ConsumeBuf(void* port)
{
    int ret;
    u8 buf[64];
    u8 ch;
    
    while (1){
        ret = USARTIO_ReadLine(port, 
                               buf, 
                               sizeof(buf));

        if (ERROR_SUCCESS != ret){
            break;
        }
    }

    /* 即便没有完整的行，也需要把残余的字符清除干净 */
    while (1){
        ret = USARTIO_RecvChar(port, &ch);
        if (ERROR_SUCCESS != ret){
            break;
        }
    }

    return;
}

/* 等待设备返回结果，直到收到OK或者ERROR才退出 */
int CUTILS_WaitForResault(void *port, 
                          int delay_time, 
                          int delay_count)
{
    int read_ret;
    int ret = ERROR_TIMEOUT;
    int retry = delay_count;
    s8 buf[32];
    
    /* 读取设备返回数据并进行解析 */
    while(retry--) {
        read_ret = USARTIO_ReadLine(port, 
                                    (u8*)buf, 
                                    sizeof(buf));
        if (ERROR_SUCCESS != read_ret){
            WDG_Reload();
            delay_ms(delay_time);
            continue;
        }

        /* 读取到成功 */
        if (0 == strncmp((char*)buf, 
                         COM_UTILS_RESPONSE_OK, 
                         COM_UTILS_RESPONSE_OK_LEN)){
            ret = ERROR_SUCCESS;
            break;
        }

        /* 读取到失败，由于失败时可能包含错误码，所以判定是否包含ERROR */
        if (NULL != strstr((char*)buf, 
                            COM_UTILS_RESPONSE_ERROR)){
            ret = ERROR_FAILED;
            break;
        }
    }
    
    return ret;
}

/* 等待设备返回结果，直到收到OK或者ERROR才退出 */
int CUTILS_WaitForMessage(void *port, 
                          char* successMSG,
                          char* failedMSG,
                          int delay_time, 
                          int delay_count)
{
    int read_ret;
    int ret = ERROR_TIMEOUT;
    int retry = delay_count;
    s8 buf[32];
    
    /* 读取设备返回数据并进行解析 */
    while(retry--) {
        read_ret = USARTIO_ReadLine(port, 
                                    (u8*)buf, 
                                    sizeof(buf));
        if (ERROR_SUCCESS != read_ret){
            WDG_Reload();
            delay_ms(delay_time);
            continue;
        }

        if (NULL != strstr((char*)buf, 
                            successMSG)){
            ret = ERROR_SUCCESS;
            break;
        }
        
        if (NULL != strstr((char*)buf, 
                            failedMSG)){
            ret = ERROR_FAILED;
            break;
        }
    }
    
    return ret;
}

/* 读取并处理消息 */
int CUTILS_ReadAndParserMsg(void* port, 
                            char* buf,
                            int bufsize,
                            CTRL_ReadCB cb,
                            void* userdata,
                            int delay_time, 
                            int delay_count)
{
    int ret = ERROR_FAILED;
    int read_ret;
    int cb_ret;
    int retry = delay_count;

    /* 读取设备返回数据并进行解析 */
    while(retry--) {
        read_ret = USARTIO_ReadLine(port, 
                                    (u8*)buf, 
                                    bufsize);
        /* 如果读取失败，等待一些时间再试 */
        if (ERROR_SUCCESS != read_ret){
            WDG_Reload();
            delay_ms(delay_time);
            continue;
        }

        /* 如果读取到成功或者失败，则退出循环 */
        if ((0 == strcmp(buf, COM_UTILS_RESPONSE_OK)) || 
            (NULL != strstr(buf, COM_UTILS_RESPONSE_ERROR))){
            break;
        }

        /* 跳过空行 */
        if ('\0' == buf[0]){
            continue;
        }

        //执行回调，
        cb_ret = cb(buf, userdata);
        if (CTRL_BREAK == cb_ret){
            ret = ERROR_SUCCESS;
            break;
        }
    }

    return ret;
}

/* 读取指定长度的数据 */
int CUTILS_ReadData(void* port, unsigned char* buf, int bufsize, int timeout)
{
	u32 tm;
    int ret;
	u8 ch;
	int cur;

    /* 读取设备返回数据并进行解析 */
	cur = 0;
	tm = g_jiffies + timeout;
    while(time_after(tm, g_jiffies)) {

		WDG_Reload();

        ret = USARTIO_RecvChar(port, &ch);
        if (ERROR_SUCCESS != ret){
			delay_ms(100);
            continue;
        }

		if (cur < bufsize){
			buf[cur++] = ch;
		} 
		
		if (cur >= bufsize) {
			break;
		}
    }

	if (cur == bufsize){
		return ERROR_SUCCESS;
	} else {
		return ERROR_FAILED;
	}
}


