#include "stm32f10x.h"
#include "led.h"

#define LED_MCUSTATUS_PIN   GPIO_Pin_3

#define LED_TIMER          TIM2
#define LED_MCUSTATUS_PORT  GPIOA 
#define LED_MCUSTATUS_CLOCK RCC_APB2Periph_GPIOA
#define LED_TIM_RCC        RCC_APB1Periph_TIM2
#define LED_TIM_IRQ        TIM2_IRQn
#define LED_TIM_ROUTINE    TIM2_IRQHandler

static int g_totalCycle;
static int g_lightCycle; 
static int g_disCounter;

static void _timerInit(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef         NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(LED_TIM_RCC, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = LED_TIM_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_DeInit(LED_TIMER);	
	TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/1000;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
	TIM_TimeBaseStructure.TIM_Period = 1000;
	TIM_TimeBaseInit(LED_TIMER, &TIM_TimeBaseStructure);

	TIM_ARRPreloadConfig(LED_TIMER, ENABLE);
	TIM_SetCounter(LED_TIMER, 1000);
    TIM_ClearFlag(LED_TIMER, TIM_FLAG_Update);  
    TIM_ITConfig(LED_TIMER, TIM_IT_Update, ENABLE); 
    				   
	TIM_Cmd(LED_TIMER, ENABLE);

    return;
}

void LED_TIM_ROUTINE(void)
{
    if (TIM_GetITStatus(LED_TIMER, TIM_IT_Update) != RESET){
        TIM_ClearITPendingBit(LED_TIMER , TIM_FLAG_Update);
		
		if (g_disCounter < g_lightCycle){
			GPIO_SetBits(LED_MCUSTATUS_PORT, LED_MCUSTATUS_PIN);
		} else {
			GPIO_ResetBits(LED_MCUSTATUS_PORT, LED_MCUSTATUS_PIN);
		} 

		g_disCounter++;
		if (g_disCounter > g_totalCycle){
			g_disCounter = 0;
		}
    }

	return;	
}

/*
void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	//…Ë÷√gpio
	RCC_APB2PeriphClockCmd(LED_MCUSTATUS_CLOCK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = LED_MCUSTATUS_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(LED_MCUSTATUS_PORT, &GPIO_InitStructure);	

	LED_SetMode(LED_NORMAL);

	_timerInit();

	return;
} */

void LED_Init(void) {};

void LED_SetMode(enum LED_DisplayMode mode)
{
	switch(mode)
	{
		case LED_ERROR:
		{
			g_totalCycle = 10;
			g_lightCycle = 3;
			break;
		}
		case LED_NORMAL:
		default:
		{
			g_totalCycle = 30;
			g_lightCycle = 3;
			break;
		}
	}


	return;	
}



