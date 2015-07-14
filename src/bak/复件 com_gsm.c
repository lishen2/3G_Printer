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
static int g_GSMStatus = GSM_STATUS_POWER_OFF;

/* ģ�����ƶ������ע��״̬ */
static int g_RegCode = GSM_REG_STATUS_READERROR;

/* ��Ϣ������ */
static int g_msgBuf[2048];

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

    g_GSMStatus = GSM_STATUS_POWER_OFF;
				
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
    int ret;
	
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

    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

        /* ����Ӳ������ */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIFC=2,2\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);   

    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;
}

/* ����AT+CREG����
AT+CREG?

+CREG: 0,1
*/
static int _parserCREG(char* buf, int* pstatus)
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
	*pstatus = status;
	
	return CTRL_BREAK;
}

/* �ȴ��豸ע������ */
static int _waitForConnect(void)
{
    int ret;

    /* ����Ƿ��������� */
	g_RegCode = GSM_REG_STATUS_READERROR;
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CREG?\r\n");
    ret = CUTILS_ReadAndParserMsg(GSM_USART_PORT,
								  g_msgBuf,
								  sizeof(g_msgBuf),
								  _parserCREG,
								  &g_RegCode,
                                  GSM_WAIT_RESPONSE_TIMEOUT, 
                                  GSM_WAIT_RESPONSE_COUNT);   

	/* �����ȡ���ɹ�������״̬��Ϊ��ע�ᣬֱ�ӷ���ʧ�ܣ��ȴ���һ�γ��� */
    if (ERROR_SUCCESS != ret || 
		((GSM_REG_STATUS_REGISTED_LOCAL != g_RegCode) &&
		 (GSM_REG_STATUS_REGISTED_ROAMING != g_RegCode))){
		return ERROR_FAILED;
    }

    /* ����GPRS���и��� */
	USARTIO_SendString(GSM_USART_PORT, "AT+CGATT=1\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);  

	/* ���GPRS�Ƿ����� */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CGATT?\r\n");
    ret = CUTILS_WaitForMessage(GSM_USART_PORT,
								"+CGATT: 1",
								"ERROR"
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);   

    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }
    
    return ERROR_SUCCESS;
}

static int _establistTCPConn(void)
{
	int ret;
	char buf[128];
	
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
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPMODE=0,1\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);  
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* ��������ʱ�� */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=0,40,0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* ��������ע���
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
	
    /* ����������
    ����������Ϊ:��д��ĸB */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPPACK=0,\"24A0\"\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

	/* ��������
    ����ΪTCP
    ������IP
    �˿�
    ����Keeplive
    �ͻ���ģʽ
    */
    snprintf(buf, sizeof(buf), "AT+CIPSTART=\"TCP\",\"%s\",%hu,2\r\n", 
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

#ifdef 0
/* �������� */
static void _updateConfig(void)
{
    

     /* ȡ���Զ�͸�� */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=0,40,0\r\n");
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT);  


    /* ����APN */
    snprintf(buf, sizeof(buf), "AT+CSTT=\"%s\"\r\n", CONF_GetAPN());
	buf[sizeof(buf) - 1] = '\0';
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, (u8*)buf);
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT);     

    /* ����͸������
    
    �Զ�����͸��ģʽ
    �������ͼ��40��
    �Ƿ����ı�����
    ����������ʱ10����
    �յ�������180�ַ������̷���
    */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=1,40,0,10,180\r\n");
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT); 

    /* ��������ע���
	CREG:SERIAL_NUMBER
	 */
    snprintf(buf, sizeof(buf), "AT+CIPPACK=1,\"255474A3%sD0A0\"\r\n", UTILS_GetDeviceID());
	buf[sizeof(buf) - 1] = '\0';
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, (u8*)buf);
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT); 
    
    /* ����������
    ����������Ϊ:��д��ĸB */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPPACK=0,\"24D0A0\"\r\n");
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT); 

    /* ���õ����� */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPMUX=0\r\n");
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT);    

    /* ����͸��ģʽ */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPMODE=1,1\r\n");
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT);  

    /* ������������
    ����IDΪ0
    ����ΪTCP
    ������IP
    �˿�
    ����Keeplive
    �ͻ���ģʽ
    */
    snprintf(buf, sizeof(buf), "AT+CIPSCONT=0,\"TCP\",\"%s\",%hu,2,0\r\n", 
             CONF_GetAddress(), CONF_GetPort());
	buf[sizeof(buf) - 1] = '\0';
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, (u8*)buf);
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT);    
    
    return;
}
#endif

static int _handleMsg(void)
{
	//USARTIO_SendString(GSM_USART_PORT, "Hello there!\r\n");
	
}

/* ��ѭ�� */
static void _gsmPoll(void)
{
	int ret;

    TLST_AddTimer(1000, _gsmPoll);

    switch(g_GSMStatus)
    {
        /* �豸���ϵ磬����Ƿ�������� */
        case GSM_STATUS_POWERING_ON:
        {
			/* ����豸������ϣ�������������״̬ */
            ret = _waitForStart();
			if (ERROR_SUCCESS == ret){
				g_GSMStatus = GSM_STATUS_SEARCHING;
			}
            
            break;
        }
        /* ������ϣ�����Ƿ�ע������Ӫ������ */
        case GSM_STATUS_SEARCHING:
        {
			/* ������������������������ӽ���״̬ */
            ret = _waitForConnect();
			if (ERROR_SUCCESS == ret){
				g_GSMStatus = GSM_STATUS_CONNECTING;
			}
            break;
        }
        /* ����ע������������IP���� */
        case GSM_STATUS_CONNECTING:
        {
			/* �������ӣ��������ɹ�ִ�У����������Ϣѭ�� */
			ret = _establistTCPConn();
			if (ERROR_SUCCESS == ret){
				g_GSMStatus = GSM_STATUS_READY;
			}
            break;
        }
        /* IP�����Ѿ����������ڽ�����Ϣ���� */
        case GSM_STATUS_READY:
        {
			ret = _handleMsg();
            break;
        }
        default:
        {
			g_GSMStatus = GSM_STATUS_POWERING_ON;
            break;
        }
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

    /* 100�������Ӻ������������ */
    TLST_AddTimer(100, _gsmPoll);

    /* ��������״̬ */
    g_GSMStatus = GSM_STATUS_POWERING_ON;

	return;
}

