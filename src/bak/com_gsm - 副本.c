#include <stdio.h>
#include "stm32f10x.h"
#include "com_gsm.h"
#include "utils.h"
#include "ringbuf.h"
#include "usart_io.h"
#include "com_utils.h"
#include "config.h"
#include "timer_list.h"

#define GSM_POWERKEY_PIN   GPIO_Pin_8
#define GSM_POWERKEY_PORT  GPIOA 
#define GSM_POWERKEY_CLOCK RCC_APB2Periph_GPIOA

#define GSM_POWERSTATUS_PIN   GPIO_Pin_15
#define GSM_POWERSTATUS_PORT  GPIOB 
#define GSM_POWERSTATUS_CLOCK RCC_APB2Periph_GPIOB

#define GSM_RESET_PIN	   GPIO_Pin_12
#define GSM_RESET_PORT	   GPIOB
#define GSM_RESET_CLOCK	   RCC_APB2Periph_GPIOB

/* 等待模块返回数据的时间，每次等待100毫秒，等待50次，即最多等待5秒钟 */
#define GSM_WAIT_RESPONSE_TIMEOUT    100
#define GSM_WAIT_RESPONSE_COUNT      50

/* 等待系统响应时，每次重试的次数 */
#define GSM_RETRY_COUNT              10

/* Heart beat上次发送间隔 */
#define GSM_HEART_BEAT_INTERVAL      40 * 100 

/* 心跳丢失的最大次数 */
#define GSM_MAX_BEAT_LOST_COUNT      5

/* 模块状态 */
static int g_GSMStatus = GSM_STATUS_POWERING_ON;

/* 心跳丢失的次数 */
static int g_heartBeatLostCount = 0;

/* 上次发送时间心跳的时间 */
static int g_lastSendTime;

void GSM_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(GSM_POWERKEY_CLOCK | GSM_POWERSTATUS_CLOCK | GSM_RESET_CLOCK
						   , ENABLE);

	//POWER KEY
	GPIO_SetBits(GSM_POWERKEY_PORT, GSM_POWERKEY_PIN);
	GPIO_InitStructure.GPIO_Pin = GSM_POWERKEY_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GSM_POWERKEY_PORT, &GPIO_InitStructure);
	GPIO_SetBits(GSM_POWERKEY_PORT, GSM_POWERKEY_PIN);	

	//poweron status
	GPIO_InitStructure.GPIO_Pin = GSM_POWERSTATUS_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GSM_POWERSTATUS_PORT, &GPIO_InitStructure);
	
	//reset pin
	GPIO_SetBits(GSM_RESET_PORT, GSM_RESET_PIN);
	GPIO_InitStructure.GPIO_Pin = GSM_RESET_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GSM_RESET_PORT, &GPIO_InitStructure);
	GPIO_SetBits(GSM_RESET_PORT, GSM_RESET_PIN);

    g_GSMStatus = GSM_STATUS_POWERING_ON;
    g_lastSendTime = 0;
	g_heartBeatLostCount = 0;
	
	return;
}

void GSM_Reset(void)
{
	GPIO_ResetBits(GSM_RESET_PORT, GSM_RESET_PIN);
	delay_ms(200);
	GPIO_SetBits(GSM_RESET_PORT, GSM_RESET_PIN);

	return;
}

/* 检测是否完成启动 */
static int _waitForStart(void)
{
    int resault;
    int ret;
    int retry;

    resault = ERROR_FAILED;
    for(retry = 0; retry < 10; ++retry) {
        
        /* 首先退出透传模式 */
        delay_ms(1200);
        USARTIO_SendString(GSM_USART_PORT, "+++");       
        delay_ms(1200);

        /* 检测是否正常启动 */
        CUTILS_ConsumeBuf(GSM_USART_PORT);    
        USARTIO_SendString(GSM_USART_PORT, "AT\r\n");
        ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                    GSM_WAIT_RESPONSE_TIMEOUT, 
                                    GSM_WAIT_RESPONSE_COUNT);   

        if (ERROR_SUCCESS == ret){
            resault = ERROR_SUCCESS;
            break;
        }

        delay_ms(1000);
    }

    return resault;
}

