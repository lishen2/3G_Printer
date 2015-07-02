#include "stm32f10x.h"
#include "utils.h"
#include "buzzer.h"

#define BUZ_POWER_PIN   GPIO_Pin_9
#define BUZ_POWER_PORT  GPIOB 
#define BUZ_POWER_CLOCK RCC_APB2Periph_GPIOB

void BUZ_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(BUZ_POWER_CLOCK, ENABLE);

	//POWER KEY
	GPIO_InitStructure.GPIO_Pin = BUZ_POWER_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(BUZ_POWER_PORT, &GPIO_InitStructure);	
	GPIO_ResetBits(BUZ_POWER_PORT, BUZ_POWER_PIN);

	return;
}

void BUZ_StartupOK(void)
{
	return;
}

void BUZ_UpdateConfig(void)
{}

