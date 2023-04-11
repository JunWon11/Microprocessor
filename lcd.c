#include "lcd.h"

void LCD_delay(Byte ms)
{
    delay_ms(ms);
}

void PortInit(void)
{
    DDRA = 0xFF; // PORTA를 출력으로
    DDRG = 0x0F; // PORTG의 하위 4비트를 출력으로
}

void LCD_Data(Byte ch) // LCD_DR에 데이터 출력
{
    LCD_CTRL |= (1 << LCD_RS);  // RS=1, 𝑅/𝑊ഥ =0으로 데이터 쓰기 사이클
    LCD_CTRL &= ~(1 << LCD_RW);
    LCD_CTRL |= (1 << LCD_EN);  // LCD Enable
    delay_us(50);               // 시간지연
    LCD_WDATA = ch;             // 데이터 출력
    delay_us(50);               // 시간지연
    LCD_CTRL &= ~(1 << LCD_EN); // LCD Disable
}

void LCD_Comm(Byte ch) // LCD IR에 명령어 쓰기
{
    LCD_CTRL &= ~(1 << LCD_RS);     // RS=𝑅/𝑊ഥ =0으로 명령어 쓰기 사이클
    LCD_CTRL &= ~(1 << LCD_RW);
    LCD_CTRL |= (1 << LCD_EN);      // LCD Enable
    delay_us(50);                   // 시간지연
    LCD_WINST = ch;                 // 명령어 쓰기
    delay_us(50);                   // 시간지연
    LCD_CTRL &= ~(1 << LCD_EN);     // LCD Disable
}

void LCD_Shift(char p) // LCD 화면을 좌로 또는 우로 이동
{
// 표시 화면 전체를 오른쪽으로 이동
    if(p == RIGHT) {
        LCD_Comm(0x1C); // * A에서 C로 바꿈
        LCD_delay(1); // 시간 지연
    }
// 표시 화면 전체를 왼쪽으로 이동
    else if(p == LEFT) {
        LCD_Comm(0x18);
        LCD_delay(1);
    }
}

void LCD_CHAR(Byte c) // 한 문자 출력
{
    LCD_Data(c);
    delay_ms(1);
}

void LCD_STR(Byte *str) // 문자열 출력
{
    while(*str != 0) {
        LCD_CHAR(*str);
        str++;
    }
}

void LCD_pos(unsigned char row, unsigned char col) // LCD 포지션 설정
{
    LCD_Comm(0x80|(row*0x40+col)); // row = 문자행, col = 문자열
}

void LCD_Clear(void) // 화면 클리어 (1)
{
    LCD_Comm(0x01);
    LCD_delay(2);
}

void LCD_Init(void) // LCD 초기화
{
    LCD_Comm(0x38); // DDRAM, 데이터 8비트사용, LCD 2열로 사용 (6)
    LCD_delay(2); // 2ms 지연
    LCD_Comm(0x38); // DDRAM, 데이터 8비트사용, LCD 2열로 사용 (6)
    LCD_delay(2); // 2ms 지연
    LCD_Comm(0x38); // DDRAM, 데이터 8비트사용, LCD 2열로 사용 (6)
    LCD_delay(2); // 2ms 지연
    LCD_Comm(0x0e); // Display ON/OFF
    LCD_delay(2); // 2ms 지연
    LCD_Comm(0x06); // 주소+1 , 커서를 우측 이동 (3)
    LCD_delay(2); // 2ms 지연
    LCD_Clear(); // LCD 화면 클리어
}

void Cursor_home(void)
{
    LCD_Comm(0x02); // Cursor Home
    LCD_delay(2); // 2ms 시간지연
}
