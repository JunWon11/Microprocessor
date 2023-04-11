#include <iom128v.h>
#include "lcd.h"
#include "delay.h"
unsigned int cnt = 0;//cnt ����
unsigned int flag = 0;
Byte a = 0x30;
Byte b = 0x30;
Byte c = 0x30;
Byte d = 0x30;
#pragma interrupt_handler timer0_comp_isr:iv_TIM0_COMP
void timer0_comp_isr(void)
{
    if(flag)
    {
        cnt++;
        OCR0 = 124;
    }
}

#pragma interrupt_handler ext_int0_isr:iv_EXT_INT0  //���ͷ�Ʈ ���� ��ƾ ����
void ext_int0_isr(void)
{
    flag=1;
}

#pragma interrupt_handler ext_int1_isr:iv_EXT_INT1  //���ͷ�Ʈ ���� ��ƾ ����
void ext_int1_isr(void)
{
    flag=0;
}

#pragma interrupt_handler ext_int2_isr:iv_EXT_INT2  //���ͷ�Ʈ ���� ��ƾ ����
void ext_int2_isr(void)
{
    cnt=0;
    a=0x30;
    b=0x30;
    c=0x30;
    d=0x30;
}

void init_timer(void)
{
    TCCR0 = (1<<WGM01) | (1<<CS00) | (1<<CS01) | (1<<CS02);//�Ϲݸ��, 1024���� ���
    TIMSK = (1<<OCIE0);
    OCR0 = 124;//TCNT0�� ����
}

void Interrupt_init(void)
{
    EIMSK = 0x07; // INT�� ���
    EICRA = 1<<ISC01; // INT0�� falling edge trigger
    EICRA = 1<<ISC11; // INT1�� falling edge trigger
    EICRA = 1<<ISC21; // INT2�� falling edge trigger
    SREG |= 0x80; // ��ü ���ͷ�Ʈ ���
}

void main(void)
{
    DDRB = 0x0F;//B��Ʈ�� ����4��Ʈ�� ���
    DDRD = 0x00;
    LCD_Init(); // LCD �ʱ�ȭ
    PortInit();
    init_timer();//Ÿ�̸� �ʱ�ȭ �Լ�
    Interrupt_init();
    PORTB = 0xFF;//LED�� �ʱⰪ

    LCD_pos(0,0);
    LCD_Data(a);
    LCD_pos(0,2);
    LCD_STR(":");
    LCD_pos(0,3);
    LCD_Data(b);

    while(1)
    {
        if(cnt == 125)
        {
            cnt = 0;
            d++;
        }
        if(d == 0x3A)
        {
            c++;
            d = 0x30;
        }
        if(c == 0x36)
        {
            b++;
            c = 0x30;
        }
        if(b== 0x3A)
        {
            a++;
            b = 0x30;
        }
        LCD_pos(0,0);
        LCD_Data(a);
        LCD_Data(b);
        LCD_STR(":");
        LCD_Data(c);
        LCD_Data(d);
    }

}
