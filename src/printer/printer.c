#include <stdio.h>
#include "stm32f10x.h"
#include "ringbuf.h"
#include "usart_io.h"
#include "printer.h"
#include "com_gsm.h"
#include "utils.h"

void PRINT_PrintStatus(void)
{
    int retn;
	int quality;
    char buf[128];
  
    #define PRINT_MSG_BEGIN "****************��ӭʹ��****************\x0A"
	USARTIO_SendData(PRINT_USART_PORT, PRINT_MSG_BEGIN, sizeof(PRINT_MSG_BEGIN));

    retn = snprintf(buf, sizeof(buf), "�豸״̬:%s\x0A", GSM_GetStatusStr());
    if (sizeof(buf) == retn){
        buf[sizeof(buf) - 1] = '\0';
        retn -= 1;
    }
    USARTIO_SendData(PRINT_USART_PORT, (u8*)buf, retn);

	quality = GSM_GetSingalQuality();
	if (quality <= 0){
		retn = snprintf(buf, sizeof(buf), "�ź�����:δ֪\x0A");
	} else {
		quality = (quality * 100)/31;
		retn = snprintf(buf, sizeof(buf), "�ź�����:%d%%\x0A", quality);
	}
    if (sizeof(buf) == retn){
        buf[sizeof(buf) - 1] = '\0';
        retn -= 1;
    }
    USARTIO_SendData(PRINT_USART_PORT, (u8*)buf, retn);

    retn = snprintf(buf, sizeof(buf), "�����ն˱��:%s\x0A", UTILS_GetDeviceID());
    if (sizeof(buf) == retn){
        buf[sizeof(buf) - 1] = '\0';
        retn -= 1;
    }
    USARTIO_SendData(PRINT_USART_PORT, (u8*)buf, retn);
    
    retn = snprintf(buf, sizeof(buf), "�����ն˰汾:%s\x0A", UTILS_GetVersion());
    if (sizeof(buf) == retn){
        buf[sizeof(buf) - 1] = '\0';
        retn -= 1;
    }
    USARTIO_SendData(PRINT_USART_PORT, (u8*)buf, retn);
    
    #define PRINT_MSG_END "****************��ӭʹ��****************\x0A"
    USARTIO_SendData(PRINT_USART_PORT, PRINT_MSG_END, sizeof(PRINT_MSG_END));
	   
	USARTIO_SendData(PRINT_USART_PORT, "\x1D\x56\x41\x3C", 4);
	return;   
}


