#include "stm32f10x.h"
#include "utils.h"
#include "watchdog.h"

/* ��ʼ��watchdog */
void WDG_Init(void)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); 	//��������
    IWDG_SetPrescaler(IWDG_Prescaler_64);           //�ڲ�ʱ�ӵ�Ƶ��Ϊ40khz
    IWDG_SetReload(3200);							//ԼΪ5���һ���
    IWDG_ReloadCounter();
    IWDG_Enable(); 	

    return;
}

/* ι�� */
void WDG_Reload(void)
{
    IWDG_ReloadCounter();

    return;
}

/* �Ƿ��ǹ������� */
int WDG_IsBootFromWDG(void)
{
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET){
        return BOOL_TRUE;
    } else {
        return BOOL_FALSE;
    }
}

