#ifndef _GSM_HW_H_
#define _GSM_HW_H_

#define GSM_USART_PORT  USART3

enum GSMStatus{
    GSM_STATUS_POWERING_ON,  /* 执行完开机命令之后 */
	GSM_STATUS_REGISTING,    /* 模块正在注册 */
    GSM_STATUS_CONNECTING,   /* 模块正在建立连接 */
    GSM_STATUS_CONNECTED,    /* IP连接已经建立 */
    GSM_STATUS_FAILED,       /* 通讯模块建立连接失败 */
};

void GSM_Init(void);
void GSM_PowerOn(void);

/* 启动循环定时器 */
void GSM_RegStart(void);

/* 获取模块状态 */
enum GSMStatus GSM_GetStatus(void);

/* 获取模块状态字符串 */
const char* GSM_GetStatusStr(void);

/* 获取信号质量 */
int GSM_GetSingalQuality(void);

#endif

