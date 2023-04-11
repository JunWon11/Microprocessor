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

//전역변수 초기화
int txdata=0;
int flag=0; //외부핀 0번을 누르면 화면을 바뀌게끔 하는 변수
int number;
int flag2=0; //외부핀 1번을 누르면 retry 또는 축하한다는 문자를 출력하기 위한 변수
int flag3=0; //3번째 화면으로 넘어가기 위한 flag
unsigned int ADCW=0;    //초기값 설정
typedef unsigned char Byte;

void Init_USART0(void)
{
UBRR0H = 0x00;
UBRR0L = 103;  // 통신 속도 9,600 bps
UCSR0A = 0x00;
UCSR0B = (1<<RXEN0)|(1<<TXEN0);
UCSR0C = (1<<UPM01)|(1<<UCSZ01) | (1<<UCSZ00); // 짝수 패리티, 8비트 데이터, 비동기모드, 정지비트 1비트
}

//한 문자를 USART0로 송신하는 함수
void putch_USART0(unsigned char data)
{
    while(!(UCSR0A&(1<<UDRE0)));
    //UDR이 준비될 떄까지 while문 반복(polling)
    UDR0 = data;//UDR0에 값 입력 -> 전송
}
//인터럽트를 초기화하는 함수
void Interrupt_init(void)
{
    EIMSK = 0x07; //INT0의 사용
    EICRA = (1<<ISC21)|(1<<ISC01)|(1<<ISC01); //INT0의 falling edge trigger
    SREG |= 0x80; //전체 인터럽트 허용
}

void AD_Init(void)
{
    DDRF = 0x00; //Use Port F as an input port
    ADMUX = 0x00; // REFS1~0=’00’ -> AREF의 기준전압 이용, ADLAR=0 -> 변환된 데이터를 우측부터 저장, MUX0~4=‘00000’ -> ADC0에 단극성 입력 연결 (Single-ended)
    ADCSRA |= (1<<ADEN) | (1<<ADSC) | (1<<ADFR) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
    // 비트연산, ADEN=1 -> ADC 허가 , ADSC=1 -> 자동으로 변환이 반복 수행, ADFR=1 -> Free running모드, ADPS2~0=‘111’ -> 시스템 클럭 분주비는 128
}

void led_Lshift(void) // LED 좌에서 우로 점등시키는 함수
{
    Byte LED; //LED 변수정의 (8비트 데이터형)
    int i;
    LED=0xfe; //LED 변수의 초기값선언(0b 1111 1110)
    for(i=0;i<8;i++)
    {
         delay_ms(85);
         PORTB=LED;
         LED=(LED<<1)|0x01; // 최하위비트에 1을 SET시키면서 좌에서 우로 led켜짐꺼짐반복 OR연산자이용
    }
}

void led_Rshift(void) // LED 우에서 좌로 점등시키는 함수
{
    Byte LED; //LED 변수정의 (8비트 데이터형)
    int i;
    LED=0x7f; //LED 변수의 초기값선언(0b 0111 1111)
    for(i=0;i<8;i++)
    {
         delay_ms(85);
         PORTB=LED;
         LED=(LED>>1)|0x80; // 최상위비트에 1을 SET시키면서 우에서 좌로 led켜짐꺼짐반복 OR연산자이용
    }
}

unsigned int read_ADC(void)
{
    while( !(ADCSRA & (1<<ADIF))); // Polling 방식
    ADCW = ADCL+ADCH*256; //AC 변환 데이터 읽음
    return ADC; //ADC 출력
}
//랜덤한 숫자를 받는 함수
unsigned int random_number(void)
{
    int n;
    AD_Init();
    srand(read_ADC());//조도센서의 아날로그 값을 rand값의 seed로 사용
    n = rand()%8+1; //1~8까지의 숫자를 받기위함
    return n;
}

void LED_control(int n)  //CLCD에 출력되는 숫자만큼 LED를 표시하는 함수
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
    LCD_Clear();    //LCD 초기화

    while(1){
    LCD_pos(0,0);   //첫번쨰 줄에
    LCD_STR("Stop at NO.7");    //LCD 출력
    number = random_number();   //랜덤한 숫자를 입력받음
    LCD_pos(1,0);
    LCD_STR("No.");
    LCD_pos(1,3);
    LCD_CHAR(number+'0');   //랜덤한 숫자를 두번쨰줄 4번쨰칸에 출력
    LED_control(number);    //숫자만큼 LED 점등
    LCD_pos(1,7);
    LCD_STR("INT1");
    delay_ms(600);
        if(flag2==1&&number==7) //INT1을 누르고 숫자가 7일때
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
        if(flag2==1&&number!=7) //INT1을 눌렀을때 숫자가 7이 아니면
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
void ext_int0_isr(void) //스탑워치화면 알람화면 버튼으로 조절하는 함수
{
    flag++;
    if(flag>1)
        flag=1;

}
#pragma interrupt_handler ext_int1_isr:iv_EXT_INT1
void ext_int1_isr(void) //스탑워치화면 알람화면 버튼으로 조절하는 함수
{
    flag2++;
    delay_ms(100);
    if(flag2>1)
        flag2=0;
}

void main(void) //메인함수
{
    DDRD = 0x00;    // PORTD 입력으로 설정
    DDRA = 0xff;    //PORTA 출력으로 설정
    DDRB=0xff;   //PORTB를 출력으로 사용
    PORTB=0xff; //초기값 설정 (전체소등)

    PortInit(); // LCD 출력 포트 설정
    LCD_Init(); // LCD 초기화
    Interrupt_init(); //인터럽트 초기화
    Init_USART0(); // USART0 초기화
    AD_Init();  //AD 초기화

    first_CLCD();   //초기화면 출력

    while(1)
    {
    if(flag==1){    //외부핀 0번이 눌리면
        second_CLCD();  //두번째 LCD 출력

        if(flag3==1)    //7일때 INT1번을 눌렀을때

        putch_USART0(1);    //"1" 송신
        third_CLCD();   //3번째 LCD출력
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
