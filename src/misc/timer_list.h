#ifndef _TIMER_LIST_H_
#define _TIMER_LIST_H_

/* 定时器超时回调函数 */
typedef void (*TLST_CB)(void);

/* 增加单次定时器 */
int TLST_AddTimer(u32 usec, TLST_CB cb);

/* 定时器主循环 */
void TLST_Poll(void);

#endif
