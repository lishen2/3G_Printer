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

/* GSM模块的注册状态 */
enum GSMRegStatus{
	GSM_REG_STATUS_NOTEVENTRY       = 0,
	GSM_REG_STATUS_REGISTED_LOCAL   = 1,
	GSM_REG_STATUS_SEARCHING        = 2,
	GSM_REG_STATUS_REFUSED          = 3,
	GSM_REG_STATUS_UNKONWN          = 4,
	GSM_REG_STATUS_REGISTED_ROAMING = 5,
	GSM_REG_STATUS_READERROR,
};

/* 模块状态 */
static int g_GSMStatus = GSM_STATUS_POWER_OFF;

/* 模块向移动网络的注册状态 */
static int g_RegCode = GSM_REG_STATUS_READERROR;

/* 消息缓冲区 */
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

/* 检测是否完成启动 */
static int _waitForStart(void)
{
    int ret;
	
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

    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

        /* 启用硬件流控 */
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

/* 解析AT+CREG命令
AT+CREG?

+CREG: 0,1
*/
static int _parserCREG(char* buf, int* pstatus)
{
	char* cur;
	int status;

	/* 查找起始标记 */
	cur = strstr(buf, "+CREG:");
	if (NULL == cur){
		return CTRL_CONTINUE;
	}

	/* 查找状态 */
	cur = strchr(buf, ',');
	if (NULL == cur){
		return CTRL_CONTINUE;
	}
	cur += 1;
	status = atoi(cur);
	*pstatus = status;
	
	return CTRL_BREAK;
}

/* 等待设备注册网络 */
static int _waitForConnect(void)
{
    int ret;

    /* 检测是否正常启动 */
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

	/* 如果读取不成功，或者状态不为已注册，直接返回失败，等待下一次尝试 */
    if (ERROR_SUCCESS != ret || 
		((GSM_REG_STATUS_REGISTED_LOCAL != g_RegCode) &&
		 (GSM_REG_STATUS_REGISTED_ROAMING != g_RegCode))){
		return ERROR_FAILED;
    }

    /* 设置GPRS进行附着 */
	USARTIO_SendString(GSM_USART_PORT, "AT+CGATT=1\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);  

	/* 检查GPRS是否连接 */
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
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPMODE=0,1\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);  
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* 设置心跳时间 */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=0,40,0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* 设置启动注册包
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
	
    /* 设置心跳包
    心跳包内容为:大写字母B */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPPACK=0,\"24A0\"\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

	/* 建立连接
    类型为TCP
    域名或IP
    端口
    发送Keeplive
    客户端模式
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

	/* 这里命令执行成功即认为成功，如果网络不好，通信模块会不断尝试重连 */
    return ERROR_SUCCESS;
}

#ifdef 0
/* 更新配置 */
static void _updateConfig(void)
{
    

     /* 取消自动透传 */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=0,40,0\r\n");
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT);  


    /* 配置APN */
    snprintf(buf, sizeof(buf), "AT+CSTT=\"%s\"\r\n", CONF_GetAPN());
	buf[sizeof(buf) - 1] = '\0';
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, (u8*)buf);
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT);     

    /* 配置透传参数
    
    自动进入透传模式
    心跳发送间隔40秒
    是否以文本发送
    发送数据延时10毫秒
    收到内容满180字符后立刻发送
    */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=1,40,0,10,180\r\n");
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT); 

    /* 设置启动注册包
	CREG:SERIAL_NUMBER
	 */
    snprintf(buf, sizeof(buf), "AT+CIPPACK=1,\"255474A3%sD0A0\"\r\n", UTILS_GetDeviceID());
	buf[sizeof(buf) - 1] = '\0';
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, (u8*)buf);
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT); 
    
    /* 设置心跳包
    心跳包内容为:大写字母B */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPPACK=0,\"24D0A0\"\r\n");
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT); 

    /* 设置单链接 */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPMUX=0\r\n");
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT);    

    /* 设置透传模式 */
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPMODE=1,1\r\n");
    CUTILS_WaitForResault(GSM_USART_PORT, 
                          GSM_WAIT_RESPONSE_TIMEOUT, 
                          GSM_WAIT_RESPONSE_COUNT);  

    /* 保存连接配置
    连接ID为0
    类型为TCP
    域名或IP
    端口
    发送Keeplive
    客户端模式
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

/* 主循环 */
static void _gsmPoll(void)
{
	int ret;

    TLST_AddTimer(1000, _gsmPoll);

    switch(g_GSMStatus)
    {
        /* 设备刚上电，检测是否启动完毕 */
        case GSM_STATUS_POWERING_ON:
        {
			/* 如果设备启动完毕，进入网络搜索状态 */
            ret = _waitForStart();
			if (ERROR_SUCCESS == ret){
				g_GSMStatus = GSM_STATUS_SEARCHING;
			}
            
            break;
        }
        /* 启动完毕，检查是否注册上运营商网络 */
        case GSM_STATUS_SEARCHING:
        {
			/* 如果网络连接正常，进入连接建立状态 */
            ret = _waitForConnect();
			if (ERROR_SUCCESS == ret){
				g_GSMStatus = GSM_STATUS_CONNECTING;
			}
            break;
        }
        /* 网络注册正常，建立IP连接 */
        case GSM_STATUS_CONNECTING:
        {
			/* 建立连接，如果命令成功执行，则进入主消息循环 */
			ret = _establistTCPConn();
			if (ERROR_SUCCESS == ret){
				g_GSMStatus = GSM_STATUS_READY;
			}
            break;
        }
        /* IP连接已经建立，正在进行消息处理 */
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
    /* 如果没有开机先进行开机 */
	if (Bit_RESET == GPIO_ReadInputDataBit(GSM_POWERSTATUS_PORT, GSM_POWERSTATUS_PIN)){

        /* 按键拉低，定时器关闭 */
		GPIO_SetBits(GSM_POWERKEY_PORT, GSM_POWERKEY_PIN);
        delay_ms(1500);
        GPIO_ResetBits(GSM_POWERKEY_PORT, GSM_POWERKEY_PIN); 
	} 

    /* 100毫秒秒钟后进行启动流程 */
    TLST_AddTimer(100, _gsmPoll);

    /* 设置启动状态 */
    g_GSMStatus = GSM_STATUS_POWERING_ON;

	return;
}

