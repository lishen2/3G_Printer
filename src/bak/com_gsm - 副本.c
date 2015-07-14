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

/* �ȴ�ģ�鷵�����ݵ�ʱ�䣬ÿ�εȴ�100���룬�ȴ�50�Σ������ȴ�5���� */
#define GSM_WAIT_RESPONSE_TIMEOUT    100
#define GSM_WAIT_RESPONSE_COUNT      50

/* �ȴ�ϵͳ��Ӧʱ��ÿ�����ԵĴ��� */
#define GSM_RETRY_COUNT              10

/* Heart beat�ϴη��ͼ�� */
#define GSM_HEART_BEAT_INTERVAL      40 * 100 

/* ������ʧ�������� */
#define GSM_MAX_BEAT_LOST_COUNT      5

/* ģ��״̬ */
static int g_GSMStatus = GSM_STATUS_POWERING_ON;

/* ������ʧ�Ĵ��� */
static int g_heartBeatLostCount = 0;

/* �ϴη���ʱ��������ʱ�� */
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

/* ����Ƿ�������� */
static int _waitForStart(void)
{
    int resault;
    int ret;
    int retry;

    resault = ERROR_FAILED;
    for(retry = 0; retry < 10; ++retry) {
        
        /* �����˳�͸��ģʽ */
        delay_ms(1200);
        USARTIO_SendString(GSM_USART_PORT, "+++");       
        delay_ms(1200);

        /* ����Ƿ��������� */
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

    /* ����Ӳ������ */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIFC=2,2\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);   
    
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

     /* ȡ���Զ�͸�� */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=0,40,0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);  
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }
    
     /* �ر����� */	
	CUTILS_ConsumeBuf(GSM_USART_PORT);	  
	USARTIO_SendString(GSM_USART_PORT, "AT+CIPCLOSE=0\r\n");
	CUTILS_WaitForResault(GSM_USART_PORT, 
 						  GSM_WAIT_RESPONSE_TIMEOUT, 
 						  GSM_WAIT_RESPONSE_COUNT); 

	/* ����APN */
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

    /* ���õ����� */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPMUX=0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);  
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* ����͸��ģʽ */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPMODE=1,1\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);  
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* ����͸������
    
    �Զ�����͸��ģʽ
    �������ͼ��40��
    �Ƿ����ı�����
    ����������ʱ10����
    �յ�������180�ַ������̷���
    */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=1,40,0,10,180\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

    /* ��������ע���(��������MCU����)
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

    /* ������������
    ����IDΪ0
    ����ΪTCP
    ������IP
    �˿�
    �Զ���������������Keeplive
    �ͻ���ģʽ
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

	/* ��������ִ�гɹ�����Ϊ�ɹ���������粻�ã�ͨ��ģ��᲻�ϳ������� */
    return ERROR_SUCCESS;
}

static int _handleHeartBeat(void)
{
	int ret;

    /* ��ʱ�������� */
    if (time_after(g_jiffies, g_lastSendTime + GSM_HEART_BEAT_INTERVAL)){
		g_lastSendTime = g_jiffies;
        USARTIO_SendString(GSM_USART_PORT, "B\n");

        /* �ȴ���� */
	    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
	                                100, 
	                                100);
		
		/* ���ͳɹ������ʧ�ܼ������޸�״̬Ϊ���� */
		if (ERROR_SUCCESS == ret){
			g_heartBeatLostCount = 0;
			g_GSMStatus = GSM_STATUS_READY;
		} 
		/* �������ʧ�ܣ���¼ʧ�ܴ��� */
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

    /* �ȴ��豸���� */
    ret = _waitForStart();
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

    /* ������������ */
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
            /* ����������޸ģ�ִ�����ø��� */
            if (BOOL_TRUE == CONF_IsUpdated()){
                g_GSMStatus = GSM_STATUS_UPDATE_CONFIG;
            } 
            /* ����û���޸ģ�ֱ�ӽ������ݴ���ģʽ */
            else {
                g_GSMStatus = GSM_STATUS_READY;
            }
            
            break;
        }
        case GSM_STATUS_UPDATE_CONFIG:
        {
            /* ���³ɹ���������ģʽ�����򱣳�������ģʽ��1��������� */
            ret = _updateConfig();
            if (ERROR_SUCCESS == ret){
                g_GSMStatus = GSM_STATUS_READY;
            }
            
            break;
        }
        case GSM_STATUS_READY:
        case GSM_STATUS_LOST_CONN:
        {
            /* ���������������� */
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
    /* ���û�п����Ƚ��п��� */
	if (Bit_RESET == GPIO_ReadInputDataBit(GSM_POWERSTATUS_PORT, GSM_POWERSTATUS_PIN)){

        /* �������ͣ���ʱ���ر� */
		GPIO_SetBits(GSM_POWERKEY_PORT, GSM_POWERKEY_PIN);
        delay_ms(1500);
        GPIO_ResetBits(GSM_POWERKEY_PORT, GSM_POWERKEY_PIN); 
	} 

	return;
}

void GSM_RegStart(void)
{
    /* 100�������Ӻ�ִ���������� */
    TLST_AddTimer(100, _gsmPoll);
    
    return;
}

