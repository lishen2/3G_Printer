#ifndef _CONFIG_H_
#define _CONFIG_H_

/* 配置文件中各字段长度 */
#define CONF_APN_LENGTH         16
#define CONF_ADDRESS_LENGTH     64

/* 初始化配置 */
void CONF_InitConfig(void);

/* 配置是否更新 */
int CONF_IsUpdated(void);

/* 读取数据 */
char* CONF_GetAddress(void);
u16 CONF_GetPort(void);
char* CONF_GetAPN(void);

#endif

