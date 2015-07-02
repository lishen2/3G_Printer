#ifndef _COM_UTILS_H_
#define _COM_UTILS_H_

/* 控制循环是否继续 */
enum CTRL_ParserCtrl{
    CTRL_CONTINUE,
    CTRL_BREAK,
};

/* 定时器超时回调函数 */
typedef int (*CTRL_ReadCB)(char *, void* buf);

/* 消耗掉缓存中的数据 */
void CUTILS_ConsumeBuf(void* port);

/* 等待模块返回OK或者ERROR */
int CUTILS_WaitForResault(void *port, 
                          int delay_time, 
                          int delay_count);

/* 等待特定消息
   如果读取到成功消息返回成功，
   读取到错误消息或等待超时，都返回失败
*/
int CUTILS_WaitForMessage(void *port, 
                          char* successMSG,
                          char* failedMSG,
                          int delay_time, 
                          int delay_count);

/* 解析消息
@param port 端口
@param buf  数据缓冲区
@param bufsize 数据缓冲区大小
@param cb   读到消息时的回调函数
@param userdata 透明传递到回调函数中
@param delay_time 读取失败后等待的时间
@param delay_count 读取失败后重试的次数
总的等待时间为delay_time * delay_count
*/
int CUTILS_ReadAndParserMsg(void* port, 
							char* buf,
							int bufsize,
							CTRL_ReadCB cb,
							void* userdata,
							int delay_time, 
							int delay_count);

/* 将数据发送蓝牙并保存SD卡 */
void COM_SendAndSave(char* buf, int len);

#endif

