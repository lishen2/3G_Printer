#ifndef _TIMER_LIST_H_
#define _TIMER_LIST_H_

/* ��ʱ����ʱ�ص����� */
typedef void (*TLST_CB)(void);

/* ���ӵ��ζ�ʱ�� */
int TLST_AddTimer(u32 usec, TLST_CB cb);

/* ��ʱ����ѭ�� */
void TLST_Poll(void);

#endif
