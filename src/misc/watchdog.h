#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

/* ��ʼ��watchdog */
void WDG_Init(void);
void WDG_Deinit(void);

/* ι�� */
void WDG_Reload(void);

/* �Ƿ��ǹ������� */
int WDG_IsBootFromWDG(void);

#endif

