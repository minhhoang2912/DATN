#include "user.h"
#include "main.h"
#include "LCD_I2C.h"
#include "RFID.h"
#include "flash.h"
#include "stdbool.h"
#include <stdio.h>

#define ROW_1_Pin R1_Pin
#define ROW_2_Pin R2_Pin
#define ROW_3_Pin R3_Pin
#define ROW_4_Pin R4_Pin
#define COL_1_Pin C1_Pin
#define COL_2_Pin C2_Pin
#define COL_3_Pin C3_Pin
#define COL_4_Pin C4_Pin
GPIO_TypeDef* ROW_1_Port = GPIOB;
GPIO_TypeDef* ROW_2_Port = GPIOB;
GPIO_TypeDef* ROW_3_Port = GPIOB;
GPIO_TypeDef* ROW_4_Port = GPIOB;
GPIO_TypeDef* COL_1_Port = GPIOA;
GPIO_TypeDef* COL_2_Port = GPIOA;
GPIO_TypeDef* COL_3_Port = GPIOA;
GPIO_TypeDef* COL_4_Port = GPIOA;

#define SAVE_CARD1_ADDR	((uint32_t)0x0801FC00)
#define SAVE_CARD2_ADDR	((uint32_t)SAVE_CARD1_ADDR + 10)
#define SAVE_CARD3_ADDR	((uint32_t)SAVE_CARD2_ADDR + 10)
#define SAVE_CARD4_ADDR	((uint32_t)SAVE_CARD3_ADDR + 10)
#define SAVE_CARD5_ADDR	((uint32_t)SAVE_CARD4_ADDR + 10)
#define SAVE_PW_ADDR	((uint32_t)SAVE_CARD5_ADDR + 10)

#define MAX_CARD 5
#define MAX_SIZE_ID 5
#define LEN_PW  6

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim1;
extern UART_HandleTypeDef huart3;

uint8_t status;
uint8_t str[MAX_LEN]; // Max_LEN = 16
uint8_t sNum[5];
volatile char key_pressed = 0;
char StringLCD[30];
uint8_t isCorrect = 0;
bool passisCorrect;
uint8_t failedAttempts;
uint8_t nPass = 0;
uint8_t mode_run = 0;

volatile bool mem_isFull = false;
bool card_isAvailable;
uint8_t pos_card;

uint8_t data_truyen[] = "e";
uint8_t CARD_CORRECT[MAX_CARD][MAX_SIZE_ID];
uint8_t PASSWORD_CORRECT[LEN_PW] = {'0','0','0','0','0','0'};
//uint8_t PASSWORD_CORRECT[LEN_PW];

