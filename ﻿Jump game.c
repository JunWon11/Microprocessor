#include <iom128v.h>
#include <stdlib.h> // rand()�Լ��� ���� ���� ������� ����
#include "lcd.h"
#include "delay.h"

unsigned int time = 0; // ���ӿ��� ��ƾ �ð��� ������ ����
unsigned int huddle_delay=0; // ��ֹ��� ������ �ð��� �����ϴ� ����
unsigned int jumpping_time=0; // ������ ������ �ð��� �����ϴ� ����
unsigned int msec = 0; // �����̸� ���� �ð������ �� ����
unsigned int jumpping = 0; // ���� ���¸� ��Ÿ�� ����(0 : ����x 1: ������)
unsigned int space = 1; // ��� ȭ���� ������ ����(1: �ʱ�ȭ��, 2: �ε� ȭ��, 3: ���� ȭ��, 4: ���� ����, 5: �ű�� ���, 6: ��� ��� x)
unsigned int pause = 0; // ���� ���� ��Ȳ���� �������¸� ��Ÿ�� ����(0: ����, 1: ��������)
unsigned int sec = 0; // ���ӿ��� ��Ƴ��� �ð�(��)�� ����� ����
unsigned int min = 0; // ���ӿ��� ��Ƴ��� �ð�(��)�� ����� ����
unsigned int over = 0; // ���� ������ ��Ÿ�� ����(0: ����, 1: ���� ����)
unsigned int huddle_on = 0; // ��ֹ��� �ִ� ���¸� ��Ÿ�� ����(0: ��ֹ��� ����, 1: ��ֹ��� ����)
unsigned int huddle_speed = 0; // ��ֹ��� �ӵ��� ������ ����
unsigned int max_time = 0; // �ְ� ����� �ʴ����� ������ ����
int huddle_index = 15; // ��ֹ��� ��ġ�� ������ ����(��ֹ��� ȭ�鿡 ���� ��츦 ����� ���� ������ ���尡���� int�� ����)

Byte jump=0; // ������ ������ �� �ְ� �ϴ� ����(0: ���� ����, 1: ���� �Ұ���)
Byte txdata=0; // 8��Ʈ ���� ������
Byte rxdata=0; // 8��Ʈ ���� ������

void Interrupt_init(void)
{
    EIMSK = (1<<INT2)|(1<<INT1)|(1<<INT0); // INT0~INT2�� ���
    EICRA = (1<<ISC21)|(1<<ISC11)|(1<<ISC01);   //���ͷ�Ʈ �������� ����
    SREG |= 0x80;   //���ͷ�Ʈ �㰡
}

void Init_Timer0(void){
    TCCR0=(1<<WGM01)|(1<<CS02); // CTC ���, 64����, ���x
    TIMSK |=(1<<OCIE0); // ��º� ���ͷ�Ʈ �㰡
    OCR0=249;   // 250*64/16MHz = 1msec
}

void Init_USART0(void)
{
    UBRR0H = 0x00; // ��� �ӵ� 9,600 bps
    UBRR0L = 0x67;  // ��� �ӵ� 9,600 bps, UCSR0A = 0x00(1���)�̶� UCSR0A ����
    UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0)|(1<<UDRIE0);
    // �ۼ��� ����, RXC���͸�Ʈ, UDR ���ͷ�Ʈ �㰡
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00)|(1<<UPM01); // ¦�� �и�Ƽ, 8��Ʈ ������, �񵿱���, ������Ʈ 1��Ʈ
    SREG |= 0x80;
}   // ¦�� �и�Ƽ, 8��Ʈ ������, �񵿱���, ������Ʈ 1��Ʈ, 1���, ��� �ӵ� 9,600 bps

void AD_Init(void)
{
    DDRF = 0x00; // Use Port F as an input port
    ADMUX = 0x00; // REFS1~0=��00�� ('AREF��), ADLAR=0, MUX0~4=��00000��('ADC0 Single��)
    ADCSRA |= (1<<ADEN)|(1<<ADSC)|(1<<ADFR)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
    // ADEN=1, ADSC=1, ADFR=1, ADPS2~0=��111�� (Prescaler=128)
}

void Init_data(void) // ���ӿ��� ���̴� ��� �����͸� �ʱ�ȭ(������ ���� ������ ��)
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
    UDR0 = txdata;  //�۽ŵ����� �Է�
}// �۽� ���ͷ�Ʈ
#pragma interrupt_handler usart0_receive:iv_USART0_RXC
void usart0_receive(void)
{
    rxdata = UDR0;  // ���ŵ����� �Է�
}// ���� �Ϸ� ���ͷ�Ʈ

