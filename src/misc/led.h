#ifndef _LED_H_
#define _LED_H_

enum LED_DisplayMode{
	LED_NORMAL,
	LED_ERROR,
};

void LED_Init(void);
void LED_SetMode(enum LED_DisplayMode mode);

#endif


