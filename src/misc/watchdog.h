#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

/* 初始化watchdog */
void WDG_Init(void);
void WDG_Deinit(void);

/* 喂狗 */
void WDG_Reload(void);

/* 是否是狗叫重启 */
int WDG_IsBootFromWDG(void);

#endif

