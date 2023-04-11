#define ADC (*(volatile unsigned int *)0x24)
#define ADCL (*(volatile unsigned char *)0x24)
#define ADCH (*(volatile unsigned char *)0x25)
#define ADCSRA (*(volatile unsigned char *)0x26)
#define ADEN 7
#define ADSC 6
#define ADFR 5
#define ADATE 5
#define ADIF 4
#define ADIE 3
#define ADPS2 2
#define ADPS1 1
#define ADPS0 0
#define ADMUX (*(volatile unsigned char *)0x27)
#define REFS1 7
#define REFS0 6
#define ADLAR 5
#define MUX4 4
#define MUX3 3
#define MUX2 2
#define MUX1 1
#define MUX0 0

#include <iom128v.h>
#include "delay.h"
#include "lcd.h"
#include <stdlib.h>

//�������� �ʱ�ȭ
int txdata=0;
int flag=0; //�ܺ��� 0���� ������ ȭ���� �ٲ�Բ� �ϴ� ����
int number;
int flag2=0; //�ܺ��� 1���� ������ retry �Ǵ� �����Ѵٴ� ���ڸ� ����ϱ� ���� ����
int flag3=0; //3��° ȭ������ �Ѿ�� ���� flag
unsigned int ADCW=0;    //�ʱⰪ ����
typedef unsigned char Byte;

void Init_USART0(void)
{
UBRR0H = 0x00;
UBRR0L = 103;  // ��� �ӵ� 9,600 bps
UCSR0A = 0x00;
UCSR0B = (1<<RXEN0)|(1<<TXEN0);
UCSR0C = (1<<UPM01)|(1<<UCSZ01) | (1<<UCSZ00); // ¦�� �и�Ƽ, 8��Ʈ ������, �񵿱���, ������Ʈ 1��Ʈ
}

//�� ���ڸ� USART0�� �۽��ϴ� �Լ�
void putch_USART0(unsigned char data)
{
    while(!(UCSR0A&(1<<UDRE0)));
    //UDR�� �غ�� ������ while�� �ݺ�(polling)
    UDR0 = data;//UDR0�� �� �Է� -> ����
}
//���ͷ�Ʈ�� �ʱ�ȭ�ϴ� �Լ�
void Interrupt_init(void)
{
    EIMSK = 0x07; //INT0�� ���
    EICRA = (1<<ISC21)|(1<<ISC01)|(1<<ISC01); //INT0�� falling edge trigger
    SREG |= 0x80; //��ü ���ͷ�Ʈ ���
}

void AD_Init(void)
{
    DDRF = 0x00; //Use Port F as an input port
    ADMUX = 0x00; // REFS1~0=��00�� -> AREF�� �������� �̿�, ADLAR=0 -> ��ȯ�� �����͸� �������� ����, MUX0~4=��00000�� -> ADC0�� �ܱؼ� �Է� ���� (Single-ended)
    ADCSRA |= (1<<ADEN) | (1<<ADSC) | (1<<ADFR) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
    // ��Ʈ����, ADEN=1 -> ADC �㰡 , ADSC=1 -> �ڵ����� ��ȯ�� �ݺ� ����, ADFR=1 -> Free running���, ADPS2~0=��111�� -> �ý��� Ŭ�� ���ֺ�� 128
}

void led_Lshift(void) // LED �¿��� ��� �����Ű�� �Լ�
{
    Byte LED; //LED �������� (8��Ʈ ��������)
    int i;
    LED=0xfe; //LED ������ �ʱⰪ����(0b 1111 1110)
    for(i=0;i<8;i++)
    {
         delay_ms(85);
         PORTB=LED;
         LED=(LED<<1)|0x01; // ��������Ʈ�� 1�� SET��Ű�鼭 �¿��� ��� led���������ݺ� OR�������̿�
    }
}

void led_Rshift(void) // LED �쿡�� �·� �����Ű�� �Լ�
{
    Byte LED; //LED �������� (8��Ʈ ��������)
    int i;
    LED=0x7f; //LED ������ �ʱⰪ����(0b 0111 1111)
    for(i=0;i<8;i++)
    {
         delay_ms(85);
         PORTB=LED;
         LED=(LED>>1)|0x80; // �ֻ�����Ʈ�� 1�� SET��Ű�鼭 �쿡�� �·� led���������ݺ� OR�������̿�
    }
}

