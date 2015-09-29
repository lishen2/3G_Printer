#include "stm32f10x.h"
#include "utils.h"
#include "ringbuf.h"
#include "printer.h"
#include "usart_io.h"
#include "timer_list.h"
#include "com_gsm.h"
#include "com_heartbeat.h"

#define COM_HEARTBEAT_INTERVAL (10 * 1000)

static void _heartBeat(void)
{
	enum GSMStatus net_status;
	u8   prn_status;

    TLST_AddTimer(COM_HEARTBEAT_INTERVAL, _heartBeat);

	net_status = GSM_GetStatus();
	if (GSM_STATUS_CONNECTING == net_status ||
		GSM_STATUS_CONNECTED == net_status){

		prn_status = PRINT_GetPrintStatus();
		//成功读取到状态且不缺纸，返回状态0
		if ((0 != prn_status) && (0 == (PRINT_STATUS_N1_OUTLINE & prn_status))){
			USARTIO_SendString(GSM_USART_PORT, "+STATUS:0\n");
		}
		//否则返回状态1
		else {
			USARTIO_SendString(GSM_USART_PORT, "+STATUS:1\n");
		}
	}

	return;
}

void COM_RegHeartBeat(void)
{
    /* 首次心跳40后发送 */
    TLST_AddTimer(COM_HEARTBEAT_INTERVAL, _heartBeat);

	return;
}
