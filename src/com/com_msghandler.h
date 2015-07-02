#ifndef _COM_MSGHANDLER_H_
#define _COM_MSGHANDLER_H_

enum COM_ReciveResault{
    COM_REC_RESAULT_OK,        //表示当前已经数据并处理
    COM_REC_RESAULT_NONE,      //表示当前没有收到数据
    COM_REC_RESAULT_HEARTBEAT, //表示读取到心跳数据
};

enum COM_ReciveResault COM_HandleMessage(void);

#endif
