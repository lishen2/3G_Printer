#ifndef _GSM_HW_H_
#define _GSM_HW_H_

#define GSM_USART_PORT  USART3

enum GSMStatus{
    GSM_STATUS_POWERING_ON,  /* ִ���꿪������֮�� */
	GSM_STATUS_REGISTING,    /* ģ������ע�� */
    GSM_STATUS_CONNECTING,   /* ģ�����ڽ������� */
    GSM_STATUS_CONNECTED,    /* IP�����Ѿ����� */
    GSM_STATUS_FAILED,       /* ͨѶģ�齨������ʧ�� */
};

void GSM_Init(void);
void GSM_PowerOn(void);

/* ����ѭ����ʱ�� */
void GSM_RegStart(void);

/* ��ȡģ��״̬ */
enum GSMStatus GSM_GetStatus(void);

/* ��ȡģ��״̬�ַ��� */
const char* GSM_GetStatusStr(void);

/* ��ȡ�ź����� */
int GSM_GetSingalQuality(void);

#endif

