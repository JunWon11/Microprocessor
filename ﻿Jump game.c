#include <iom128v.h>
#include <stdlib.h> // rand()함수를 쓰기 위한 헤더파일 선언
#include "lcd.h"
#include "delay.h"

unsigned int time = 0; // 게임에서 버틴 시간을 측정할 변수
unsigned int huddle_delay=0; // 장애물이 존재할 시간을 측정하는 변수
unsigned int jumpping_time=0; // 점프를 유지할 시간을 측정하는 변수
unsigned int msec = 0; // 딜레이를 위한 시간계수를 할 변수
unsigned int jumpping = 0; // 점프 상태를 나타낼 변수(0 : 점프x 1: 점프중)
unsigned int space = 1; // 띄울 화면을 구분할 변수(1: 초기화면, 2: 로딩 화면, 3: 게임 화면, 4: 게임 오버, 5: 신기록 경신, 6: 기록 경신 x)
unsigned int pause = 0; // 게임 시작 상황에서 정지상태를 나타낼 변수(0: 평상시, 1: 정지상태)
unsigned int sec = 0; // 게임에서 살아남은 시간(초)을 계수할 변수
unsigned int min = 0; // 게임에서 살아남은 시간(분)을 계수할 변수
unsigned int over = 0; // 게임 오버를 나타낼 변수(0: 평상시, 1: 게임 오버)
unsigned int huddle_on = 0; // 장애물이 있는 상태를 나타낼 변수(0: 장애물이 없음, 1: 장애물이 있음)
unsigned int huddle_speed = 0; // 장애물의 속도를 저장할 변수
unsigned int max_time = 0; // 최고 기록을 초단위로 저장할 변수
int huddle_index = 15; // 장애물의 위치를 저장할 변수(장애물이 화면에 없는 경우를 만들기 위해 음수도 저장가능한 int로 선언)

Byte jump=0; // 점프를 제한할 수 있게 하는 변수(0: 점프 가능, 1: 점프 불가능)
Byte txdata=0; // 8비트 수신 데이터
Byte rxdata=0; // 8비트 수신 데이터

void Interrupt_init(void)
{
    EIMSK = (1<<INT2)|(1<<INT1)|(1<<INT0); // INT0~INT2의 사용
    EICRA = (1<<ISC21)|(1<<ISC11)|(1<<ISC01);   //인터럽트 레지스터 설정
    SREG |= 0x80;   //인터럽트 허가
}

void Init_Timer0(void){
    TCCR0=(1<<WGM01)|(1<<CS02); // CTC 모드, 64분주, 출력x
    TIMSK |=(1<<OCIE0); // 출력비교 인터럽트 허가
    OCR0=249;   // 250*64/16MHz = 1msec
}

void Init_USART0(void)
{
    UBRR0H = 0x00; // 통신 속도 9,600 bps
    UBRR0L = 0x67;  // 통신 속도 9,600 bps, UCSR0A = 0x00(1배속)이라서 UCSR0A 생략
    UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0)|(1<<UDRIE0);
    // 송수신 가능, RXC인터립트, UDR 인터럽트 허가
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00)|(1<<UPM01); // 짝수 패리티, 8비트 데이터, 비동기모드, 정지비트 1비트
    SREG |= 0x80;
}   // 짝수 패리티, 8비트 데이터, 비동기모드, 정지비트 1비트, 1배속, 통신 속도 9,600 bps

void AD_Init(void)
{
    DDRF = 0x00; // Use Port F as an input port
    ADMUX = 0x00; // REFS1~0=’00’ ('AREF‘), ADLAR=0, MUX0~4=‘00000’('ADC0 Single‘)
    ADCSRA |= (1<<ADEN)|(1<<ADSC)|(1<<ADFR)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
    // ADEN=1, ADSC=1, ADFR=1, ADPS2~0=‘111’ (Prescaler=128)
}

void Init_data(void) // 게임에서 쓰이는 모든 데이터를 초기화(게임을 새로 시작할 때)
{
    time = 0;
    huddle_delay=0;
    jumpping_time=0;
    msec = 0;
    jumpping = 0;
    pause = 0;
    sec = 0;
    min = 0;
    over = 0;
    huddle_on = 0;
    huddle_speed = 0;
    huddle_index = 15;
    jump=0;
    LCD_Clear();
}

