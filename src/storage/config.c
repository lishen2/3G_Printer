#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f10x.h"
#include "utils.h"
#include "config.h"
#include "conf_parser.h"
#include "ff.h"
#include "assert.h"
#include "onchip_flash.h"

#define CONF_CONFIG_FLAG  0x4321 

#define CONF_CONFIG_FILENAME  "config.ini"

/* 配置文件中的配置信息名称 */
#define CONF_KEY_APN          "ACCESS_POINT_NAME"
#define CONF_KEY_ADDRESS      "SERVER_ADDRESS"
#define CONF_KEY_PORT         "SERVER_PORT"

/* 配置文件结构 */
struct CONFDeviceConfig{
	u16 flag;
	u16 port;    
    char APN[CONF_APN_LENGTH + 1];
	char address[CONF_ADDRESS_LENGTH + 1];
};

struct CONFDeviceConfig g_config;
static int g_configUpdated = BOOL_FALSE;

/* 全局文件系统 */
static FATFS g_fs;

/* 配置文件 */
static FIL g_configFile;

static void _handleKeyValue(char* key, char* value)
{
    if (0 == strcmp(CONF_KEY_APN, key)){
        
        strncpy(g_config.APN, value, sizeof(g_config.APN));
        g_config.APN[sizeof(g_config.APN) - 1] = '\0';
        
    } else if (0 == strcmp(CONF_KEY_ADDRESS, key)) {

        strncpy(g_config.address, value, sizeof(g_config.address));
        g_config.address[sizeof(g_config.address) - 1] = '\0';    
		      
    } else if (0 == strcmp(CONF_KEY_PORT, key)) {
    
        g_config.port = atoi(value);
  
	}

    return;
}

static int _openAndRead(void)
{
   	FRESULT res;
    char buf[32];
    u32 readSize;

    /* 打开配置文件 */    
    res = f_open(&g_configFile, CONF_CONFIG_FILENAME, FA_READ);

    if (FR_OK != res){
        return ERROR_FAILED;
    }

    /* 读取文件 */
    while(1){
        res = f_read(&g_configFile, buf, sizeof(buf), &readSize);

        if (FR_OK == res){
            CONF_ParserBuf(buf, readSize);
        } 

        if (FR_OK != res || readSize < sizeof(buf)){
            break;
        }
    }
    
    /* 关闭文件 */
    f_close(&g_configFile);

    return ERROR_SUCCESS;
}

/* 尝试打开配置文件，成功返回ERROR_SUCCESS，失败返回ERROR_FAILED */
static int _tryReadConfigFile(void)
{
	int ret = ERROR_FAILED;
	FRESULT res;
    
	/* 先清空配置 */
	memset(&g_config, 0, sizeof(g_config));

	/* 设置配置文件处理函数 */
	CONF_SetHandler(_handleKeyValue);

	/* 挂载文件系统 */
	res = f_mount(&g_fs, "", 1);    
	if (FR_OK == res){
	
		/* 尝试打开并读取文件 */
		ret = _openAndRead();

		f_mount(NULL, "", 0);
	}
	
	return ret;
}

/* 检查配置是否合法 */
static int _isConfigValid(struct CONFDeviceConfig* config)
{
	/* 检查端口范围 */
	if (0 == config->port) {
		return ERROR_FAILED;
	}

	/* 地址不能为空 */
	if ('\0' == config->address[0]){
		return ERROR_FAILED;
	}

	return ERROR_SUCCESS;
}

/* 配置文件模块初始化
TODO: 可以优化直接使用flash，节省内存
 */
void CONF_InitConfig(void)
{
	int ret;

    /* 先将更新标记设置为FALSE */
    g_configUpdated = BOOL_FALSE;

	/* 尝试读取sd卡中的配置文件 */
	ret = _tryReadConfigFile();

	/* 如果成功读取 */
	if (ERROR_SUCCESS == ret){			

		printf("Read config APN[%s] address[%s] port[%hu]\r\n", 
				g_config.APN,
				g_config.address,
				g_config.port);

		/* 检查配置合法性 */
		ret = _isConfigValid(&g_config);
		if (ERROR_SUCCESS == ret){

			/* 将配置更新flash */
	        g_config.flag = CONF_CONFIG_FLAG;            
			FLASH_Write(&g_config, sizeof(g_config));

            /* 标记配置已经更新 */
            g_configUpdated = BOOL_TRUE;
			printf("Config is updated.\r\n");
		}
	}

    /* 如果没有配置更新，则从flash中恢复数据 */
    if (BOOL_FALSE == g_configUpdated){
        
    	/* 从flash读取配置 */
    	FLASH_Read(&g_config, sizeof(g_config));

    	/* 如果是首次启动，将配置初始化为0 */
    	if (CONF_CONFIG_FLAG != g_config.flag){
    		memset(&g_config, 0, sizeof(g_config));
    	}
    }
    
	return;
}

/* 配置是否更新 */
int CONF_IsUpdated(void)
{
	return g_configUpdated;
}

/* 读取数据 */
char* CONF_GetAddress(void)
{
	return g_config.address;
}

u16 CONF_GetPort(void)
{
	return g_config.port;
}

char* CONF_GetAPN(void)
{
	return g_config.APN;
}


