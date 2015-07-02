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
	/* 硬件初始化 */
	HW_CommonInit();
	HW_InitSysTick();

    /* 先启动GSM模块 */
	GSM_Init();
    GSM_PowerOn();
    
	//BUZ_Init();
	LED_Init();
	BTN_Init();

	USARTIO_InitUsart1();
	USARTIO_InitUsart2();
	USARTIO_InitUsart3();

	/* 尝试从MicroSD卡读取配置并更新FLASH */
	CONF_InitConfig();

	/* 启动完成后，发出提示音 */
    if (BOOL_TRUE == CONF_IsUpdated()){
        
      	printf("Config updated\r\n");
		BUZ_UpdateConfig();

    } else {
    
    	printf("Started\r\n");
    	BUZ_StartupOK();
    }

	/* 调试 */
	//DEBUG_UsartInit();

	/* 注册GSM处理函数 */
    GSM_RegStart();

	/* 进入定时器循环 */
	while(1){
		TLST_Poll();
        delay_ms(2);
	}
}




