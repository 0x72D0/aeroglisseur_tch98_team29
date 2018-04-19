/**
	\file aero.c
	\brief code de l'aeroglisseur
	\author Lucas Mongrain
	\date 18/04/18
*/

/******************************************************************************
Includes
******************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.h"
#include "lcd.h"
#include "utils.h"
#include "util_29.h"
#include "driver.h"

/******************************************************************************
Defines
******************************************************************************/
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

/******************************************************************************
Programme
******************************************************************************/

/**
    \brief logique de l'aeroglisseur
    \return la valeur de fin
    
    l'aeroglisseur utilise un autre protocole que celui decrit dans le cour de tch98, le protocole utilise
    le principe de l'escape byte, l'escape byte decider par l'equipe est le 'A', ainsi "AB" -> debut de la communication,
    "AC" -> fin de la communication, "AA" -> byte de donnee A, "AD" -> byte 0. les autres donnees sont envoyer en "raw byte"
    Donc, une chaine transmit a l'aeroglisseur peut ressembler a "ABAAEF8AC". L'aeroglisseur et la manette agisse comme des
    transmetteur/recepteur, le recepteur est la finite state machine et, a l'interrieur de la manette, le message est transmit
    dans les temps ou rien est receptionner. Tandis qu'a l'interieur de l'aeroglisseur le message est transmit a la fin de la
    reception d'un message (pour profiter du 50ms de la manette)
*/
int main(int argc, char** argv)
{
    uint8_t index = 0;
    uint8_t byte = 0;
    uint8_t bat = 0;

    // egal 1 si le programme enregistre des bytes;
    uint8_t in_data_write = 0;

    // regarde si le byte et un vieu byte
    uint8_t byte_state = OLD_BYTE;

    // contient l'etat de la state machine
    uint8_t state = REJECT_STATE;

    // s'assure que AT+SEND est envoyer une seul fois
    uint8_t config_wifi = 0;

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

    // initialise le wifi wifi
    OSCCAL = OSCCAL+6; // atmega avec marque

    lcd_write_string("setup wifi...");

    DDRD = set_bit(DDRD, PD2);
    PORTD = clear_bit(PORTD, PD2);
    PORTD = set_bit(PORTD, PD2);

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



    while(1)
    {
        // si le rx buffer n'est pas vide
        if(uart_is_rx_buffer_empty() == FALSE && !byte_state)
        {
            byte = uart_get_byte();
            byte_state = NEW_BYTE;
        }

        // state machine
        switch(state)
        {
            // si a l'etat de rejet des bytes
            case REJECT_STATE:
                // regarde si le byte est vide
                if(byte_state)
                {
                    // si le byte est l'escape byte, change l'etat en escape state
                    if(byte == ESCAPE)
                    {
                        state = ESCAPE_STATE;
                        byte_state = OLD_BYTE;
                    }

                    // jete tout les autres bytes
                    else
                    {
                        byte_state = OLD_BYTE;
                    }
                }
                break;

            // si en escape state
            case ESCAPE_STATE:
                if(byte_state)
                {
                    switch(byte)
                    {
                        // si le prochain byte est 'B'
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

                        // si le prochain byte est 'C'
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

                        // si le prochain byte est 'A'
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

                        // si le prochain byte est 'D'
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

                        // si le prochain byte est du garbage
                        default:
                            byte_state = OLD_BYTE;
                            state = REJECT_STATE;
                            in_data_write = 0;
                            break;
                    }
                }
                break;

            // si en begin state, initialise les donnees
            case BEGIN_STATE:
                state = ACCEPT_STATE;
                in_data_write = 1;
                index = 0;

                memory_set(data, 0, 64);
                break;

            // si en end state, utilise les donnees
            case END_STATE:
                state = REJECT_STATE;
                in_data_write = 0;
                data[index] = 0;

                // afficher au lcd pour debugging
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

                // execute la logique du programme
                pwm_set_b(data[1]);
                pwm_set_a(data[2]);

                lcd_clear_display();
                lcd_write_string(result);

                // transmet le pourcentage de la batterie
                bat = ((adc_read(PA0)-107)*100)/26;

                // envoie AT+CIPSEND si pas encore envoyer
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

            // si en accept state, met les donnees dans un buffer
            case ACCEPT_STATE:
                if(byte_state)
                {
                    // si on trouve un escape byte
                    if(byte == ESCAPE)
                    {
                        state = ESCAPE_STATE;
                        byte_state = OLD_BYTE;
                    }

                    // sinon on ecrit le data
                    else
                    {
                        data[index] = byte;
                        index++;
                        byte_state = OLD_BYTE;
                    }
                }
                break;

            // si l'evenement qui arrive n'est pas gerer
            default:
                lcd_clear_display();
                lcd_write_string("error 101");
                return 101;
                break;
        }
    }
}