#pragma interrupt_handler ext_int1_isr:iv_EXT_INT1
#pragma interrupt_handler ext_int0_isr:iv_EXT_INT0
#pragma interrupt_handler ext_int2_isr:iv_EXT_INT2
#pragma interrupt_handler timer0_comp_isr:iv_TIM0_COMP
#pragma interrupt_handler usart0_transmit:iv_USART0_DRE
void usart0_transmit(void)
{
    UDR0 = txdata;  //송신데이터 입력
}// 송신 인터럽트
#pragma interrupt_handler usart0_receive:iv_USART0_RXC
void usart0_receive(void)
{
    rxdata = UDR0;  // 수신데이터 입력
}// 수신 완료 인터럽트

unsigned int read_ADC(void)
{
    while(!(ADCSRA & (1<<ADIF))); // Polling 방식
    return ADC; // ADC값을 리턴
}

void ext_int0_isr(void)
{
       if(jump==0&&space==3){ // 점프가 제한되어 있지 않고, 게임 진행 상황일 때
       jumpping = 1; // 캐릭터가 점프상태
       jumpping_time = 0; // 점프를 하는 시간을 계수하기 위해 시간을 0으로 초기화
       }
}

void ext_int1_isr(void)
{
    if(space == 5 ||space == 6){ // 게임 오버 상태에서 리셋버튼을 눌렀을 때
       space = 2; // 로딩중인 화면으로 변경
       Init_data(); // 게임에 대한 모든 데이터를 초기화(기록 제외)
    }
}

void ext_int2_isr(void)
{
    if(space == 1) // 초기 화면일 때 누르면 게임 시작
        space = 2; // 로딩화면으로 변경
    else if(space == 5 ||space == 6){ // 게임 오버일 때 누르면
        space = 1; // 초기 화면으로 변경
        Init_data(); // 모든 데이터 초기화
    }
}

void timer0_comp_isr(void){
    msec++; // 항상 계수하는 시간 (delay에 사용)
    if(space == 3&&!pause&&!over){
        time++;   // 게임중에 버틴 시간을 저장하는 변수
        jumpping_time++;  //캐릭터가 점프중인 시간을 저장하는 변수
        huddle_delay++;  // 장애물이 유지되는 시간을 저장하는 변수
        if(time == 1000){ // 타이머의 클럭당 1msec이기 때문에 time이 1000이 되면
            sec++; // 1초를 계수한다.
            time = 0; // time은 초기화
        }
        if(sec == 60){ // 만약 60초가 지나면
            min++; // 1분을 추가하고
            sec = 0; // 0초로 초기화한다.
        }
    }
}

void CGRAM_Set(void){

    int i;
    Byte human[]={0x04, 0x0A, 0x04, 0x1F, 0x04, 0x04, 0x0A, 0x0A}; // 캐릭터 저장
    Byte jumping[]={0x04, 0x0A, 0x15, 0x0E, 0x04, 0x04, 0x0A, 0x11}; // 점프하는 모습
    Byte hurdle[]={0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}; // 장애물
    Byte white[]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 아무것도 없는 네모

    LCD_Comm(0x40); // 0x40에

    for(i=0;i<8;i++){
        LCD_Data(human[i]); //캐릭터를 저장
    }

    LCD_Comm(0x48); // 0x48에

    for(i=0;i<8;i++){
        LCD_Data(jumping[i]); // 점프하는 모습의 캐릭터를 저장
    }

    LCD_Comm(0x50); // 0x50에

    for(i=0;i<8;i++){
        LCD_Data(hurdle[i]); // 장애물을 저장
    }
    LCD_Comm(0x58); // 0x58에
    for(i=0;i<8;i++){
        LCD_Data(white[i]); // 아무것도 없는 네모를 저장
    }

}

void delay_isr_ms(unsigned int m)
{
    msec = 0; // 우선 msec를 0으로 초기화
    while(1){
        if(msec == m){ // 1msec * m초를 셀 때 까지 반복문에 머무름
            msec = 0;
            break;
        }
    }
}

