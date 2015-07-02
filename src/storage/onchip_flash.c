#include <string.h>
#include "stm32f10x.h"
#include "onchip_flash.h"
#include "utils.h"
#include "assert.h"

#if defined (STM32F10X_HD) || defined (STM32F10X_HD_VL) || defined (STM32F10X_CL) || defined (STM32F10X_XL)
  #define FLASH_PAGE_SIZE    ((uint16_t)0x800)
#else
  #define FLASH_PAGE_SIZE    ((uint16_t)0x400)
#endif

/* stm32f103带了64kb的flash，
   分为两部分60KB用于存放程序，
   后面4KB用于存放配置数据 */
#define FLASH_DATA_LENGTH         0x1000   

/* flash中用于存放数据的位置, START_ADDRESS + 60KB */
#define FLASH_DATA_START_ADDRESS  (0x8000000 + 0xF000)

/* 从片内flash中读取数据 */
int FLASH_Read(void* buf, int bufsize){
	
	ASSERT(bufsize <= FLASH_DATA_LENGTH);

	memcpy(buf, (void*)FLASH_DATA_START_ADDRESS, bufsize);

	return ERROR_SUCCESS;
}

/* 将配置写入片内flash */
int FLASH_Write(void* buf, int bufsize)
{
	int i, retry;
	FLASH_Status status;
	u32 des_addr, src_addr;

	int ret = ERROR_SUCCESS;

	ASSERT(bufsize <= FLASH_DATA_LENGTH);

    /* 解锁flash */
    FLASH_UnlockBank1();

	/* 擦除页 */
	i = 0;
	retry = 0;
	while (1){
		des_addr = FLASH_DATA_START_ADDRESS + (FLASH_PAGE_SIZE * i);	

		status = FLASH_ErasePage(des_addr);
		if (FLASH_COMPLETE == status){
			i++;
			retry = 0;
		} else {
		 	retry++;
		}

		if (retry > 5 || FLASH_PAGE_SIZE * i >= bufsize){
			break;
		}
	}

	if (retry > 5){
		ret = ERROR_FAILED;
	}

	/* 写入数据
	   原数据不为字对齐时会写超，暂不处理，原因如下：
	   1、将一点无用数据写入falsh不会有不良影响，读取的时候是按照字节读取的，多写入的数据不会被读出；
	   2、STM32没有内存访问越界检查，所以越界访问内存应该不会造成问题
	   3、就在flash中保存一个数据
	 */
	retry = 0;
	des_addr = FLASH_DATA_START_ADDRESS;
	src_addr = (u32)buf;
	while(1){
		status = FLASH_ProgramWord(des_addr, *((u32*)src_addr));
		if (FLASH_COMPLETE == status){
			src_addr += 4;
			des_addr += 4;
			retry = 0;
		} else {
		 	retry++;
		}
		
		if (retry > 5 || src_addr >= ((u32)buf + bufsize)){
			break;
		}	
	}

	if (retry > 5){
		ret = ERROR_FAILED;
	}

	/* 重新锁定 */
	FLASH_LockBank1();

	return ret;
}

