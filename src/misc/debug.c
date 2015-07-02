#include "stm32f10x.h"
#include "debug.h"
#include "ringbuf.h"
#include "usart_io.h"
#include "timer_list.h"
#include "utils.h"

static char g_debugBuf[256];

static void _CopyUsartCallback(void)
{
    int ret;

    ret = USARTIO_ReadLine(USART1, (unsigned char*)g_debugBuf, sizeof(g_debugBuf));
    if (ERROR_SUCCESS == ret){

		USARTIO_SendString(USART3, (unsigned char*)g_debugBuf);
		if (g_debugBuf[0] != '+'){
			USARTIO_SendString(USART3, "\r\n");
		}
    }
   
    ret = USARTIO_ReadLine(USART3, (unsigned char*)g_debugBuf, sizeof(g_debugBuf));
    if (ERROR_SUCCESS == ret){
        USARTIO_SendString(USART1, (unsigned char*)g_debugBuf);
		USARTIO_SendString(USART1, "\r\n");
    } 

    TLST_AddTimer(20, _CopyUsartCallback);
    
    return;
}

void DEBUG_UsartInit(void)
{
    TLST_AddTimer(10, _CopyUsartCallback);    
    
    return;
}

