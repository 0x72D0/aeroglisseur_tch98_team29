#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "driver.h"
#include "uart.h"
#include "utils.h"
#include "lcd.h"
#include "util_29.h"

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
    char data[64];
    char result[32];
    char hor_buffer[4];
    char ver_buffer[4];
    char sus_buffer[4];
    char bat_man[4];
    char bat_aero[4];
    char transmit_data[64];

    uint8_t ver;
    uint8_t hor;
    uint8_t sus;
    uint8_t bat;

    uint8_t index = 0;
    uint8_t byte = 0;

    // equal 1 if the program is registering byte;
    uint8_t in_data_write = 0;

    // check if the byte is a new byte
    uint8_t byte_state = OLD_BYTE;

    // contain the state of the program
    uint8_t state = REJECT_STATE;

    uart_init();
    lcd_init();
    adc_init();
    sei();
    DDRD = set_bit(DDRD, PD2);
    PORTD = clear_bit(PORTD, PD2);
    PORTD = set_bit(PORTD, PD2);

    //DDRB = clear_bit(DDRB, PD3);
    OSCCAL = OSCCAL + 8;
    lcd_write_string("connecting...");
    uart_put_string("AT+CWMODE_DEF=1\r\n");
    _delay_ms(1000);
    uart_flush();
    uart_put_string("AT+CWJAP_DEF=\"THING\",\"f8aa2328679b\"\r\n");
    _delay_ms(5000);
    uart_flush();
    uart_put_string("AT+CIPMODE=1\r\n");
    _delay_ms(1000);
    uart_flush();
    uart_put_string("AT+CIPSTART=\"UDP\",\"192.168.4.1\",1337,31337\r\n");
    _delay_ms(10000);
    uart_flush();
    uart_put_string("AT+CIPSTATUS\r\n");
    _delay_ms(1000);
    uart_flush();
    uart_put_string("AT+CIPSEND\r\n");
    _delay_ms(1000);
    uart_flush();
    lcd_clear_display();
    lcd_write_string("failed to connect");

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
                // regarde le pourcentage de la batterie
                ver = adc_read(PA1);
                hor = adc_read(PA0);
                sus = adc_read(PA3);
                bat = ((adc_read(PA2)-125)*100)/38;

                memory_set(transmit_data, 0, 64);
                string_concat(transmit_data, transmit_data, "AB");
                add_data_to_string(transmit_data, hor_buffer, hor);
                add_data_to_string(transmit_data, ver_buffer, ver);
                add_data_to_string(transmit_data, sus_buffer, sus);
                string_concat(transmit_data, transmit_data, "AC");
                uint8_to_string(bat_man, bat);
                uart_put_string(transmit_data);
                _delay_ms(50);
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
                uint8_to_string(bat_aero, data[0]);

                memory_set(result, 0, 32);

                string_concat(result, result, "H");
                string_concat(result, result, hor_buffer);
                string_concat(result, result, "/V");
                string_concat(result, result, ver_buffer);
                string_concat(result, result, "/S");
                string_concat(result, result, sus_buffer);
                string_concat(result, result, "\n\r");
                string_concat(result, result, "M:");
                string_concat(result, result, bat_man);
                string_concat(result, result, "%/");
                string_concat(result, result, "A:");
                string_concat(result, result, bat_aero);
                string_concat(result, result, "%");

                lcd_clear_display();
                lcd_write_string(result);
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