void main(void){

    Byte time_str[]="00:00"; // 버틴 시간을 문자열로 저장
    Byte max_time_str[] = "00:00"; // 최고기록을 문자열로 저장
    unsigned int adc_data;  // ADC값을 저장할 변수

    Interrupt_init(); // 인터럽트 초기화
    Init_USART0(); // 통신 초기화
    PortInit(); // 포트 초기화
    LCD_Init(); // LCD 초기화

    CGRAM_Set(); // 그린 그림을 사용하기 위해 데이터를 불러들임
    Init_Timer0(); // 타이머 허가
    AD_Init(); // ADC 허가
    LCD_Clear(); // LCD 화면을 모두 지움


    DDRD = 0x00; // PORTD를 입력모드로 설정
    DDRB = 0xFF; // PORTB를 출력모드로 설정
    PORTB = 0xFF; // PORTB의 LED를 꺼놓은 초기 상태 설정

    while(1){ // 무한 루프
        if(rxdata == 1){ // 만약 1이라는 값이 들어오면
            time_str[4] = '0' + sec%10; // 초의 첫 번째 자리 저장
            time_str[3] = '0' + sec/10; // 초의 두 번째 자리 저장
            time_str[1] = '0' + min%10; // 분의 첫 번째 자리 저장
            time_str[0] = '0' + min/10; // 분의 두 번째 자리 저장
            if(space == 1){ // 초기 화면일 때
                LCD_pos(0,0);
                LCD_STR("    JUMP  GAME");
                LCD_pos(1,0);
                LCD_STR("  START : INT2");
            }
            else if(space == 2){ // 로딩화면
                LCD_Clear();
                for(int i=0;i<16;i++){ // 0.05sec * 16 = 0.8sec
                    LCD_pos(0,i);
                    LCD_Data(0x02); // 꽉 찬 그림
                    delay_isr_ms(50); // 0.05sec
                }
                for(int i=0;i<16;i++){ // 0.05sec * 16 = 0.8sec
                    LCD_pos(1,i);
                    LCD_Data(0x02); // 꽉 찬 그림
                    delay_isr_ms(50); // 0.05sec
                }
                LCD_Clear(); // LCD 화면 클리어
                space = 3; // 게임 실행 화면으로 넘어감
            }
            else if(space == 3){ // 게임 실행 화면일 때
                adc_data=read_ADC(); // 조도센서의 ADC값 저장
                if(adc_data<200){   // 정해둔 조도 이하로 떨어지면
                    pause = 1; // 게임을 정지시킴
                    LCD_Clear(); // LCD 화면 클리어
                    LCD_pos(0,4);
                    LCD_STR("PAUSE");   //PAUSE 출력
                    LCD_delay(1);
                }
                else{ // 정해둔 조도를 넘을 때
                    pause = 0; // 정지상태 해제
                    LCD_pos(0,3);
                    LCD_STR("TIME: "); //스코어 글자 출력
                    LCD_pos(0,9);
                    LCD_STR(time_str);   // 버틴 시간 출력
                    if(over){ // 게임 오버 상태일 때
                        space = 4; // 게임 오버 화면으로 넘어감
                        over = 0; // 게임 오버 값을 0으로 초기화
                    }
                    else{ // 정상적인 진행상태 일 때(게임 오버가 아닐 때)
                        LCD_pos(1,1);
                        LCD_Data(0x00); // 원래 자리에 캐릭터를 출력
                        if(jumpping&&jumpping_time<1000){ // 만약 점프를 한 상태, 점프를 한 지 1초가 되지 않았을 때
                            jump = 1; // 점프를 제한시킴
                            LCD_pos(1,1); // 원래 캐릭터가 있던 자리를
                            LCD_Data(0x03); // 아무것도 없는 화면으로 비워둠
                            LCD_pos(0,1); // 점프를 했으므로 한 칸 위에
                            LCD_Data(0x01); // 점프중인 모습의 캐릭터를 출력
                            PORTB = 0x00; // 점프를 할 때 마다 LED를 모두 켠다.
                        }
                        else if(jumpping&&jumpping_time>=1000){ // 점프를 한 상태, 점프를 한 지 1초가 지났을 때
                            jumpping = 0; // 0을 저장하여 점프중이 아님을 기록
                            LCD_pos(0,1); // 캐릭터가 점프를 한 자리를
                            LCD_Data(0x03); // 아무것도 없는 화면으로 비워둠
                            LCD_pos(1,1); // 원래 있던 자리에
                            LCD_Data(0x00); // 캐릭터를 다시 출력
                            PORTB = 0xFF; // 점프가 끝났으므로 LED를 모두 끈다.
                        }
                        else if(jumpping_time>1400){ // 점프를 한 지, 1.4초가 지났을 때
                            jumpping_time = 0; // 점프를 한 시간을 초기화 한다.
                            jump = 0; // 점프 제한을 해제한다.
                        }
                        if(huddle_on){ // 만약 장애물이 있는 상태이면
                            LCD_pos(1,huddle_index); // huddle_index의 위치에
                            LCD_Data(0x02); // 장애물을 출력한다.
                            if(huddle_delay>(250-huddle_speed*50)){ // 장애물이 250-huddle_speed*50을 넘으면
                                LCD_pos(1,huddle_index); // 장애물을 출력할 위치
                                LCD_Data(0x03); // 장애물 출력
                                huddle_index--; // 장애물의 위치를 한 칸 당긴다.
                                huddle_delay = 0; // 다시 장애물이 존재하는 시간을 저장하는 변수 초기화
                            }
                            if(huddle_index < 0) // 만약 장애물이 끝까지 갔다면
                                huddle_on = 0; // 장애물이 없는 상태로 저장한다.
                        }
                        else{ // 장애물이 없는 상태
                            huddle_speed = rand()%4; // 장애물의 속도를 rand()함수를 이용하여 저장한다.(난이도 조절)
                            huddle_index = 15; // 장애물의 위치를 초기화한다.
                            huddle_on = 1; // 장애물이 있는 상태로 바꿔준다.
                        }
                        if(!jumpping && huddle_index == 1) // 만약 점프중이지 않은데 장애물의 위치와 캐릭터의 위치가 같으면
                            over = 1; // 게임 오버 상태로 바꾼다.
                    }
                }
            }
            else if(space == 4){ // 게임 오버 상태일 때
                LCD_pos(0,0);
                LCD_STR("   GAME OVER  "); // 게임 오버 문구 출력
                LCD_pos(1,0);
                LCD_STR("  TIME : "); // TIME 문구 출력
                LCD_pos(1,9);
                LCD_STR(time_str);   // 버틴 시간을 출력
                delay_isr_ms(3000); // 3초간 딜레이
                if(min*60 + sec > max_time){ // 만약 최고기록을 경신했다면
                    max_time = min*60 + sec;
                    max_time_str[4] = '0' + sec%10;
                    max_time_str[3] = '0' + sec/10;
                    max_time_str[1] = '0' + min%10;
                    max_time_str[0] = '0' + min/10; // 최고기록을 저장하는 문자열의 요소들을 바꿔줌
                    space = 5; // 최고기록을 갱신했다는 화면으로 넘어감
                }
                else // 최고기록을 경신하지 못했다면
                    space = 6; // 다른 화면으로 넘어감
                LCD_Clear(); // LCD 클리어
            }
            else if(space == 5){ // 최고기록 경신 화면
                LCD_pos(0,0);
                LCD_STR(" NEW RECORD!!!!"); // 최고기록을 나타내는 문구 출력
                LCD_pos(1,0);
                LCD_STR(" RECORD : ");
                LCD_pos(1,10);
                LCD_STR(max_time_str); // 경신한 최고기록 출력
                PORTB = 0xFE; // 최고기록을 축하하기 위해 LED를 끝에서 끝으로 하나씩 킨다.
                for(int i=0;i<8;i++){
                    delay_ms(50);
                    PORTB = (PORTB<<1)|0x01; //출력을 다음 포트로 넘긴다.(아래로)
                }
                //방향을 바꿔준다.
                PORTB = 0x7F; // 가장 아래의 LED만 켜짐
                for(int i=0;i<8;i++){
                    delay_ms(50);
                    PORTB = (PORTB>>1)|0x80; //출력을 다음 포트로 넘긴다.(위로)
                }
            }
            else if(space == 6){ // 최고기록을 경신하지 못했다면
                LCD_pos(0,0);
                LCD_STR(" RECORD : ");
                LCD_pos(0,10);
                LCD_STR(max_time_str); // 기존 최고 기록을 출력하고
                LCD_pos(1,0);
                LCD_STR("RESET: INT1 or 2"); // 리셋을 위한 안내문구 출력
            }
        }
        else{ // 통신값으로 1을 받지 못한 상태라면
            LCD_pos(0,0);
            LCD_STR("Wait"); // Wait라는 문구를 출력
        }
    }
}