char keys[4][4] = {{'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

uint8_t pass_input[6];


void keypad_init(void)
{
    // Configure GPIO pins for keypad matrix
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = ROW_1_Pin | ROW_2_Pin | ROW_3_Pin | ROW_4_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ROW_1_Port, &GPIO_InitStruct);
    HAL_GPIO_Init(ROW_2_Port, &GPIO_InitStruct);
    HAL_GPIO_Init(ROW_3_Port, &GPIO_InitStruct);
    HAL_GPIO_Init(ROW_4_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = COL_1_Pin | COL_2_Pin | COL_3_Pin | COL_4_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(COL_1_Port, &GPIO_InitStruct);
    HAL_GPIO_Init(COL_2_Port, &GPIO_InitStruct);
    HAL_GPIO_Init(COL_3_Port, &GPIO_InitStruct);
    HAL_GPIO_Init(COL_4_Port, &GPIO_InitStruct);
}
char keypad_scan(void)
{
    for(int i = 0; i < 4; i++)
    {
        // Set current column as output and low
        switch(i)
        {
        case 0:
            HAL_GPIO_WritePin(COL_1_Port, COL_1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(COL_2_Port, COL_2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(COL_3_Port, COL_3_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(COL_4_Port, COL_4_Pin, GPIO_PIN_SET);
            break;

        case 1:
            HAL_GPIO_WritePin(COL_1_Port, COL_1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(COL_2_Port, COL_2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(COL_3_Port, COL_3_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(COL_4_Port, COL_4_Pin, GPIO_PIN_SET);
            break;

        case 2:
            HAL_GPIO_WritePin(COL_1_Port, COL_1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(COL_2_Port, COL_2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(COL_3_Port, COL_3_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(COL_4_Port, COL_4_Pin, GPIO_PIN_SET);
            break;

        case 3:
            HAL_GPIO_WritePin(COL_1_Port, COL_1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(COL_2_Port, COL_2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(COL_3_Port, COL_3_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(COL_4_Port, COL_4_Pin, GPIO_PIN_RESET);
            break;
        }
// Read current rows
        if(HAL_GPIO_ReadPin(ROW_1_Port, ROW_1_Pin) == GPIO_PIN_RESET)
            return keys[0][i];
        if(HAL_GPIO_ReadPin(ROW_2_Port, ROW_2_Pin) == GPIO_PIN_RESET)
            return keys[1][i];
        if(HAL_GPIO_ReadPin(ROW_3_Port, ROW_3_Pin) == GPIO_PIN_RESET)
            return keys[2][i];
        if(HAL_GPIO_ReadPin(ROW_4_Port, ROW_4_Pin) == GPIO_PIN_RESET)
            return keys[3][i];
    }
    return 0; // No key pressed
}

bool Check_Card_ID()
{
    for(uint8_t i = 0; i < MAX_CARD; i++)
    {
        bool match = true;
        for(uint8_t j = 0; j < MAX_SIZE_ID; j++)
        {
            if(CARD_CORRECT[i][j] != sNum[j])
            {
                match = false;
                break;
            }
        }
        if (match == true)
            return true;
    }
    return false;
}

void isFull(void)
{
//  pos_card = 0xFF;
    for(int i = 0; i < MAX_CARD; i++)
    {
        //bool match = false;
        mem_isFull = false;
        for(int j = 0; j < MAX_SIZE_ID; j++)
        {
            if(CARD_CORRECT[i][j] != 0x00)// || CARD_CORRECT[i][j] != 0xFF)
            {
                mem_isFull = true;
                break;
            }
        }
        if (mem_isFull == false)
        {
            pos_card = i;
            mem_isFull = false;
            return;
            //return false;
        }
    }
    mem_isFull = true;
    return;
    //return true;
}

bool Check_PW()
{
//		PASSWORD_CORRECT[0] = '0';
//		PASSWORD_CORRECT[1] = '0';
//		PASSWORD_CORRECT[2] = '0';
//		PASSWORD_CORRECT[3] = '0';
//		PASSWORD_CORRECT[4] = '0';
//		PASSWORD_CORRECT[5] = '0';
    for(uint8_t i = 0; i < 6; i++)
    {
        if(pass_input[i] != PASSWORD_CORRECT[i])
            return false;
    }
    return true;
}


void handleFailedAttempts() {
    failedAttempts++;
    if (failedAttempts >= 3) {
        failedAttempts = 0;
        nPass = 0;
        lcd_clear();
        lcd_put_cur(0,0);
        lcd_send_string("TOO MUCH MISTAKES");

        for(int i = 30; i >= 0; i--)
        {
            lcd_put_cur(1,0);
            sprintf(&StringLCD[0],"Wait %d seconds!",i);
            lcd_send_string(&StringLCD[0]);
            HAL_Delay(1000);
        }
        lcd_clear();
    }
    else
    {
        nPass = 0;
        lcd_clear();
        lcd_put_cur(0,1);
        lcd_send_string("PASSWORD WRONG!");
        lcd_put_cur(1,0);
        lcd_send_string("TRY AGAIN, Pls!");
        HAL_Delay(1000);
        lcd_clear();
    }
}

void SWIPE_CARD_CHECK()
{
    status = MFRC522_Request(PICC_REQIDL, str);
    status = MFRC522_Anticoll(str);
    memcpy(sNum, str, MAX_SIZE_ID);

    if(status == MI_OK)
    {
        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
        HAL_Delay(200);
        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);

        isCorrect = Check_Card_ID();

        if(isCorrect == true)
        {
            lcd_clear();
            lcd_put_cur(0,0);
            lcd_send_string("COME IN, PLEASE!");
            HAL_GPIO_WritePin(LOCK_GPIO_Port,LOCK_Pin, 1);
            for(int i = 5; i >= 0; i--)
            {
                HAL_Delay(500);
                lcd_put_cur(1,2);
                sprintf(&StringLCD[0],"CLOSE IN %ds!",i);
                lcd_send_string(&StringLCD[0]);
                HAL_Delay(500);
            }
            HAL_GPIO_WritePin(LOCK_GPIO_Port,LOCK_Pin, 0);
            lcd_clear();
        }
        else
        {
            lcd_clear();
            lcd_put_cur(0,3);
            lcd_send_string("WRONG CARD!");
            lcd_put_cur(1,0);
            lcd_send_string("TRY AGAIN, Pls!");
            HAL_Delay(1000);
            lcd_clear();
        }
    }
    return;
}

void mode_password()
{
    lcd_clear();
    lcd_put_cur(0,0);
    lcd_send_string("PASSWORD: ");
    lcd_put_cur(1,0);
    lcd_send_string("B. Back to Home! ");
    nPass = 0;
    while(key_pressed != 0);
    while(1)
    {
        if( key_pressed != 0 && key_pressed == 'B')
        {
            HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
            HAL_Delay(200);
            HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
            lcd_clear();
            return;
        }

        if(key_pressed != 0 && key_pressed != 'A' && key_pressed != 'B' &&
                key_pressed != 'C' && key_pressed != 'D')
        {
            while(key_pressed == 0);
            lcd_put_cur(0,nPass+10);
//            lcd_send_string(&key_pressed);
            lcd_send_string("*");
            pass_input[nPass++] = key_pressed;
            HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
            HAL_Delay(200);
            HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
        }
        if(nPass > 5)
        {
            passisCorrect = Check_PW();

            if(passisCorrect == true)
            {
                lcd_clear();
                lcd_put_cur(0,0);
                lcd_send_string("COME IN, PLEASE!");
                lcd_put_cur(1,2);
                lcd_send_string("CLOSE IN ");
                HAL_GPIO_WritePin(LOCK_GPIO_Port,LOCK_Pin, 1);
								HAL_UART_Transmit(&huart3, data_truyen, sizeof(data_truyen), 100);
                for(int i = 0; i <= 5; i++)
                {
                    lcd_put_cur(1,11);
                    sprintf(&StringLCD[0],"%ds!",(5-i));
                    lcd_send_string(&StringLCD[0]);
                    HAL_Delay(1000);
                }
                HAL_GPIO_WritePin(LOCK_GPIO_Port,LOCK_Pin, 0);
                passisCorrect = false;
                failedAttempts = 0;
                nPass = 0;
                lcd_clear();
                return;
            }
            else
            {
                handleFailedAttempts();
                lcd_put_cur(0,0);
                lcd_send_string("PASSWORD:");
                lcd_put_cur(1,0);
                lcd_send_string("B. Back to Home! ");
            }
        }
    }
}

void change_PW()
{
    lcd_clear();
    lcd_put_cur(0,0);
    lcd_send_string("OLD PW: ");
    lcd_put_cur(1,0);
    lcd_send_string("NEW PW: ");
    nPass = 0;
    while(key_pressed != 0);
    while(1)
    {
        uint8_t enter_lv = 0;
        while(enter_lv == 0)
        {
            if( key_pressed != 0 && key_pressed == 'B')
            {
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                lcd_clear();
                failedAttempts = 0;
                nPass = 0;
                enter_lv = 3;
                return;
            }

            if(key_pressed != 0 && key_pressed != 'A' && key_pressed != 'B' &&
                    key_pressed != 'C' && key_pressed != 'D')
            {
                while(key_pressed == 0);
                lcd_put_cur(0,nPass+8);
//                lcd_send_string(&key_pressed);
                lcd_send_string("*");
                pass_input[nPass++] = key_pressed;
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
            }
            if(nPass > 5)
            {
                passisCorrect = Check_PW();

                if(passisCorrect == true)
                {
                    lcd_put_cur(0,15);
                    lcd_send_string("V");
                    nPass = 0;
                    enter_lv = 1;
                }
                else
                {
                    handleFailedAttempts();
                    lcd_put_cur(0,0);
                    lcd_send_string("OLD PW: ");
                    lcd_put_cur(1,0);
                    lcd_send_string("NEW PW: ");
                }
            }
        }
        while(key_pressed != 0);
        while(enter_lv == 1)
        {
            if( key_pressed != 0 && key_pressed == 'B')
            {
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                lcd_clear();
                failedAttempts = 0;
                nPass = 0;
                return;
            }

            if(key_pressed != 0 && key_pressed != 'A' && key_pressed != 'B' &&
                    key_pressed != 'C' && key_pressed != 'D')
            {
                while(key_pressed == 0);
                lcd_put_cur(1,nPass+8);
                sprintf(&StringLCD[0],"%c",key_pressed);
                lcd_send_string(&StringLCD[0]);
//                lcd_send_string("*");
                pass_input[nPass++] = key_pressed;
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
            }
            if(nPass > 5)
            {
                memcpy(PASSWORD_CORRECT, pass_input, 6);

                Flash_Erase(SAVE_CARD1_ADDR);
                Flash_Write_Array(SAVE_CARD1_ADDR,(uint16_t*)CARD_CORRECT[0],MAX_CARD*MAX_SIZE_ID);
                Flash_Write_Array(SAVE_PW_ADDR,(uint16_t*)PASSWORD_CORRECT,6);

                lcd_clear();
                lcd_put_cur(0,0);
                lcd_send_string("PASSWORD CHANGED!");

                HAL_Delay(1000);

                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);

                enter_lv = 2;
                lcd_clear();
                failedAttempts = 0;
                nPass = 0;
                return;
            }
        }
    }
}

void del_Card_Func()
{
    lcd_clear();
    lcd_put_cur(0,0);
    lcd_send_string("PASSWORD: ");
    lcd_put_cur(1,0);
    lcd_send_string("B. Back to Home! ");
    while(key_pressed != 0);
    nPass = 0;
    while(true)
    {
        uint8_t enter_lv = 0;

        while(enter_lv == 0)
        {
            if(key_pressed == 'B')
            {
                while(key_pressed != 'B');
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                lcd_clear();
                failedAttempts = 0;
                nPass = 0;
                return;
            }

            if(key_pressed != 0 && key_pressed != 'A' && key_pressed != 'B' &&
                    key_pressed != 'C' && key_pressed != 'D')
            {
                while(key_pressed == 0);
                lcd_put_cur(0,nPass+10);
//                lcd_send_string(&key_pressed);
                lcd_send_string("*");
                pass_input[nPass++] = key_pressed;
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
            }
            if(nPass > 5)
            {
                passisCorrect = Check_PW();
                if(passisCorrect == true)
                {
                    lcd_put_cur(0,15);
                    lcd_send_string("V");
                    nPass = 0;
                    failedAttempts = 0;
                    enter_lv = 1;
                }
                else
                {
                    handleFailedAttempts();
                    lcd_clear();
                    lcd_put_cur(0,0);
                    lcd_send_string("PASSWORD: ");
                    lcd_put_cur(1,0);
                    lcd_send_string("B. Back to Home! ");
                }
            }
        }
        while(key_pressed != 0);

        lcd_clear();
        lcd_put_cur(0,0);
        lcd_send_string("CHO0SE CARD: ");
        lcd_put_cur(1,0);
        lcd_send_string("B. Back to Home!");
        while(enter_lv == 1)
        {
            if(key_pressed == 'B')
            {
                while(key_pressed != 'B');
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                lcd_clear();
                failedAttempts = 0;
                nPass = 0;
                return;
            }
            if(key_pressed != 0 && key_pressed != 'A' && key_pressed != 'B' &&
                    key_pressed != 'C' && key_pressed != 'D')
            {
                while(key_pressed == 0);
                lcd_put_cur(0,0);
                sprintf(&StringLCD[0],"SELECT CARD: %c",key_pressed);
                lcd_send_string(&StringLCD[0]);
                uint8_t num = key_pressed - '0';
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);

                if(num != 0 && num <=5)
                {
                    bool areUsure = false;
                    lcd_clear();
                    while(areUsure == false && num != 0)
                    {
                        lcd_put_cur(0,2);
                        lcd_send_string("ARE U SURE ?");
                        lcd_put_cur(1,0);
                        lcd_send_string("A. YES     B. NO");
                        if(key_pressed != 0 && (key_pressed == 'A' || key_pressed == 'B'))
                        {
                            if(key_pressed == 'A')
                                areUsure = true;
                            else
                            {
                                lcd_clear();
                                lcd_put_cur(0,0);
                                lcd_send_string("SELECT CARD: ");
                                lcd_put_cur(1,0);
                                lcd_send_string("B. Back to Home! ");
                                areUsure = false;
                                num = 0;
                            }

                            HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                            HAL_Delay(200);
                            HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                        }
                    }
                    if(areUsure == true && num != 0)
                    {
                        for(uint8_t index = 0; index < MAX_SIZE_ID; index++)
                        {
                            CARD_CORRECT[num - 1][index] = 0x00;
                        }
                        Flash_Erase(SAVE_CARD1_ADDR);
                        Flash_Write_Array(SAVE_CARD1_ADDR,(uint16_t*)CARD_CORRECT[0],MAX_CARD*MAX_SIZE_ID);
                        Flash_Write_Array(SAVE_PW_ADDR,(uint16_t*)PASSWORD_CORRECT,6);

                        lcd_clear();
                        lcd_put_cur(0,2);
                        lcd_send_string("DELETED CARD!");
                        HAL_Delay(1000);
                        lcd_clear();
                        failedAttempts = 0;
                        nPass = 0;
                        return;
                    }
                }
                else
                {
                    HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                    HAL_Delay(200);
                    HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                    lcd_clear();
                    lcd_put_cur(0,0);
                    lcd_send_string("Pls! TRY AGAIN!");
                    lcd_put_cur(1,0);
                    lcd_send_string("Choose from 1->5");
                    HAL_Delay(1000);
                    lcd_clear();
                    lcd_put_cur(0,0);
                    lcd_send_string("SELECT CARD: ");
                    lcd_put_cur(1,0);
                    lcd_send_string("B. Back to Home! ");
                }
            }
        }
    }
}
void replace_Func(void)
{
    //volatile bool userChose = false;
    while(mem_isFull == true)
    {
        //HAL_Delay(1);
        if(key_pressed == 'A')
        {
            while(key_pressed != 'A');
            lcd_clear();
            lcd_put_cur(0,0);
            lcd_send_string("REPLACE IN: ");
            lcd_put_cur(1,0);
            lcd_send_string("B. Back to Home! ");
            HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
            HAL_Delay(200);
            HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);

            while(1)
            {
                if(key_pressed == 'B')
                {
                    while(key_pressed != 'B');
                    HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                    HAL_Delay(200);
                    HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                    lcd_clear();
                    return;
                }
                if(key_pressed != 0 && key_pressed != 'A' && key_pressed != 'B' &&
                        key_pressed != 'C' && key_pressed != 'D')
                {
                    while(key_pressed == 0);
                    volatile uint8_t num = key_pressed - 48;
                    lcd_put_cur(0,0);
                    sprintf(&StringLCD[0],"REPLACE IN: %c",key_pressed);
                    lcd_send_string(&StringLCD[0]);
                    HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                    HAL_Delay(200);
                    HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                    if(num != 0 && num <= 5)
                    {
                        volatile bool areUsure = false;

                        {
                            lcd_clear();
                            lcd_put_cur(0,2);
                            lcd_send_string("ARE U SURE ?");
                            lcd_put_cur(1,0);
                            lcd_send_string("A. YES     B. NO");
                        }
                        while(areUsure == false && num != 0)
                        {

                            if(key_pressed == 'A')
                            {
                                while(key_pressed != 'A');
                                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                                HAL_Delay(200);
                                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                                areUsure = true;
                            }
                            else if(key_pressed == 'B')
                            {
                                while(key_pressed != 'B');
                                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                                HAL_Delay(200);
                                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                                lcd_clear();
                                lcd_put_cur(0,0);
                                lcd_send_string("REPLACE IN: ");
                                lcd_put_cur(1,0);
                                lcd_send_string("B. Back to Home! ");
                                areUsure = false;
                                num = 0;
                            }

                        }
                        
                        
                        if(areUsure == true && num != 0)
                        {
                            for(uint8_t index = 0; index < MAX_SIZE_ID; index++)
                            {
                                CARD_CORRECT[num - 1][index] = sNum[index];
                            }
                                Flash_Erase(SAVE_CARD1_ADDR);
                                Flash_Write_Array(SAVE_CARD1_ADDR,(uint16_t*)CARD_CORRECT[0],MAX_CARD*MAX_SIZE_ID);
                                Flash_Write_Array(SAVE_PW_ADDR,(uint16_t*)PASSWORD_CORRECT,6);

                                Flash_Read_Array(SAVE_CARD1_ADDR,(uint16_t*)CARD_CORRECT[0],MAX_CARD*MAX_SIZE_ID);
                                Flash_Read_Array(SAVE_PW_ADDR,(uint16_t*)PASSWORD_CORRECT,6);
                                lcd_clear();
                                lcd_put_cur(0,1);
                                lcd_send_string("SUCCESSFULLY!");
                                HAL_Delay(1000);
                                lcd_clear();
                                failedAttempts = 0;
                                nPass = 0;
                                mem_isFull = false;
                                return;
                        }
                    }
                    else
                    {
                        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                        HAL_Delay(200);
                        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                        lcd_clear();
                        lcd_put_cur(0,0);
                        lcd_send_string("Pls! TRY AGAIN!");
                        lcd_put_cur(1,0);
                        lcd_send_string("Choose from 1->5");
                        HAL_Delay(1000);
                        lcd_clear();
                        lcd_put_cur(0,0);
                        lcd_send_string("REPLACE IN: ");
                        lcd_put_cur(1,0);
                        lcd_send_string("B. Back to Home! ");
                    }
                }
            }
        }
        else if (key_pressed == 'B')
        {
            while(key_pressed != 'B');
            HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
            HAL_Delay(200);
            HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
            // userChose = false;
            mem_isFull = false;
            return;
        }
    }
}

void add_New_Card(void)
{
    lcd_clear();
    lcd_put_cur(0,0);
    lcd_send_string("PASSWORD: ");
    lcd_put_cur(1,0);
    lcd_send_string("B. Back to Home! ");
    while(key_pressed != 0);
    nPass = 0;
    while(true)
    {
        uint8_t enter_lv = 0;

        while(enter_lv == 0)
        {
            if(key_pressed == 'B')
            {
                while(key_pressed != 'B');
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                lcd_clear();
                return;
            }

            if(key_pressed != 0 && key_pressed != 'A' && key_pressed != 'B' &&
                    key_pressed != 'C' && key_pressed != 'D')
            {
                while(key_pressed == 0);
                lcd_put_cur(0,nPass+10);
//                lcd_send_string(&key_pressed);
                lcd_send_string("*");
                pass_input[nPass++] = key_pressed;
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
            }
            if(nPass > 5)
            {
                passisCorrect = Check_PW();
                if(passisCorrect == true)
                {
                    lcd_put_cur(0,15);
                    lcd_send_string("V");
                    nPass = 0;
                    enter_lv = 1;
                }
                else
                {
                    handleFailedAttempts();
                    lcd_clear();
                    lcd_put_cur(0,0);
                    lcd_send_string("PASSWORD: ");
                    lcd_put_cur(1,0);
                    lcd_send_string("B. Back to Home! ");
                }
            }
        }
        lcd_clear();
        lcd_put_cur(0,0);
        lcd_send_string("SWIPE NEW CARD!");
        lcd_put_cur(1,0);
        lcd_send_string("B. Back to Home! ");
        while(key_pressed != 0);
        while(enter_lv == 1)
        {
            if(key_pressed == 'B')
            {
                while(key_pressed != 'B');
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                lcd_clear();
                return;
            }

            status = MFRC522_Request(PICC_REQIDL, str);
            status = MFRC522_Anticoll(str);
            memcpy(sNum, str, MAX_SIZE_ID);

            if(status == MI_OK)
            {
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                HAL_Delay(200);
                HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);

                card_isAvailable = Check_Card_ID();

                if(card_isAvailable == true)
                {
                    lcd_clear();
                    lcd_put_cur(0,1);
                    lcd_send_string("CARD IS AVAIL!");
                    HAL_Delay(1000);
                    lcd_clear();
                    lcd_put_cur(0,0);
                    lcd_send_string("SWIPE NEW CARD!");
                    lcd_put_cur(1,0);
                    lcd_send_string("B. Back to Home! ");
                }
                else
                {
                    isFull();
                    if(mem_isFull == true)
                    {
                       
                        {
                            lcd_clear();
                            lcd_put_cur(0,2);
                            lcd_send_string("MEM IS FULL!");
                            lcd_put_cur(1,0);
                            lcd_send_string("A.REPLACE B.EXIT");
                        }
                         replace_Func();
                         if(mem_isFull == false)
                            return;
                    }
                    else
                    {
                        lcd_clear();
                        lcd_put_cur(0,0);
                        lcd_send_string("SUCCESSFULLY!");
                        lcd_put_cur(1,0);
                        sprintf(&StringLCD[0],"CARD %d IS SAVED!", pos_card + 1);
                        lcd_send_string(&StringLCD[0]);
                        for(uint8_t index = 0; index < MAX_SIZE_ID; index++)
                        {
                            CARD_CORRECT[pos_card][index] = sNum[index];
                        }
                        Flash_Erase(SAVE_CARD1_ADDR);
                        Flash_Write_Array(SAVE_CARD1_ADDR,(uint16_t*)CARD_CORRECT[0],MAX_CARD*MAX_SIZE_ID);
                        Flash_Write_Array(SAVE_PW_ADDR,(uint16_t*)PASSWORD_CORRECT,6);
                        
                        Flash_Read_Array(SAVE_CARD1_ADDR,(uint16_t*)CARD_CORRECT[0],MAX_CARD*MAX_SIZE_ID);
                        Flash_Read_Array(SAVE_PW_ADDR,(uint16_t*)PASSWORD_CORRECT,6);
                        
                        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
                        HAL_Delay(200);
                        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
                        HAL_Delay(2000);
                        lcd_clear();
                        return;
                    }
                }
            }
        }
    }
}
uint8_t CARD_CORRECT_check[MAX_CARD][MAX_SIZE_ID];
char PW_CHECK[6];
void main_init()
{
    HAL_TIM_Base_Start_IT(&htim1);
    HAL_TIM_Base_Start_IT(&htim2);
    keypad_init();
    MFRC522_Init();
    LCD_Innit();
//    Flash_Erase(SAVE_CARD1_ADDR);
//    Flash_Write_Array(SAVE_CARD1_ADDR,(uint16_t*)CARD_CORRECT[0],MAX_CARD*MAX_SIZE_ID);
//    Flash_Write_Array(SAVE_PW_ADDR,(uint16_t*)PASSWORD_CORRECT,6);


    Flash_Read_Array(SAVE_CARD1_ADDR,(uint16_t*)CARD_CORRECT[0],MAX_CARD*MAX_SIZE_ID);
    Flash_Read_Array(SAVE_PW_ADDR,(uint16_t*)PASSWORD_CORRECT,6);
}

uint32_t curr_time = 0;
int8_t step_show = 0;
void main_process()
{
    // USE CARD TO OPEN
    SWIPE_CARD_CHECK();

    switch (key_pressed)
    {
    // USE PASSWORD TO OPEN
    case 'A':
        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
        HAL_Delay(200);
        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
        mode_password();
        break;
    // CHANGE PASSWORD
    case 'B':
        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
        HAL_Delay(200);
        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
        change_PW();
        break;
    // ADD NEW CARD
    case 'C':
        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
        HAL_Delay(200);
        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
        add_New_Card();
        break;
    // DELETE NEW CARD
    case 'D':
        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 1);
        HAL_Delay(200);
        HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin, 0);
        del_Card_Func();
        break;
    default:
        //curr_time = HAL_GetTick();
        if(step_show == 1)
        {
            lcd_put_cur(0,1);
            lcd_send_string("SWIPE CARD IN!");
            lcd_put_cur(1,0);
            lcd_send_string("A.ENTER PASSWORD");
        }
        else if(step_show == 2)
        {
            lcd_put_cur(0,1);
            lcd_send_string("SWIPE CARD IN!");
            lcd_put_cur(1,0);
            lcd_send_string("B.CHANGE PASSWD");
        }
        
        else if(step_show == 3)
        {
            lcd_put_cur(0,1);
            lcd_send_string("SWIPE CARD IN!");
            lcd_put_cur(1,0);
            lcd_send_string("C.ADD NEW CARD");
        }
        else if(step_show == 4)
        {
            lcd_put_cur(0,1);
            lcd_send_string("SWIPE CARD IN!");
            lcd_put_cur(1,0);
            lcd_send_string("D.DELETE CARD");
            step_show = 0;
        }
        if(HAL_GetTick() -  curr_time >= 2500)
        {
            curr_time = HAL_GetTick();
            lcd_put_cur(1,0);
            lcd_send_string("                ");
            step_show++;
        }
				
        break;
    }
		
}
uint32_t time_set = 0;
uint8_t start_open = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2)
        key_pressed = keypad_scan();
    if(htim->Instance == TIM1)
    {
        if(HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == 0)
        {
            time_set = HAL_GetTick();
            HAL_GPIO_WritePin(LOCK_GPIO_Port, LOCK_Pin, 1);
            while(HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == 0);
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, 1);
            start_open = 1;
        }
        
        if(start_open == 1 && HAL_GetTick() - time_set >= 200)
        {
            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, 0);
            start_open = 2;
        }
        
        
        if(start_open == 2 && HAL_GetTick() - time_set >= 5000)
        {
            HAL_GPIO_WritePin(LOCK_GPIO_Port, LOCK_Pin, 0);
            start_open = 0;
            time_set = 0;
        }
    }
}