unsigned int read_ADC(void)
{
    while(!(ADCSRA & (1<<ADIF))); // Polling ���
    return ADC; // ADC���� ����
}

void ext_int0_isr(void)
{
       if(jump==0&&space==3){ // ������ ���ѵǾ� ���� �ʰ�, ���� ���� ��Ȳ�� ��
       jumpping = 1; // ĳ���Ͱ� ��������
       jumpping_time = 0; // ������ �ϴ� �ð��� ����ϱ� ���� �ð��� 0���� �ʱ�ȭ
       }
}

void ext_int1_isr(void)
{
    if(space == 5 ||space == 6){ // ���� ���� ���¿��� ���¹�ư�� ������ ��
       space = 2; // �ε����� ȭ������ ����
       Init_data(); // ���ӿ� ���� ��� �����͸� �ʱ�ȭ(��� ����)
    }
}

void ext_int2_isr(void)
{
    if(space == 1) // �ʱ� ȭ���� �� ������ ���� ����
        space = 2; // �ε�ȭ������ ����
    else if(space == 5 ||space == 6){ // ���� ������ �� ������
        space = 1; // �ʱ� ȭ������ ����
        Init_data(); // ��� ������ �ʱ�ȭ
    }
}

void timer0_comp_isr(void){
    msec++; // �׻� ����ϴ� �ð� (delay�� ���)
    if(space == 3&&!pause&&!over){
        time++;   // �����߿� ��ƾ �ð��� �����ϴ� ����
        jumpping_time++;  //ĳ���Ͱ� �������� �ð��� �����ϴ� ����
        huddle_delay++;  // ��ֹ��� �����Ǵ� �ð��� �����ϴ� ����
        if(time == 1000){ // Ÿ�̸��� Ŭ���� 1msec�̱� ������ time�� 1000�� �Ǹ�
            sec++; // 1�ʸ� ����Ѵ�.
            time = 0; // time�� �ʱ�ȭ
        }
        if(sec == 60){ // ���� 60�ʰ� ������
            min++; // 1���� �߰��ϰ�
            sec = 0; // 0�ʷ� �ʱ�ȭ�Ѵ�.
        }
    }
}

void CGRAM_Set(void){

    int i;
    Byte human[]={0x04, 0x0A, 0x04, 0x1F, 0x04, 0x04, 0x0A, 0x0A}; // ĳ���� ����
    Byte jumping[]={0x04, 0x0A, 0x15, 0x0E, 0x04, 0x04, 0x0A, 0x11}; // �����ϴ� ���
    Byte hurdle[]={0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}; // ��ֹ�
    Byte white[]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // �ƹ��͵� ���� �׸�

    LCD_Comm(0x40); // 0x40��

    for(i=0;i<8;i++){
        LCD_Data(human[i]); //ĳ���͸� ����
    }

    LCD_Comm(0x48); // 0x48��

    for(i=0;i<8;i++){
        LCD_Data(jumping[i]); // �����ϴ� ����� ĳ���͸� ����
    }

    LCD_Comm(0x50); // 0x50��

    for(i=0;i<8;i++){
        LCD_Data(hurdle[i]); // ��ֹ��� ����
    }
    LCD_Comm(0x58); // 0x58��
    for(i=0;i<8;i++){
        LCD_Data(white[i]); // �ƹ��͵� ���� �׸� ����
    }

}

void delay_isr_ms(unsigned int m)
{
    msec = 0; // �켱 msec�� 0���� �ʱ�ȭ
    while(1){
        if(msec == m){ // 1msec * m�ʸ� �� �� ���� �ݺ����� �ӹ���
            msec = 0;
            break;
        }
    }
}

