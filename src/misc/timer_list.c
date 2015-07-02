#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"
#include "utils.h"
#include "timer_list.h"
#include "assert.h"

#define TLST_MAX_LIST_LENGTH     8

struct TLSTCtlBlk{
    u32 execTime;  //�ص�ִ��ʱ�� 
    TLST_CB cb;  
};

/* ��ִ�е������б� */
static struct TLSTCtlBlk g_List[TLST_MAX_LIST_LENGTH] = {0};

/* ��ʱ�����������Ӷ�ʱ���ص� */
static int _tlstAddTimer(struct TLSTCtlBlk* lst, u32 usec10, TLST_CB cb)
{
    int i;
    struct TLSTCtlBlk* ctlBlk = NULL;

    for (i = 0; i < TLST_MAX_LIST_LENGTH; ++i){
        if (NULL == (lst + i)->cb){
            ctlBlk = lst + i;
            break;
        }
    }

    if (NULL == ctlBlk){
        printf("BUG report: timer list full.\r\n");
		ASSERT(0);
        return ERROR_FAILED;
    } 

    ctlBlk->execTime = g_jiffies + usec10;
    ctlBlk->cb = cb;

    return ERROR_SUCCESS;
}

/* ���ӵ��ζ�ʱ������ͨ���ȼ� */
int TLST_AddTimer(u32 usec, TLST_CB cb)
{
    return _tlstAddTimer(g_List, usec/10, cb);
}

/* ��ʱ����ѭ�� */
void TLST_Poll(void)
{
    int i;
    TLST_CB cb;
    struct TLSTCtlBlk* ctlBlk;

    for (i = 0; i < TLST_MAX_LIST_LENGTH; ++i){
        ctlBlk = g_List + i;
        
        if (NULL != ctlBlk->cb && 
            time_after(g_jiffies, ctlBlk->execTime)){
            cb = ctlBlk->cb;
            
            /* ��������ݺ�ִ�лص����ص��п����ٴ���Ӷ�ʱ�� */
            memset(ctlBlk, 0, sizeof(struct TLSTCtlBlk));
            
            cb();
        } 
    }

    return;
}


