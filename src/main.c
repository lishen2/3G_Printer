#include <stdio.h>
#include "stm32f10x.h"
#include "utils.h"
#include "com_gsm.h"
#include "led.h"
#include "ringbuf.h"
#include "usart_io.h"
#include "buzzer.h"
#include "config.h"
#include "timer_list.h"
#include "debug.h"
#include "button.h"

int main()
{
	/* Ӳ����ʼ�� */
	HW_CommonInit();
	HW_InitSysTick();

    /* ������GSMģ�� */
	GSM_Init();
    GSM_PowerOn();
    
	//BUZ_Init();
	LED_Init();
	BTN_Init();

	USARTIO_InitUsart1();
	USARTIO_InitUsart2();
	USARTIO_InitUsart3();

	/* ���Դ�MicroSD����ȡ���ò�����FLASH */
	CONF_InitConfig();

	/* ������ɺ󣬷�����ʾ�� */
    if (BOOL_TRUE == CONF_IsUpdated()){
        
      	printf("Config updated\r\n");
		BUZ_UpdateConfig();

    } else {
    
    	printf("Started\r\n");
    	BUZ_StartupOK();
    }

	/* ���� */
	//DEBUG_UsartInit();

	/* ע��GSM������ */
    GSM_RegStart();

	/* ���붨ʱ��ѭ�� */
	while(1){
		TLST_Poll();
        delay_ms(2);
	}
}




