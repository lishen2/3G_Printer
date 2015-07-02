/********************************************************************************/
/*!
	@file			mmc_stm32f1.h
	@author         Nemui Trinomius (http://nemuisan.blog.bai.ne.jp)
    @version        5.00
    @date           2013.10.16
	@brief          MMC Driver For STM32 F-1  Devices					@n
					Based on STM32F2xx_StdPeriph_Driver V1.0.0.			@n
					Based on ChaN's FatFs Sample thanks!

    @section HISTORY
		2011.10.21	V1.00 	Start Here.
		2012.01.31	V2.00 	Fixed Variable-Handling for DMA.
		2012.08.31  V3.00 	Updated Support grater than 32GB Cards.
		2013.01.30  V4.00	Fixed Minor bugs.
		2013.10.16	V5.00	Adopted FatFs0.10.

    @section LICENSE
		BSD License. See Copyright.txt
*/
/********************************************************************************/
#ifndef __MMC_STM32F1_H
#define __MMC_STM32F1_H 0x0500

#ifdef __cplusplus
 extern "C" {
#endif

/* Generic Inclusion */
#include "stm32f10x.h"
#include "diskio.h"

#define USE_SPIMMC_DMA
/*#define USE_PWRCTRL*/

#define SPIMMC						SPI1
#define SPIMMC_MAXBAUD				SPI_BaudRatePrescaler_4
#define SPIMMC_PIN_CS				GPIO_Pin_4
#define SPIMMC_PIN_SCK				GPIO_Pin_5
#define SPIMMC_PIN_MISO    			GPIO_Pin_6
#define SPIMMC_PIN_MOSI				GPIO_Pin_7

#define SPIMMC_PORT_CS				GPIOA
#define SPIMMC_PORT_SCK				GPIOA
#define SPIMMC_PORT_MISO    		GPIOA
#define SPIMMC_PORT_MOSI			GPIOA

#define SPIMMC_CLK_CS				RCC_APB2Periph_GPIOA
#define SPIMMC_CLK_SCK				RCC_APB2Periph_GPIOA
#define SPIMMC_CLK_MISO    			RCC_APB2Periph_GPIOA
#define SPIMMC_CLK_MOSI				RCC_APB2Periph_GPIOA
 
/* STM32 SPI Peripheral Settings */
#define SPI_PERIF_CLK(x)        	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1,x);
#define SPI_GPIO_CLK(x)				RCC_APB2PeriphClockCmd(SPIMMC_CLK_CS,x); 		\
									RCC_APB2PeriphClockCmd(SPIMMC_CLK_SCK,x); 		\
									RCC_APB2PeriphClockCmd(SPIMMC_CLK_MISO,x); 		\
									RCC_APB2PeriphClockCmd(SPIMMC_CLK_MOSI,x);		\
									RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,x)

#define SPI_AF_SET()				

/* STM32 SPI DMA Settings */
#define SPIMMC_DMA_STREAM_CLOCK 	RCC_AHBPeriph_DMA1
#define SPIMMC_DMA_CHANNEL_RX		DMA1_Channel2
#define SPIMMC_DMA_CHANNEL_TX		DMA1_Channel3
#define SPIMMC_DMA_STREAM_RX
#define SPIMMC_DMA_STREAM_TX
#define SPIMMC_DMA_FLAG_RX_TC       DMA1_FLAG_TC2
#define SPIMMC_DMA_FLAG_TX_TC       DMA1_FLAG_TC3
#define DMA_STREAM_IRQ           	

/* Socket Contacts is not used on this project */
#define SOCKPORT					1			/* Socket contact port 	1: NONE */
#define SOCKWP						0			/* Write protect switch	0: WP Disable */
#define SOCKINS						1			/* Card detect switch	1: SD Incerted */

/* Port Controls */
#define CS_HIGH()					SPIMMC_PORT_CS->BSRR   = SPIMMC_PIN_CS		/* MMC CS = H */
#define CS_LOW()					SPIMMC_PORT_CS->BRR    = SPIMMC_PIN_CS		/* MMC CS = L */

#define PWR_ON()
#define PWR_OFF()
#define PWR_ISON()				1

/* Function Prototypes */

u32 get_fattime(void);

DSTATUS disk_initialize (uint8_t drv);


#ifdef __cplusplus
}
#endif

#endif /*  __MMC_STM32F1_H */
