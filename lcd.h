#ifndef __lcd_h
#define __lcd_h

#include <iom128v.h>
#include "delay.h"

#define LCD_WDATA   PORTA // LCD 데이터 포트 정의
#define LCD_WINST   PORTA
#define LCD_CTRL    PORTG // LCD 제어포트 정의
#define LCD_EN      0 // PORTG.0 포트를 이용
#define LCD_RW      1 // PORTG.1 포트를 이용
#define LCD_RS      2 // PORTG.2 포트를 이용

#define On 1
#define Off 0

#define RIGHT 1
#define LEFT 0

#define sbi(x,y)    (x |= (1<<y))
#define cbi(x,y)    (x &= ~(1<<y))


void LCD_delay(Byte ms);    // LCD 시간지연 함수
void PortInit(void);        // LCD 사용을 위한 PORT A와 PORT의 지정
void LCD_Data(Byte ch);     // 데이터 레지스터에 데이터 쓰기 함수
void LCD_Comm(Byte ch);     // 명령 레지스터에 명령어 쓰기 함수
void LCD_Shift(char p);     // LCD 화면을 좌로 또는 우로 이동
void LCD_CHAR(Byte c);      // 한 문자 출력
void LCD_STR(Byte *str);    // 문자열 출력
void LCD_pos(unsigned char row, unsigned char col); // 출력 위치 지정
void LCD_Clear(void);       // LCD 지움
void LCD_Init(void);        // LCD 초기화
void Cursor_home(void);     // Cursor를 (0,0)의 위치로


#endif
