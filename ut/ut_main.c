#include <stdio.h>
#include "stub.h"

extern void _handlerMsg(void);

static void test_msgHandler(void)
{
	int i;
	
	STUB_OpenUsartDataFile("msg_test_data.txt");

	for(i = 0; i < 50; ++i){
		_handlerMsg();
	}

	STUB_CloseUsartDataFile();

	return;
}

int main()
{
	test_msgHandler();

	return 0;
}

