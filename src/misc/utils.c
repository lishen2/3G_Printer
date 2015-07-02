#include <stdio.h>
#include "stm32f10x.h"
#include "utils.h"

vu32 g_jiffies = 0;

/* 设置寄存器，准备进入休眠状态 */
static inline void PWR_PrepareSleep(u32 SysCtrl_Set)
{
    if (SysCtrl_Set)
        SCB->SCR |= SCB_SCR_SLEEPONEXIT;   // Set SLEEPONEXIT                       
    else
        SCB->SCR &= ~SCB_SCR_SLEEPONEXIT;  // Reset SLEEPONEXIT

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP;        // Clear SLEEPDEEP bit
}

/* 毫秒级延时 */
void delay_ms(int n)
{
	n = n/10;

    PWR_PrepareSleep(0);
    
    n += g_jiffies;
    while(time_after(n, g_jiffies)){
//        __WFI();
    }
    
    return;
}

void HW_InitSysTick(void)
{
    /* Setup SysTick Timer for 10 msec interrupts  */
    if (SysTick_Config(SystemCoreClock / 100)) 
    { 
        /* Capture error */ 
        while (1);
    }  
    /* Configure the SysTick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00);

    return;
}

void HW_CommonInit(void)
{   
    /* set 2 bit preemptible */  
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);    

	return;
}

/* 将任意内容转化为ASCII */
void UTILS_ConvByteToASCII(char* inbuf, int inbufsize, char* outbuf, int outbufsize)
{
	unsigned i;

	if (outbufsize >= ((inbufsize<<1) - 1)) {
		for (i = 0; i < inbufsize; ++i){
			outbuf[(i<<1) + 0] = UTILS_Num2Char((inbuf[i] >> 4) & 0x0F);
			outbuf[(i<<1) + 1] = UTILS_Num2Char((inbuf[i] >> 0) & 0x0F);
		}
	}

	return;
}


/* 获取设备ID */
static void _genDeviceID(char* buf, int bufsize)
{
#define         ID1          (0x1FFFF7E8)
#define         ID2          (0x1FFFF7EC)
#define         ID3          (0x1FFFF7F0)

    u32 serialNum[2];
        
    serialNum[0] = *(u32*)ID1;
    serialNum[1] = *(u32*)ID2;
    serialNum[0] ^= (*(u32*)ID3);  

    buf[0] = UTILS_Num2Char((serialNum[0] >> 0) & 0x0F);
    buf[1] = UTILS_Num2Char((serialNum[0] >> 4) & 0x0F);
    buf[2] = UTILS_Num2Char((serialNum[0] >> 8) & 0x0F);
    buf[3] = UTILS_Num2Char((serialNum[0] >> 12) & 0x0F);
    buf[4] = UTILS_Num2Char((serialNum[0] >> 16) & 0x0F);
    buf[5] = UTILS_Num2Char((serialNum[0] >> 20) & 0x0F);
    buf[6] = UTILS_Num2Char((serialNum[0] >> 24) & 0x0F);
    buf[7] = UTILS_Num2Char((serialNum[0] >> 28) & 0x0F);

    buf[8] = UTILS_Num2Char((serialNum[1] >> 0) & 0x0F);
    buf[9] = UTILS_Num2Char((serialNum[1] >> 4) & 0x0F);
    buf[10] = UTILS_Num2Char((serialNum[1] >> 8) & 0x0F);
    buf[11] = UTILS_Num2Char((serialNum[1] >> 12) & 0x0F);
    buf[12] = UTILS_Num2Char((serialNum[1] >> 16) & 0x0F);
    buf[13] = UTILS_Num2Char((serialNum[1] >> 20) & 0x0F);
    buf[14] = UTILS_Num2Char((serialNum[1] >> 24) & 0x0F);
    buf[15] = UTILS_Num2Char((serialNum[1] >> 28) & 0x0F);

    buf[bufsize - 1] = '\0';

    return;
}

char* UTILS_GetDeviceID(void)
{
	static char buf[17];

	_genDeviceID(buf, sizeof(buf));

	return buf;
}

static char g_versionBuf[64];

#define UTILS_VERSION_MAJOR   1
#define UTILS_VERSION_MINOR   1
const char* UTILS_GetVersion(void)
{
    int retsize;
    
    retsize = snprintf(g_versionBuf, sizeof(g_versionBuf), "Ver:V%d.%d\n"
                       "Build Time:"
                       "%s "
                       "%s", 
                       UTILS_VERSION_MAJOR,
                       UTILS_VERSION_MINOR,
                       __DATE__, 
                       __TIME__);

    /* 保证字符串结尾永远有一个结束符 */
    if (retsize == sizeof(g_versionBuf)){
        g_versionBuf[sizeof(g_versionBuf) - 1] = '\0';
    }

    return g_versionBuf;
}

