#ifndef _COM_UTILS_H_
#define _COM_UTILS_H_

/* ����ѭ���Ƿ���� */
enum CTRL_ParserCtrl{
    CTRL_CONTINUE,
    CTRL_BREAK,
};

/* ��ʱ����ʱ�ص����� */
typedef int (*CTRL_ReadCB)(char *, void* buf);

/* ���ĵ������е����� */
void CUTILS_ConsumeBuf(void* port);

/* �ȴ�ģ�鷵��OK����ERROR */
int CUTILS_WaitForResault(void *port, 
                          int delay_time, 
                          int delay_count);

/* �ȴ��ض���Ϣ
   �����ȡ���ɹ���Ϣ���سɹ���
   ��ȡ��������Ϣ��ȴ���ʱ��������ʧ��
*/
int CUTILS_WaitForMessage(void *port, 
                          char* successMSG,
                          char* failedMSG,
                          int delay_time, 
                          int delay_count);

/* ������Ϣ
@param port �˿�
@param buf  ���ݻ�����
@param bufsize ���ݻ�������С
@param cb   ������Ϣʱ�Ļص�����
@param userdata ͸�����ݵ��ص�������
@param delay_time ��ȡʧ�ܺ�ȴ���ʱ��
@param delay_count ��ȡʧ�ܺ����ԵĴ���
�ܵĵȴ�ʱ��Ϊdelay_time * delay_count
*/
int CUTILS_ReadAndParserMsg(void* port, 
							char* buf,
							int bufsize,
							CTRL_ReadCB cb,
							void* userdata,
							int delay_time, 
							int delay_count);

/* �����ݷ�������������SD�� */
void COM_SendAndSave(char* buf, int len);

#endif

