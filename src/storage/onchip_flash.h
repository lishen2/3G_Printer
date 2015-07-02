#ifndef _ONCHIP_FLASH_H_
#define _ONCHIP_FLASH_H_

/* 从片内flash中读取数据 */
int FLASH_Read(void* buf, int bufsize);

/* 将配置写入片内flash */
int FLASH_Write(void* buf, int bufsize);

#endif

