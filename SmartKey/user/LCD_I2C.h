#ifndef		__LCD_H__
#define		__LCD_H__

/*******************************    INCLUDES   ********************************/
//#include "main.h"
#include "stm32f1xx_hal.h"
/***************************    GLOBAL VARIABLES   ****************************/

extern struct FEE_def	FEE;
extern I2C_HandleTypeDef hi2c1;  // change your handler here accordingly

/***************************       FUNCTIONS       ****************************/

void lcd_init (void);   // initialize lcd

void lcd_send_cmd (char cmd);  // send command to the lcd

void lcd_send_data (char data);  // send data to the lcd

void lcd_send_string (char *str);  // send string to the lcd

void lcd_put_cur(int row, int col);  // put cursor at the entered position row (0 or 1), col (0-15);

void lcd_clear (void);

void LCD_Innit(void);

#endif

