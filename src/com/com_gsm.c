/*********************************
本文件用来将配置GSM模块，并建立连接
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

/* 等待模块返回数据的时间，每次等待100毫秒，等待50次，即最多等待5秒钟 */
#define GSM_WAIT_RESPONSE_TIMEOUT    100
#define GSM_WAIT_RESPONSE_COUNT      50

/* 两次收到数据之间的最大间隔，超过次间隔则认为和服务器断开 */
#define GSM_MAX_RECIVE_INTERVAL   60 * HZ

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
static enum GSMStatus g_GSMStatus = GSM_STATUS_POWERING_ON;

/* 重置模块时间
   在模块出错时，可能需要重置模块，此变量记录重置模块的时间戳 */
static unsigned int g_resetTimestamp = 0;
static int g_needReset = BOOL_FALSE;

/* 模块错误状态代码 */
enum GSMMsgCode{
    GSM_MSG_MODULE_STARTING = 0,
	GSM_MSG_MODULE_NOT_RESPONSE,
	GSM_MSG_MODULE_SEARCHING,
	GSM_MSG_MOBILE_REGIST_FAILED,
	GSM_MSG_IP_NETWORK_CONNECTING,
	GSM_MSG_IP_NETWORK_CONNECTED,
	GSM_MSG_MAX,
};

/* 模块错误信息，和状态代码一一对应 */
static const char* g_msgStrings[] = {
    "通信模块正在启动",
	"通信模块无响应",
	"正在搜寻移动通信网络",
	"移动通信网络连接失败，请检查SIM卡是否欠费或停机",
	"正在连接服务器",
	"已经连接服务器",
	""
};

/* 当前模块的状态字符换代码 */
static int g_msgCode = GSM_MSG_MAX;

static char g_msgBuf[64];

/* 信号强度 */
static int g_signalQuality = -1;

/* 上次收到数据时的时间戳 */
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

/* 设置模块在指定毫秒后重启 */
static void _gsmSetReset(unsigned int msec)
{
	printf("Module not response, reset after %d ms.\n", msec);

	g_resetTimestamp = g_jiffies + (msec/10);
	g_needReset = BOOL_TRUE;

	return;
}

/* 清楚模块重启 */
static void _gsmClearReset(void)
{
	g_needReset = BOOL_FALSE;
}

/* 执行模块重启 */
static void _gsmDoReset(void)
{
	printf("Reset module\n");

	GPIO_ResetBits(GSM_RESET_PORT, GSM_RESET_PIN);
	delay_ms(200);
	GPIO_SetBits(GSM_RESET_PORT, GSM_RESET_PIN);

	return;
}

