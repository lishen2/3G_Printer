#include <string.h>
#include <stdio.h>
#include "stm32f10x.h"
#include "ringbuf.h"
#include "usart_io.h"
#include "utils.h"
#include "com_utils.h"
#include "watchdog.h"

/* ִ������صĽ�� */
#define COM_UTILS_RESPONSE_OK        "OK"
#define COM_UTILS_RESPONSE_OK_LEN    (sizeof(COM_UTILS_RESPONSE_OK) - 1)
#define COM_UTILS_RESPONSE_ERROR     "ERROR"
#define COM_UTILS_RESPONSE_ERROR_LEN (sizeof(COM_UTILS_RESPONSE_ERROR) - 1)

/* ���ĵ����������е����� */
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

    /* ����û���������У�Ҳ��Ҫ�Ѳ�����ַ�����ɾ� */
    while (1){
        ret = USARTIO_RecvChar(port, &ch);
        if (ERROR_SUCCESS != ret){
            break;
        }
    }

    return;
}

/* �ȴ��豸���ؽ����ֱ���յ�OK����ERROR���˳� */
int CUTILS_WaitForResault(void *port, 
                          int delay_time, 
                          int delay_count)
{
    int read_ret;
    int ret = ERROR_TIMEOUT;
    int retry = delay_count;
    s8 buf[32];
    
    /* ��ȡ�豸�������ݲ����н��� */
    while(retry--) {
        read_ret = USARTIO_ReadLine(port, 
                                    (u8*)buf, 
                                    sizeof(buf));
        if (ERROR_SUCCESS != read_ret){
            WDG_Reload();
            delay_ms(delay_time);
            continue;
        }

        /* ��ȡ���ɹ� */
        if (0 == strncmp((char*)buf, 
                         COM_UTILS_RESPONSE_OK, 
                         COM_UTILS_RESPONSE_OK_LEN)){
            ret = ERROR_SUCCESS;
            break;
        }

        /* ��ȡ��ʧ�ܣ�����ʧ��ʱ���ܰ��������룬�����ж��Ƿ����ERROR */
        if (NULL != strstr((char*)buf, 
                            COM_UTILS_RESPONSE_ERROR)){
            ret = ERROR_FAILED;
            break;
        }
    }
    
    return ret;
}

/* �ȴ��豸���ؽ����ֱ���յ�OK����ERROR���˳� */
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
    
    /* ��ȡ�豸�������ݲ����н��� */
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

/* ��ȡ��������Ϣ */
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

    /* ��ȡ�豸�������ݲ����н��� */
    while(retry--) {
        read_ret = USARTIO_ReadLine(port, 
                                    (u8*)buf, 
                                    bufsize);
        /* �����ȡʧ�ܣ��ȴ�һЩʱ������ */
        if (ERROR_SUCCESS != read_ret){
            WDG_Reload();
            delay_ms(delay_time);
            continue;
        }

        /* �����ȡ���ɹ�����ʧ�ܣ����˳�ѭ�� */
        if ((0 == strcmp(buf, COM_UTILS_RESPONSE_OK)) || 
            (NULL != strstr(buf, COM_UTILS_RESPONSE_ERROR))){
            break;
        }

        /* �������� */
        if ('\0' == buf[0]){
            continue;
        }

        //ִ�лص���
        cb_ret = cb(buf, userdata);
        if (CTRL_BREAK == cb_ret){
            ret = ERROR_SUCCESS;
            break;
        }
    }

    return ret;
}

/* ��ȡָ�����ȵ����� */
int CUTILS_ReadData(void* port, unsigned char* buf, int bufsize, int timeout)
{
	u32 tm;
    int ret;
	u8 ch;
	int cur;

    /* ��ȡ�豸�������ݲ����н��� */
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


