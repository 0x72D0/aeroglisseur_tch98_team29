/**
	\file aero_drag.c
	\brief code de la configuration "drag" de l'aeroglisseur
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
    \brief contient la valeur de l'angle au centre de l'aeroglisseur
*/
#define CENTER 1580UL

/**
    \brief contient l'angle de droite
*/
#define ANGLE_D 100UL

/**
    \brief contient l'angle de gauche
*/
#define ANGLE_G 150UL

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
    \brief logique de l'aeroglisseur
    \param[in] argc nombre d'argument passer a la fonction
    \param[in] argv tableau de string passer a la fonction
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
    uint32_t servo_value = 0;

    // egal 1 si le programme enregistre des bytes;
    uint8_t in_data_write = 0;

    // regarde si le byte et un vieu byte
    byte_state_enum byte_state = OLD_BYTE;

    // contient l'etat de la state machine
    state_enum state = REJECT_STATE;

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

    //initialise les composante
    servo_set_a(CENTER);
    pwm_set_a(0);
    pwm_set_b(0);

    // initialise la chip wifi
    OSCCAL = OSCCAL+6; // atmega avec marque

    lcd_write_string("setup wifi...");

    DDRD = set_bit(DDRD, PD2);
    PORTD = clear_bit(PORTD, PD2);
    PORTD = set_bit(PORTD, PD2);

    uart_put_string("AT+CWMODE_DEF=3\r\n");
    _delay_ms(500);
    uart_flush();
    uart_put_string("AT+CWSAP_DEF=\"THING\",\"f8aa2328679b\",1,3\r\n");
    _delay_ms(500);
    uart_flush();
    uart_put_string("AT+CIPMODE=1\r\n");
    _delay_ms(500);
    uart_flush();
    uart_put_string("AT+CIPSTART=\"UDP\",\"0.0.0.0\",31337,1337\r\n");
    _delay_ms(500);
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
                servo_value = (uint8_t)data[0];
                // equation de droite
                if(servo_value > 126)
                {
                    servo_value = ((servo_value*(ANGLE_D*2UL))/255UL)+(CENTER-ANGLE_D);
                }
                else
                {
                    servo_value = ((servo_value*(ANGLE_G*2UL))/255UL)+(CENTER-ANGLE_G);
                }

                servo_set_a((uint16_t)servo_value);

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
