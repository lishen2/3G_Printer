/*********************************
���ļ�����������GSMģ�飬����������
*********************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stm32f10x.h"
#include "com_gsm.h"
#include "utils.h"
#include "ringbuf.h"
#include "usart_io.h"
#include "com_utils.h"
#include "config.h"
#include "timer_list.h"
#include "com_msghandler.h"

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

/* �����յ�����֮���������������μ������Ϊ�ͷ������Ͽ� */
#define GSM_MAX_RECIVE_INTERVAL   60 * HZ

/* GSMģ���ע��״̬ */
enum GSMRegStatus{
	GSM_REG_STATUS_NOTEVENTRY       = 0,
	GSM_REG_STATUS_REGISTED_LOCAL   = 1,
	GSM_REG_STATUS_SEARCHING        = 2,
	GSM_REG_STATUS_REFUSED          = 3,
	GSM_REG_STATUS_UNKONWN          = 4,
	GSM_REG_STATUS_REGISTED_ROAMING = 5,
	GSM_REG_STATUS_READERROR,
};

/* ģ��״̬ */
static enum GSMStatus g_GSMStatus = GSM_STATUS_POWERING_ON;

/* ����ģ��ʱ��
   ��ģ�����ʱ��������Ҫ����ģ�飬�˱�����¼����ģ���ʱ��� */
static unsigned int g_resetTimestamp = 0;
static int g_needReset = BOOL_FALSE;

/* ģ�����״̬���� */
enum GSMMsgCode{
    GSM_MSG_MODULE_STARTING = 0,
	GSM_MSG_MODULE_NOT_RESPONSE,
	GSM_MSG_MODULE_SEARCHING,
	GSM_MSG_MOBILE_REGIST_FAILED,
	GSM_MSG_IP_NETWORK_CONNECTING,
	GSM_MSG_IP_NETWORK_CONNECTED,
	GSM_MSG_MAX,
};

/* ģ�������Ϣ����״̬����һһ��Ӧ */
static const char* g_msgStrings[] = {
    "ͨ��ģ����������",
	"ͨ��ģ������Ӧ",
	"������Ѱ�ƶ�ͨ������",
	"�ƶ�ͨ����������ʧ�ܣ�����SIM���Ƿ�Ƿ�ѻ�ͣ��",
	"�������ӷ�����",
	"�Ѿ����ӷ�����",
	""
};

/* ��ǰģ���״̬�ַ������� */
static int g_msgCode = GSM_MSG_MAX;

static char g_msgBuf[64];

/* �ź�ǿ�� */
static int g_signalQuality = -1;

/* �ϴ��յ�����ʱ��ʱ��� */
static unsigned g_lastRecvStamp = 0x7FFFFFFF;

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
	
	return;
}

/* ����ģ����ָ����������� */
static void _gsmSetReset(unsigned int msec)
{
	printf("Module not response, reset after %d ms.\n", msec);

	g_resetTimestamp = g_jiffies + (msec/10);
	g_needReset = BOOL_TRUE;

	return;
}

/* ���ģ������ */
static void _gsmClearReset(void)
{
	g_needReset = BOOL_FALSE;
}

/* ִ��ģ������ */
static void _gsmDoReset(void)
{
	printf("Reset module\n");

	GPIO_ResetBits(GSM_RESET_PORT, GSM_RESET_PIN);
	delay_ms(200);
	GPIO_SetBits(GSM_RESET_PORT, GSM_RESET_PIN);

	return;
}

