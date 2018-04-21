/**
	\file manette.c
	\brief code de la manette
	\author Lucas Mongrain
    \author Temuujin Darkhantsetseg
	\date 18/04/18
*/

/******************************************************************************
Includes
******************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "driver.h"
#include "uart.h"
#include "utils.h"
#include "lcd.h"
#include "util_29.h"

/******************************************************************************
Defines
******************************************************************************/
/**
    \brief contient la valeur de l'escape byte
*/
#define ESCAPE 'A'

/**
    \brief contient la valeur signifiant "debut"
*/
#define BEGIN 'B'

/**
    \brief contient la valeur signifiant "fin"
*/
#define END 'C'

/**
    \brief contient la valeur signifiant 'A'
*/
#define VALUE 'A'

/**
    \brief contient la valeur signifiant 0
*/
#define ZERO_VALUE 'D'

/**
    \brief etat possible de la machine state
*/
typedef enum
{
    REJECT_STATE,
    ESCAPE_STATE,
    ACCEPT_STATE,
    BEGIN_STATE,
    END_STATE
}state_enum;

/**
    \brief etat possible des bytes
*/
typedef enum
{
    OLD_BYTE,
    NEW_BYTE
}byte_state_enum;

/******************************************************************************
Programme
******************************************************************************/
/**
    \brief logique de la manette
    \param[in] argc nombre d'argument passer a la fonction
    \param[in] argv tableau de string passer a la fonction
    \return valeur de fin

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

    // egal 1 si le programme enregistre les bytes;
    uint8_t in_data_write = 0;

    // regarde si le byte est un nouveau byte
    byte_state_enum byte_state = OLD_BYTE;

    // contient l'etat l'etat de la state machine
    state_enum state = REJECT_STATE;

    uart_init();
    lcd_init();
    adc_init();
    sei();
    DDRD = set_bit(DDRD, PD2);
    PORTD = clear_bit(PORTD, PD2);
    PORTD = set_bit(PORTD, PD2);

    // initialise le wifi
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

    // le message restera a l'ecran plus de 10 seconde si la connection a echouer
    lcd_clear_display();
    lcd_write_string("failed to connect");

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
            // si en reject state
            case REJECT_STATE:

                // regarde le pourcentage de la batterie
                ver = 255-adc_read(PA1);
                hor = adc_read(PA0);
                sus = adc_read(PA3);
                bat = ((adc_read(PA2)-125)*100)/38;

                // transmission des donnees a l'aeroglisseur
                memory_set(transmit_data, 0, 64);
                string_concat(transmit_data, transmit_data, "AB");
                add_data_to_string(transmit_data, hor_buffer, hor);
                add_data_to_string(transmit_data, ver_buffer, ver);
                add_data_to_string(transmit_data, sus_buffer, sus);
                string_concat(transmit_data, transmit_data, "AC");
                uint8_to_string(bat_man, bat);
                uart_put_string(transmit_data);
                _delay_ms(50);

                // regarde si le byte est nouveau
                if(byte_state)
                {
                    // si c'est un escape byte entre en escape state
                    if(byte == ESCAPE)
                    {
                        state = ESCAPE_STATE;
                        byte_state = OLD_BYTE;
                    }

                    // jete tout les autres byte
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

                //affiche les donnees receuillis
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

            // si en accept state, met les donnees dans le buffer
            case ACCEPT_STATE:
                if(byte_state)
                {
                    // si on trouve un escape byte
                    if(byte == ESCAPE)
                    {
                        state = ESCAPE_STATE;
                        byte_state = OLD_BYTE;
                    }

                    // sinon on garde la donnees
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