/* 尝试执行重启，成功返回ERROR_SUCCESS，失败返回ERROR_FAILED */
static int _gsmTryReset(void)
{
	int ret;
	
	/* 如果成功完成重启，则将模块状态重置 */
	if (BOOL_TRUE == g_needReset && time_after(g_jiffies, g_resetTimestamp)){
		
		_gsmDoReset();
		_gsmClearReset();

		ret = ERROR_SUCCESS;
		
	} else {
	
		ret = ERROR_FAILED;
	}

	return ret;
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

static int _basicSetup(void)
{
    int ret;
    
    /* 启用硬件流控 */
	/*
    CUTILS_ConsumeBuf(GSM_USART_PORT);    
    USARTIO_SendString(GSM_USART_PORT, "AT+CIFC=0,0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT);   
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }*/

    /* 关闭回显 */
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

/* 解析AT+CREG命令
AT+CREG?

+CREG: 0,1
*/
static int _parserCREG(char* buf, void* pstatus)
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
	*(int*)pstatus = status;
	
	return CTRL_BREAK;
}

/* 读取注册号码 */
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

/* 解析AT+CSQ命令
AT+CSQ

+CSQ: 16,0

OK
*/
static int _parserCSQ(char* buf, void* pquality)
{
	char* cur;
	int quality;

	/* 查找起始标记 */
	cur = strstr(buf, "+CSQ:");
	if (NULL == cur){
		return CTRL_CONTINUE;
	}

	/* 查找状态 */
	cur = strchr(buf, ' ');
	if (NULL == cur){
		return CTRL_CONTINUE;
	}
	cur += 1;
	quality = atoi(cur);
	*(int*)pquality = quality;
	
	return CTRL_BREAK;
}

/* 读取信号强度 */
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
    
    不自动建立连接
    心跳发送间隔40秒
    是否以文本发送
    */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPCFG=0,50,0,0,0,0\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
    if (ERROR_SUCCESS != ret){
        return ERROR_FAILED;
    }

    /* 设置心跳包
    心跳包内容为: 为@#^ */
    CUTILS_ConsumeBuf(GSM_USART_PORT); 
    USARTIO_SendString(GSM_USART_PORT, "AT+CIPPACK=0,\"@#^\"\r\n");
    ret = CUTILS_WaitForResault(GSM_USART_PORT, 
                                GSM_WAIT_RESPONSE_TIMEOUT, 
                                GSM_WAIT_RESPONSE_COUNT); 
	if (ERROR_SUCCESS != ret){
		return ERROR_FAILED;
	}

    /* 设置启动注册包
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

    /* 保存连接配置
    类型为TCP
    域名或IP
    端口
    自动重连，但不发送Keeplive
    客户端模式
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

	/* 这里命令执行成功即认为成功，如果网络不好，通信模块会不断尝试重连 */
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
			/* 等待模块启动 */
			ret = _waitForStart();

			if (ERROR_SUCCESS == ret){

                /* 启用硬件流控，设置不回显 */
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
            /* 读取信号强度 */
            g_signalQuality = _retReadSignalQuality();  
			
            /* 读取注册状态 */
            regCode = _readRegCode();  

			/* 如果成功注册 */
			if (GSM_REG_STATUS_REGISTED_LOCAL == regCode ||
				GSM_REG_STATUS_REGISTED_ROAMING == regCode){

                /* 建立IP连接 */
				ret = _gsmEstablishIPConn();
                if (ERROR_SUCCESS == ret){
                    g_GSMStatus = GSM_STATUS_CONNECTING;
                    g_msgCode = GSM_MSG_IP_NETWORK_CONNECTING;
                }			
			}
			/* 如果注册被拒绝或没有尝试注册 */
            else if (GSM_REG_STATUS_NOTEVENTRY == regCode || 
				     GSM_REG_STATUS_REFUSED == regCode){

				g_msgCode = GSM_MSG_MOBILE_REGIST_FAILED;
            }
            /* GSM_REG_STATUS_SEARCHING, 
			   GSM_REG_STATUS_UNKONWN, 
			   GSM_REG_STATUS_READERROR
			   这几种状态我们认为是正在注册
			   */
			else {
                g_msgCode = GSM_MSG_MODULE_SEARCHING;
            }        

            delay_ms(1000);
            
            break;
        }
        /* 一旦开始建立连接，则进入透传模式，此时直接调用消息处理函数进行读取和处理 */
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
		/* 如果进入错误分支，则尝试重启 */
		case GSM_STATUS_FAILED:
        default:
        {
			/* 重启条件满足，完成重启后，重置状态机 */
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
    /* 如果没有开机先进行开机 */
	if (Bit_RESET == GPIO_ReadInputDataBit(GSM_POWERSTATUS_PORT, GSM_POWERSTATUS_PIN)){

        /* 按键拉低，定时器关闭 */
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
    /* 100毫秒秒钟后执行启动流程 */
    TLST_AddTimer(100, _gsmPoll);
    
    return;
}

enum GSMStatus GSM_GetStatus(void)
{
    return g_GSMStatus;
}

/* 获取模块状态字符串 */
const char* GSM_GetStatusStr(void)
{
	if (g_msgCode <= GSM_MSG_MAX){
		return g_msgStrings[g_msgCode];
	} else {
		return "";
	}
}

/* 获取信号质量 */
int GSM_GetSingalQuality(void)
{
    return g_signalQuality;
}


