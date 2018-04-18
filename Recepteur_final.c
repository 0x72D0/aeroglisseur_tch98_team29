#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.h"
#include "lcd.h"
#include "utils.h"
#include "util_29.h"
#include "driver.h"

#define ESCAPE 'A'
#define BEGIN 'B'
#define END 'C'
#define VALUE 'A'
#define ZERO_VALUE 'D'

#define REJECT_STATE 0
#define ESCAPE_STATE 1
#define ACCEPT_STATE 2
#define BEGIN_STATE 3
#define END_STATE 4

#define OLD_BYTE 0
#define NEW_BYTE 1

int main(int argc, char** argv)
{
    uint8_t index = 0;
    uint8_t byte = 0;

    // equal 1 if the program is registering byte;
    uint8_t in_data_write = 0;

    // check if the byte is a new byte
    uint8_t byte_state = OLD_BYTE;

    // contain the state of the program
    uint8_t state = REJECT_STATE;

    //check if AT+CIPSEND is send once
    uint8_t config_wifi = 0;

    uint8_t bat = 0;

    char transmit_data[64];
    char data[64];
    char result[32];
    char hor[4];
    char ver[4];
    char sus[4];
    char bat_pourcentage[4];

    sei();
    lcd_init();
    adc_init();
    uart_init();
    pwm_init();
    servo_init();

    // initialise wifi
    OSCCAL = OSCCAL+6; // atmega avec marque

    lcd_write_string("connecting...");

    DDRD = set_bit(DDRD, PD2);
    PORTD = clear_bit(PORTD, PD2);
    PORTD = set_bit(PORTD, PD2);
    _delay_ms(1000);
    uart_put_string("AT+CWMODE_DEF=2\r\n");
    _delay_ms(1000);
    uart_flush();
    uart_put_string("AT+CWSAP_DEF=\"THING\",\"f8aa2328679b\",1,3\r\n");
    _delay_ms(2000);
    uart_flush();
    uart_put_string("AT+CIPMODE=1\r\n");
    _delay_ms(1000);
    uart_flush();
    uart_put_string("AT+CIPSTART=\"UDP\",\"0.0.0.0\",31337,1337\r\n");
    _delay_ms(1000);
    uart_flush();
    uart_put_string("AT+CIPSTATUS\r\n");
    _delay_ms(1000);
    uart_flush();
    lcd_clear_display();
    lcd_write_string("waiting for data");
    //uart_put_string("AT+CIPSEND\r\n");
    //_delay_ms(250);



    while(1)
    {
        // if the rx buffer is not empty
        if(uart_is_rx_buffer_empty() == FALSE && !byte_state)
        {
            byte = uart_get_byte();
            byte_state = NEW_BYTE;
        }

        // state machine
        switch(state)
        {
            // if on a reject state
            case REJECT_STATE:
                // check if the byte is new
                if(byte_state)
                {
                    // if it's a escape byte enter in escape state
                    if(byte == ESCAPE)
                    {
                        state = ESCAPE_STATE;
                        byte_state = OLD_BYTE;
                    }

                    // dump all the other byte
                    else
                    {
                        byte_state = OLD_BYTE;
                    }
                }
                break;

            // if on the escape state
            case ESCAPE_STATE:
                if(byte_state)
                {
                    switch(byte)
                    {
                        // if the next byte is 'B'
                        case BEGIN:
                            if(!in_data_write)
                            {
                                state = BEGIN_STATE;
                                byte_state = OLD_BYTE;
                            }
                            else
                            {
                                state = REJECT_STATE;
                            }
                            break;
                        // if the next byte is 'C'
                        case END:
                            if(in_data_write)
                            {
                                state = END_STATE;
                                byte_state = OLD_BYTE;
                            }
                            else
                            {
                                state = REJECT_STATE;
                            }
                            break;
                        // if the next byte is 'A'
                        case VALUE:
                            if(in_data_write)
                            {
                                data[index] = VALUE;
                                index++;
                                byte_state = OLD_BYTE;
                                state = ACCEPT_STATE;
                            }
                            else
                            {
                                state = REJECT_STATE;
                            }
                            break;
                        case ZERO_VALUE:
                            if(in_data_write)
                            {
                                data[index] = 0;
                                index++;
                                byte_state = OLD_BYTE;
                                state = ACCEPT_STATE;
                            }
                            else
                            {
                                state = REJECT_STATE;
                            }
                            break;
                        // if the next byte is garbage
                        default:
                            byte_state = OLD_BYTE;
                            state = REJECT_STATE;
                            in_data_write = 0;
                            break;
                    }
                }
                break;
            // if on begin state, initialize the data
            case BEGIN_STATE:
                state = ACCEPT_STATE;
                in_data_write = 1;
                index = 0;

                memory_set(data, 0, 64);
                break;
            // if on end state, print the data
            case END_STATE:
                state = REJECT_STATE;
                in_data_write = 0;
                data[index] = 0;

                //do the program logic
                uint8_to_string(hor, data[0]);
                uint8_to_string(ver, data[1]);
                uint8_to_string(sus, data[2]);

                memory_set(result, 0, 32);

                string_concat(result, result, "H");
                string_concat(result, result, hor);
                string_concat(result, result, "/V");
                string_concat(result, result, ver);
                string_concat(result, result, "/S");
                string_concat(result, result, sus);
                string_concat(result, result, "\r\n");
                string_concat(result, result, "A:");
                string_concat(result, result, bat_pourcentage);
                string_concat(result, result, "%");

                servo_set_a(data[0]);
                pwm_set_b(data[1]);
                pwm_set_a(data[2]);

                lcd_clear_display();
                lcd_write_string(result);

                // transmet le pourcentage de la batterie
                bat = ((adc_read(PA0)-107)*100)/26;

                if(config_wifi == 0)
                {
                    uart_put_string("AT+CIPSEND\r\n");
                    _delay_ms(500);
                    uart_flush();
                    config_wifi = 1;
                }

                memory_set(transmit_data, 0, 64);
                string_concat(transmit_data, transmit_data, "AB");
                add_data_to_string(transmit_data, bat_pourcentage, bat);
                string_concat(transmit_data, transmit_data, "AC");
                uart_put_string(transmit_data);
                break;
            // if on accept state, put data in the buffer
            case ACCEPT_STATE:
                if(byte_state)
                {
                    // if we find an escape byte
                    if(byte == ESCAPE)
                    {
                        state = ESCAPE_STATE;
                        byte_state = OLD_BYTE;
                    }

                    // or write the data
                    else
                    {
                        data[index] = byte;
                        index++;
                        byte_state = OLD_BYTE;
                    }
                }
                break;
            default:
                lcd_clear_display();
                lcd_write_string("error 101");
                return 101;
                break;
        }
    }
}
