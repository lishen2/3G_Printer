#include "stm32f10x.h"
#include "led.h"

#define LED_MCUSTATUS_PIN   GPIO_Pin_11
#define LED_MCUSTATUS_PORT  GPIOA 
#define LED_MCUSTATUS_CLOCK RCC_APB2Periph_GPIOA

void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	//…Ë÷√gpio
	RCC_APB2PeriphClockCmd(LED_MCUSTATUS_CLOCK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = LED_MCUSTATUS_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(LED_MCUSTATUS_PORT, &GPIO_InitStructure);	
	
	GPIO_ResetBits(LED_MCUSTATUS_PORT, LED_MCUSTATUS_PIN);

	return;
}