void main(void){

    Byte time_str[]="00:00"; // ��ƾ �ð��� ���ڿ��� ����
    Byte max_time_str[] = "00:00"; // �ְ����� ���ڿ��� ����
    unsigned int adc_data;  // ADC���� ������ ����

    Interrupt_init(); // ���ͷ�Ʈ �ʱ�ȭ
    Init_USART0(); // ��� �ʱ�ȭ
    PortInit(); // ��Ʈ �ʱ�ȭ
    LCD_Init(); // LCD �ʱ�ȭ

    CGRAM_Set(); // �׸� �׸��� ����ϱ� ���� �����͸� �ҷ�����
    Init_Timer0(); // Ÿ�̸� �㰡
    AD_Init(); // ADC �㰡
    LCD_Clear(); // LCD ȭ���� ��� ����


    DDRD = 0x00; // PORTD�� �Է¸��� ����
    DDRB = 0xFF; // PORTB�� ��¸��� ����
    PORTB = 0xFF; // PORTB�� LED�� ������ �ʱ� ���� ����

    while(1){ // ���� ����
        if(rxdata == 1){ // ���� 1�̶�� ���� ������
            time_str[4] = '0' + sec%10; // ���� ù ��° �ڸ� ����
            time_str[3] = '0' + sec/10; // ���� �� ��° �ڸ� ����
            time_str[1] = '0' + min%10; // ���� ù ��° �ڸ� ����
            time_str[0] = '0' + min/10; // ���� �� ��° �ڸ� ����
            if(space == 1){ // �ʱ� ȭ���� ��
                LCD_pos(0,0);
                LCD_STR("    JUMP  GAME");
                LCD_pos(1,0);
                LCD_STR("  START : INT2");
            }
            else if(space == 2){ // �ε�ȭ��
                LCD_Clear();
                for(int i=0;i<16;i++){ // 0.05sec * 16 = 0.8sec
                    LCD_pos(0,i);
                    LCD_Data(0x02); // �� �� �׸�
                    delay_isr_ms(50); // 0.05sec
                }
                for(int i=0;i<16;i++){ // 0.05sec * 16 = 0.8sec
                    LCD_pos(1,i);
                    LCD_Data(0x02); // �� �� �׸�
                    delay_isr_ms(50); // 0.05sec
                }
                LCD_Clear(); // LCD ȭ�� Ŭ����
                space = 3; // ���� ���� ȭ������ �Ѿ
            }
            else if(space == 3){ // ���� ���� ȭ���� ��
                adc_data=read_ADC(); // ���������� ADC�� ����
                if(adc_data<200){   // ���ص� ���� ���Ϸ� ��������
                    pause = 1; // ������ ������Ŵ
                    LCD_Clear(); // LCD ȭ�� Ŭ����
                    LCD_pos(0,4);
                    LCD_STR("PAUSE");   //PAUSE ���
                    LCD_delay(1);
                }
                else{ // ���ص� ������ ���� ��
                    pause = 0; // �������� ����
                    LCD_pos(0,3);
                    LCD_STR("TIME: "); //���ھ� ���� ���
                    LCD_pos(0,9);
                    LCD_STR(time_str);   // ��ƾ �ð� ���
                    if(over){ // ���� ���� ������ ��
                        space = 4; // ���� ���� ȭ������ �Ѿ
                        over = 0; // ���� ���� ���� 0���� �ʱ�ȭ
                    }
                    else{ // �������� ������� �� ��(���� ������ �ƴ� ��)
                        LCD_pos(1,1);
                        LCD_Data(0x00); // ���� �ڸ��� ĳ���͸� ���
                        if(jumpping&&jumpping_time<1000){ // ���� ������ �� ����, ������ �� �� 1�ʰ� ���� �ʾ��� ��
                            jump = 1; // ������ ���ѽ�Ŵ
                            LCD_pos(1,1); // ���� ĳ���Ͱ� �ִ� �ڸ���
                            LCD_Data(0x03); // �ƹ��͵� ���� ȭ������ �����
                            LCD_pos(0,1); // ������ �����Ƿ� �� ĭ ����
                            LCD_Data(0x01); // �������� ����� ĳ���͸� ���
                            PORTB = 0x00; // ������ �� �� ���� LED�� ��� �Ҵ�.
                        }
                        else if(jumpping&&jumpping_time>=1000){ // ������ �� ����, ������ �� �� 1�ʰ� ������ ��
                            jumpping = 0; // 0�� �����Ͽ� �������� �ƴ��� ���
                            LCD_pos(0,1); // ĳ���Ͱ� ������ �� �ڸ���
                            LCD_Data(0x03); // �ƹ��͵� ���� ȭ������ �����
                            LCD_pos(1,1); // ���� �ִ� �ڸ���
                            LCD_Data(0x00); // ĳ���͸� �ٽ� ���
                            PORTB = 0xFF; // ������ �������Ƿ� LED�� ��� ����.
                        }
                        else if(jumpping_time>1400){ // ������ �� ��, 1.4�ʰ� ������ ��
                            jumpping_time = 0; // ������ �� �ð��� �ʱ�ȭ �Ѵ�.
                            jump = 0; // ���� ������ �����Ѵ�.
                        }
                        if(huddle_on){ // ���� ��ֹ��� �ִ� �����̸�
                            LCD_pos(1,huddle_index); // huddle_index�� ��ġ��
                            LCD_Data(0x02); // ��ֹ��� ����Ѵ�.
                            if(huddle_delay>(250-huddle_speed*50)){ // ��ֹ��� 250-huddle_speed*50�� ������
                                LCD_pos(1,huddle_index); // ��ֹ��� ����� ��ġ
                                LCD_Data(0x03); // ��ֹ� ���
                                huddle_index--; // ��ֹ��� ��ġ�� �� ĭ ����.
                                huddle_delay = 0; // �ٽ� ��ֹ��� �����ϴ� �ð��� �����ϴ� ���� �ʱ�ȭ
                            }
                            if(huddle_index < 0) // ���� ��ֹ��� ������ ���ٸ�
                                huddle_on = 0; // ��ֹ��� ���� ���·� �����Ѵ�.
                        }
                        else{ // ��ֹ��� ���� ����
                            huddle_speed = rand()%4; // ��ֹ��� �ӵ��� rand()�Լ��� �̿��Ͽ� �����Ѵ�.(���̵� ����)
                            huddle_index = 15; // ��ֹ��� ��ġ�� �ʱ�ȭ�Ѵ�.
                            huddle_on = 1; // ��ֹ��� �ִ� ���·� �ٲ��ش�.
                        }
                        if(!jumpping && huddle_index == 1) // ���� ���������� ������ ��ֹ��� ��ġ�� ĳ������ ��ġ�� ������
                            over = 1; // ���� ���� ���·� �ٲ۴�.
                    }
                }
            }
            else if(space == 4){ // ���� ���� ������ ��
                LCD_pos(0,0);
                LCD_STR("   GAME OVER  "); // ���� ���� ���� ���
                LCD_pos(1,0);
                LCD_STR("  TIME : "); // TIME ���� ���
                LCD_pos(1,9);
                LCD_STR(time_str);   // ��ƾ �ð��� ���
                delay_isr_ms(3000); // 3�ʰ� ������
                if(min*60 + sec > max_time){ // ���� �ְ����� ����ߴٸ�
                    max_time = min*60 + sec;
                    max_time_str[4] = '0' + sec%10;
                    max_time_str[3] = '0' + sec/10;
                    max_time_str[1] = '0' + min%10;
                    max_time_str[0] = '0' + min/10; // �ְ����� �����ϴ� ���ڿ��� ��ҵ��� �ٲ���
                    space = 5; // �ְ����� �����ߴٴ� ȭ������ �Ѿ
                }
                else // �ְ����� ������� ���ߴٸ�
                    space = 6; // �ٸ� ȭ������ �Ѿ
                LCD_Clear(); // LCD Ŭ����
            }
            else if(space == 5){ // �ְ��� ��� ȭ��
                LCD_pos(0,0);
                LCD_STR(" NEW RECORD!!!!"); // �ְ����� ��Ÿ���� ���� ���
                LCD_pos(1,0);
                LCD_STR(" RECORD : ");
                LCD_pos(1,10);
                LCD_STR(max_time_str); // ����� �ְ��� ���
                PORTB = 0xFE; // �ְ����� �����ϱ� ���� LED�� ������ ������ �ϳ��� Ų��.
                for(int i=0;i<8;i++){
                    delay_ms(50);
                    PORTB = (PORTB<<1)|0x01; //����� ���� ��Ʈ�� �ѱ��.(�Ʒ���)
                }
                //������ �ٲ��ش�.
                PORTB = 0x7F; // ���� �Ʒ��� LED�� ����
                for(int i=0;i<8;i++){
                    delay_ms(50);
                    PORTB = (PORTB>>1)|0x80; //����� ���� ��Ʈ�� �ѱ��.(����)
                }
            }
            else if(space == 6){ // �ְ����� ������� ���ߴٸ�
                LCD_pos(0,0);
                LCD_STR(" RECORD : ");
                LCD_pos(0,10);
                LCD_STR(max_time_str); // ���� �ְ� ����� ����ϰ�
                LCD_pos(1,0);
                LCD_STR("RESET: INT1 or 2"); // ������ ���� �ȳ����� ���
            }
        }
        else{ // ��Ű����� 1�� ���� ���� ���¶��
            LCD_pos(0,0);
            LCD_STR("Wait"); // Wait��� ������ ���
        }
    }
}
