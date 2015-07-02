#include "stm32f10x.h"
#include "utils.h"
#include "watchdog.h"

/* 初始化watchdog */
void WDG_Init(void)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); 	//允许设置
    IWDG_SetPrescaler(IWDG_Prescaler_64);           //内部时钟的频率为40khz
    IWDG_SetReload(3200);							//约为5秒多一点点
    IWDG_ReloadCounter();
    IWDG_Enable(); 	

    return;
}

/* 喂狗 */
void WDG_Reload(void)
{
    IWDG_ReloadCounter();

    return;
}

/* 是否是狗叫重启 */
int WDG_IsBootFromWDG(void)
{
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET){
        return BOOL_TRUE;
    } else {
        return BOOL_FALSE;
    }
}

