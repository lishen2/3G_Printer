#ifndef _COM_MSGHANDLER_H_
#define _COM_MSGHANDLER_H_

enum COM_ReciveResault{
    COM_REC_RESAULT_OK,        //��ʾ��ǰ�Ѿ����ݲ�����
    COM_REC_RESAULT_NONE,      //��ʾ��ǰû���յ�����
    COM_REC_RESAULT_HEARTBEAT, //��ʾ��ȡ����������
};

enum COM_ReciveResault COM_HandleMessage(void);

#endif
