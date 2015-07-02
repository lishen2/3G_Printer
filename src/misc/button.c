#include <stdio.h>
#include "stm32f10x.h"
#include "utils.h"
#include "button.h"
#include "printer.h"
#include "timer_list.h"

/* 功能键 */
#define BTN_1_PIN          GPIO_Pin_12
#define BTN_1_PORT         GPIOA
#define BTN_1_CLK          RCC_APB2Periph_GPIOA
#define BTN_1_EXITLINE     EXTI_Line12
#define BTN_1_SOURCE_PORT  GPIO_PortSourceGPIOA
#define BTN_1_SOURCE_PIN   GPIO_PinSource12
#define BTN_1_IRQ          EXTI15_10_IRQn
#define BTN_1_IRQ_HANDLER  EXTI15_10_IRQHandler

#define BTN_ANTISHAKE_TIM_RCC   RCC_APB1Periph_TIM3
#define BTN_ANTISHAKE_TIMER     TIM3
#define BTN_ANTISHAKE_TIM_IRQ   TIM3_IRQn
#define BTN_ANTISHAKE_TIM_HDL   TIM3_IRQHandler

/* 等待20ms再次检测按键，用于按键去抖 */
#define BTN_ANTISHAKE_TIMER_COUNT   40

/* 记录按键是否被按下 */
static int g_isButtonPushed = BOOL_FALSE;

static void _timerInit(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef         NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(BTN_ANTISHAKE_TIM_RCC, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = BTN_ANTISHAKE_TIM_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_DeInit(BTN_ANTISHAKE_TIMER);	
	TIM_TimeBaseStructure.TIM_Prescaler = (65535-1);
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
	TIM_TimeBaseInit(BTN_ANTISHAKE_TIMER, &TIM_TimeBaseStructure);

    TIM_ClearFlag(BTN_ANTISHAKE_TIMER, TIM_FLAG_Update);  
    TIM_ITConfig(BTN_ANTISHAKE_TIMER, TIM_IT_Update, ENABLE); 
    				   
    return;
}

static void _pinConfig(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 

    RCC_APB2PeriphClockCmd(BTN_1_CLK|RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitStructure.GPIO_Pin = BTN_1_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;    
    GPIO_Init(BTN_1_PORT, &GPIO_InitStructure);
    GPIO_EXTILineConfig(BTN_1_SOURCE_PORT, 
                        BTN_1_SOURCE_PIN);

	return;
}

static void _intConfig(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;

    /* 电源按键 */
    NVIC_InitStructure.NVIC_IRQChannel = BTN_1_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;   
    NVIC_Init(&NVIC_InitStructure);
                                         
    EXTI_InitStructure.EXTI_Line = BTN_1_EXITLINE;              
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;      
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;                
    EXTI_Init(&EXTI_InitStructure);  

	return;
}

static void _doPrint(void)
{
    if (BOOL_TRUE == g_isButtonPushed){
        g_isButtonPushed = BOOL_FALSE;
        PRINT_PrintStatus();
    }
    
    TLST_AddTimer(200, _doPrint);

    return;
}

void BTN_Init(void)
{
	_pinConfig();
	_timerInit();
	_intConfig();

    TLST_AddTimer(1000, _doPrint);
	
	return;
}

void BTN_1_IRQ_HANDLER(void)
{
    if (EXTI_GetITStatus(BTN_1_EXITLINE) != RESET){
        EXTI_ClearFlag(BTN_1_EXITLINE);

        TIM_SetCounter(BTN_ANTISHAKE_TIMER, BTN_ANTISHAKE_TIMER_COUNT);
		TIM_Cmd(BTN_ANTISHAKE_TIMER, ENABLE);        
    }

	return;
}

void BTN_ANTISHAKE_TIM_HDL(void)
{
    if(TIM_GetITStatus(BTN_ANTISHAKE_TIMER, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(BTN_ANTISHAKE_TIMER , TIM_FLAG_Update);
		TIM_Cmd(BTN_ANTISHAKE_TIMER, DISABLE); 

		/* 执行打印操作 */
        if (Bit_RESET == GPIO_ReadInputDataBit(BTN_1_PORT, BTN_1_PIN)){
            g_isButtonPushed = BOOL_TRUE;
            printf("Button pushed\r\n");
        }

	}//if

	return;
}