unsigned int read_ADC(void)
{
    while( !(ADCSRA & (1<<ADIF))); // Polling ���
    ADCW = ADCL+ADCH*256; //AC ��ȯ ������ ����
    return ADC; //ADC ���
}
//������ ���ڸ� �޴� �Լ�
unsigned int random_number(void)
{
    int n;
    AD_Init();
    srand(read_ADC());//���������� �Ƴ��α� ���� rand���� seed�� ���
    n = rand()%8+1; //1~8������ ���ڸ� �ޱ�����
    return n;
}

void LED_control(int n)  //CLCD�� ��µǴ� ���ڸ�ŭ LED�� ǥ���ϴ� �Լ�
{
    while(1){
        if(n==8){
        PORTB = 0x00;
        break;}
        else if(n==7){
        PORTB = 0x80;
        break;}
        else if(n==6){
        PORTB = 0xc0;
        break;}
        else if(n==5){
        PORTB = 0xe0;
        break;}
        else if(n==4){
        PORTB = 0xf0;
        break;}
        else if(n==3){
        PORTB = 0xf8;
        break;}
        else if(n==2){
        PORTB = 0xfc;
        break;}
        else if(n==1){
        PORTB = 0xfe;
        break;}
    }
}

void first_CLCD(void)
{
    LCD_pos(0,0);
    LCD_STR("Welcome RunMAN");
    delay_ms(50);
    LCD_pos(1,0);
    LCD_STR("Press INT0");
    delay_ms(50);
}


void third_CLCD(void)
{
    LCD_Clear();
    LCD_pos(0,0);
    LCD_STR("Game Start");
    LCD_pos(1,0);
    LCD_STR("Game Start");
    delay_ms(1000);
}

void second_CLCD(void)
{
    LCD_Clear();    //LCD �ʱ�ȭ

    while(1){
    LCD_pos(0,0);   //ù���� �ٿ�
    LCD_STR("Stop at NO.7");    //LCD ���
    number = random_number();   //������ ���ڸ� �Է¹���
    LCD_pos(1,0);
    LCD_STR("No.");
    LCD_pos(1,3);
    LCD_CHAR(number+'0');   //������ ���ڸ� �ι����� 4����ĭ�� ���
    LED_control(number);    //���ڸ�ŭ LED ����
    LCD_pos(1,7);
    LCD_STR("INT1");
    delay_ms(600);
        if(flag2==1&&number==7) //INT1�� ������ ���ڰ� 7�϶�
        {
            LCD_Clear();
            LCD_pos(0,0);
            LCD_STR("Congratulation!");
            delay_ms(3000);
            led_Lshift();
            led_Rshift();
            flag3=1;
            break;
        }
        if(flag2==1&&number!=7) //INT1�� �������� ���ڰ� 7�� �ƴϸ�
        {
            LCD_Clear();
            LCD_pos(0,0);
            LCD_STR("Retry");
            delay_ms(500);
            continue;
        }
    }
}

#pragma interrupt_handler ext_int0_isr:iv_EXT_INT0
void ext_int0_isr(void) //��ž��ġȭ�� �˶�ȭ�� ��ư���� �����ϴ� �Լ�
{
    flag++;
    if(flag>1)
        flag=1;

}
#pragma interrupt_handler ext_int1_isr:iv_EXT_INT1
void ext_int1_isr(void) //��ž��ġȭ�� �˶�ȭ�� ��ư���� �����ϴ� �Լ�
{
    flag2++;
    delay_ms(100);
    if(flag2>1)
        flag2=0;
}

void main(void) //�����Լ�
{
    DDRD = 0x00;    // PORTD �Է����� ����
    DDRA = 0xff;    //PORTA ������� ����
    DDRB=0xff;   //PORTB�� ������� ���
    PORTB=0xff; //�ʱⰪ ���� (��ü�ҵ�)

    PortInit(); // LCD ��� ��Ʈ ����
    LCD_Init(); // LCD �ʱ�ȭ
    Interrupt_init(); //���ͷ�Ʈ �ʱ�ȭ
    Init_USART0(); // USART0 �ʱ�ȭ
    AD_Init();  //AD �ʱ�ȭ

    first_CLCD();   //�ʱ�ȭ�� ���

    while(1)
    {
    if(flag==1){    //�ܺ��� 0���� ������
        second_CLCD();  //�ι�° LCD ���

        if(flag3==1)    //7�϶� INT1���� ��������

        putch_USART0(1);    //"1" �۽�
        third_CLCD();   //3��° LCD���
        break;
        }
    }
    while(1){
    PORTB = 0xaa;
    delay_ms(500);
    PORTB = 0x55;
    delay_ms(500);
    }
}