/* ����ִ���������ɹ�����ERROR_SUCCESS��ʧ�ܷ���ERROR_FAILED */
static int _gsmTryReset(void)
{
	int ret;
	
	/* ����ɹ������������ģ��״̬���� */
	if (BOOL_TRUE == g_needReset && time_after(g_jiffies, g_resetTimestamp)){
		
		_gsmDoReset();
		_gsmClearReset();

		ret = ERROR_SUCCESS;
		
	} else {
	
		ret = ERROR_FAILED;
	}

	return ret;
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

static int _basicSetup(void)
{
    int ret;
    
    /* ����Ӳ������ */
	/*
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIFC=0,0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);   
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }*/

    /* �رջ��� */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "ATE0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);   
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

    return ret;
}

/* ����AT+CREG����
AT+CREG?

+CREG: 0,1
*/
static int _parserCREG(char* buf, void* pstatus)
{
	char* cur;
	int status;

	/* ������ʼ��� */
	cur = strstr(buf, "+CREG:");
	if (NULL == cur){
		return CTRL_CONTINUE;
	}

	/* ����״̬ */
	cur = strchr(buf, ',');
	if (NULL == cur){
		return CTRL_CONTINUE;
	}
	cur += 1;
	status = atoi(cur);
	*(int*)pstatus = status;
	
	return CTRL_BREAK;
}

/* ��ȡע����� */
static int _readRegCode(void)
{
    int regCode;
    
	regCode = GSM_REG_STATUS_READERROR;
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CREG?\r\n");
    CUTILS_ReadAndParserMsg(GSM_USART_PORT,
                            g_msgBuf,
                            sizeof(g_msgBuf),
                            _parserCREG,
                            &regCode,
                            GSM_WAIT_RESPONSE_TIMEOUT, 
                            GSM_WAIT_RESPONSE_COUNT);   

    return regCode;
}

/* ����AT+CSQ����
AT+CSQ

+CSQ: 16,0

OK
*/
static int _parserCSQ(char* buf, void* pquality)
{
	char* cur;
	int quality;

	/* ������ʼ��� */
	cur = strstr(buf, "+CSQ:");
	if (NULL == cur){
		return CTRL_CONTINUE;
	}

	/* ����״̬ */
	cur = strchr(buf, ' ');
	if (NULL == cur){
		return CTRL_CONTINUE;
	}
	cur += 1;
	quality = atoi(cur);
	*(int*)pquality = quality;
	
	return CTRL_BREAK;
}

/* ��ȡ�ź�ǿ�� */
static int _retReadSignalQuality(void)
{
    int quality;

	quality = -1;
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CSQ\r\n");
    CUTILS_ReadAndParserMsg(GSM_USART_PORT,
                            g_msgBuf,
                            sizeof(g_msgBuf),
                            _parserCSQ,
                            &quality,
                            GSM_WAIT_RESPONSE_TIMEOUT, 
                            GSM_WAIT_RESPONSE_COUNT);   
    
    return quality;
}

static int _gsmEstablishIPConn(void)
{
	int ret;
	char buf[128];
	char *charid;

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
    
    ���Զ���������
    �������ͼ��40��
    �Ƿ����ı�����
    */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=0,50,0,0,0,0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

    /* ����������
    ����������Ϊ: Ϊ@#^ */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPPACK=0,\"@#^\"\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* ��������ע���
	REG:SERIAL_NUMBER^ */
	charid = UTILS_GetDeviceID();
    snprintf(buf, sizeof(buf), "AT+CIPPACK=1,\"\x52\x45\x47\x3A%s^\"\r\n", charid);
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
    ����ΪTCP
    ������IP
    �˿�
    �Զ���������������Keeplive
    �ͻ���ģʽ
    */
    snprintf(buf, sizeof(buf), "AT+CIPSTART=\"TCP\",\"%s\",%hu,2,C\r\n", 
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

static void _gsmPoll(void)
{
    int ret;
	int regCode;
    
    TLST_AddTimer(100, _gsmPoll);
    
    switch(g_GSMStatus)
    {
        case GSM_STATUS_POWERING_ON:
        {
			/* �ȴ�ģ������ */
			ret = _waitForStart();

			if (ERROR_SUCCESS == ret){

                /* ����Ӳ�����أ����ò����� */
                ret = _basicSetup();
                if (ERROR_SUCCESS == ret){
                    g_GSMStatus = GSM_STATUS_REGISTING;
                    g_msgCode = GSM_MSG_MODULE_SEARCHING;
                }			
			} 
			else {
				g_GSMStatus = GSM_STATUS_FAILED;
				g_msgCode = GSM_MSG_MODULE_NOT_RESPONSE;
				_gsmSetReset(10 * HZ);
			}

            delay_ms(1000);
			
            break;
        }
        case GSM_STATUS_REGISTING:
        { 
            /* ��ȡ�ź�ǿ�� */
            g_signalQuality = _retReadSignalQuality();  
			
            /* ��ȡע��״̬ */
            regCode = _readRegCode();  

			/* ����ɹ�ע�� */
			if (GSM_REG_STATUS_REGISTED_LOCAL == regCode ||
				GSM_REG_STATUS_REGISTED_ROAMING == regCode){

                /* ����IP���� */
				ret = _gsmEstablishIPConn();
                if (ERROR_SUCCESS == ret){
                    g_GSMStatus = GSM_STATUS_CONNECTING;
                    g_msgCode = GSM_MSG_IP_NETWORK_CONNECTING;
                }			
			}
			/* ���ע�ᱻ�ܾ���û�г���ע�� */
            else if (GSM_REG_STATUS_NOTEVENTRY == regCode || 
				     GSM_REG_STATUS_REFUSED == regCode){

				g_msgCode = GSM_MSG_MOBILE_REGIST_FAILED;
            }
            /* GSM_REG_STATUS_SEARCHING, 
			   GSM_REG_STATUS_UNKONWN, 
			   GSM_REG_STATUS_READERROR
			   �⼸��״̬������Ϊ������ע��
			   */
			else {
                g_msgCode = GSM_MSG_MODULE_SEARCHING;
            }        

            delay_ms(1000);
            
            break;
        }
        /* һ����ʼ�������ӣ������͸��ģʽ����ʱֱ�ӵ�����Ϣ���������ж�ȡ�ʹ��� */
        case GSM_STATUS_CONNECTING:
        case GSM_STATUS_CONNECTED:
        {         
            ret = COM_HandleMessage();
            if (COM_REC_RESAULT_OK == ret || COM_REC_RESAULT_HEARTBEAT == ret){
                g_lastRecvStamp = g_jiffies;
            }

            if (time_after(g_jiffies, g_lastRecvStamp + GSM_MAX_RECIVE_INTERVAL)){
                g_GSMStatus = GSM_STATUS_CONNECTING;
                g_msgCode = GSM_MSG_IP_NETWORK_CONNECTING;                
            } else {
                g_GSMStatus = GSM_STATUS_CONNECTED;
                g_msgCode = GSM_MSG_IP_NETWORK_CONNECTED;                                
            }
            
            break;
        }
		/* �����������֧���������� */
		case GSM_STATUS_FAILED:
        default:
        {
			/* �����������㣬�������������״̬�� */
			ret = _gsmTryReset();
			if (ERROR_SUCCESS == ret) {
				g_GSMStatus = GSM_STATUS_POWERING_ON;
                g_msgCode = GSM_MSG_MODULE_STARTING;
			}
			
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
        delay_ms(2000);
        GPIO_ResetBits(GSM_POWERKEY_PORT, GSM_POWERKEY_PIN); 
	} 

	_gsmClearReset();

    g_msgCode = GSM_MSG_MODULE_STARTING;
    
	return;
}

void GSM_RegStart(void)
{
    /* 100�������Ӻ�ִ���������� */
    TLST_AddTimer(100, _gsmPoll);
    
    return;
}

enum GSMStatus GSM_GetStatus(void)
{
    return g_GSMStatus;
}

/* ��ȡģ��״̬�ַ��� */
const char* GSM_GetStatusStr(void)
{
	if (g_msgCode <= GSM_MSG_MAX){
		return g_msgStrings[g_msgCode];
	} else {
		return "";
	}
}

/* ��ȡ�ź����� */
int GSM_GetSingalQuality(void)
{
    return g_signalQuality;
}