static int _doUpdateConfig(void)
{
	int ret;
	char buf[128];

    /* 启用硬件流控 */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIFC=2,2\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);   
    
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

     /* 取消自动透传 */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=0,40,0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);  
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }
    
     /* 关闭连接 */	
	CUTILS_ConsumeBuf(GSM_USART_PORT);	  
	USARTIO_SendString(GSM_USART_PORT, "AT+CIPCLOSE=0\r\n");
	CUTILS_WaitForResault(GSM_USART_PORT, 
 						  GSM_WAIT_RESPONSE_TIMEOUT, 
 						  GSM_WAIT_RESPONSE_COUNT); 

	/* 配置APN */
    snprintf(buf, sizeof(buf), "AT+CSTT=\"%s\"\r\n", CONF_GetAPN());
	buf[sizeof(buf) - 1] = '\0';	
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, (u8*)buf);
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
  	                            GSM_WAIT_RESPONSE_TIMEOUT, 
  	                            GSM_WAIT_RESPONSE_COUNT);
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* 设置单链接 */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPMUX=0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);  
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* 设置透传模式 */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPMODE=1,1\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);  
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* 配置透传参数
    
    自动进入透传模式
    心跳发送间隔40秒
    是否以文本发送
    发送数据延时10毫秒
    收到内容满180字符后立刻发送
    */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=1,40,0,10,180\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

    /* 设置启动注册包(心跳包由MCU发送)
	CREG:SERIAL_NUMBER
	 */
    snprintf(buf, sizeof(buf), "AT+CIPPACK=1,\"255474A3%sA0\"\r\n", UTILS_GetDeviceID());
	buf[sizeof(buf) - 1] = '\0';
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, (u8*)buf);
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
 	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* 保存连接配置
    连接ID为0
    类型为TCP
    域名或IP
    端口
    自动重连，但不发送Keeplive
    客户端模式
    */
    snprintf(buf, sizeof(buf), "AT+CIPSCONT=0,\"TCP\",\"%s\",%hu,1,0\r\n", 
             CONF_GetAddress(), CONF_GetPort());
	buf[sizeof(buf) - 1] = '\0';
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, (u8*)buf);
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);
 	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}    

	/* 这里命令执行成功即认为成功，如果网络不好，通信模块会不断尝试重连 */
    return ERROR_SUCCESS;
}

static int _handleHeartBeat(void)
{
	int ret;

    /* 定时发送心跳 */
    if (time_after(g_jiffies, g_lastSendTime + GSM_HEART_BEAT_INTERVAL)){
		g_lastSendTime = g_jiffies;
        USARTIO_SendString(GSM_USART_PORT, "B\n");

        /* 等待结果 */
	    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
	                                100, 
	                                100);
		
		/* 发送成功，清空失败计数，修改状态为正常 */
		if (ERROR_SUCCESS == ret){
			g_heartBeatLostCount = 0;
			g_GSMStatus = GSM_STATUS_READY;
		} 
		/* 如果发送失败，记录失败次数 */
		else {
			g_heartBeatLostCount += 1;
		}

		if (g_heartBeatLostCount > GSM_MAX_BEAT_LOST_COUNT){
			g_GSMStatus = GSM_STATUS_LOST_CONN;
		}
    }
    
	return ERROR_SUCCESS;
}

static int _updateConfig(void)
{
    int ret;

    /* 等待设备启动 */
    ret = _waitForStart();
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

    /* 建立更新配置 */
    ret = _doUpdateConfig();
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;
}

static void _gsmPoll(void)
{
    int ret;
    
    TLST_AddTimer(1000, _gsmPoll);
    
    switch(g_GSMStatus)
    {
        case GSM_STATUS_POWERING_ON:
        {
            /* 如果配置有修改，执行配置更新 */
            if (BOOL_TRUE == CONF_IsUpdated()){
                g_GSMStatus = GSM_STATUS_UPDATE_CONFIG;
            } 
            /* 配置没有修改，直接进入数据处理模式 */
            else {
                g_GSMStatus = GSM_STATUS_READY;
            }
            
            break;
        }
        case GSM_STATUS_UPDATE_CONFIG:
        {
            /* 更新成功进入数据模式，否则保持在配置模式，1秒后再配置 */
            ret = _updateConfig();
            if (ERROR_SUCCESS == ret){
                g_GSMStatus = GSM_STATUS_READY;
            }
            
            break;
        }
        case GSM_STATUS_READY:
        case GSM_STATUS_LOST_CONN:
        {
            /* 处理发送心跳的任务 */
            _handleHeartBeat();
            
            break;
        }
        default:
        {
            g_GSMStatus = GSM_STATUS_POWERING_ON;
            break;
        };
    };

    return;
}

void GSM_PowerOn(void)
{
    /* 如果没有开机先进行开机 */
	if (Bit_RESET == GPIO_ReadInputDataBit(GSM_POWERSTATUS_PORT, GSM_POWERSTATUS_PIN)){

        /* 按键拉低，定时器关闭 */
		GPIO_SetBits(GSM_POWERKEY_PORT, GSM_POWERKEY_PIN);
        delay_ms(1500);
        GPIO_ResetBits(GSM_POWERKEY_PORT, GSM_POWERKEY_PIN); 
	} 

	return;
}

void GSM_RegStart(void)
{
    /* 100毫秒秒钟后执行启动流程 */
    TLST_AddTimer(100, _gsmPoll);
    
    return;
}

