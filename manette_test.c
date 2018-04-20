/**
	\file manette_test.c
	\brief code de test de la manette
	\author Lucas Mongrain
	\date 20/04/18
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
Programme
******************************************************************************/
/**
    \brief code de test pour la manette
    \param[in] argc nombre d'argument passer a la fonction
    \param[in] argv tableau de string passer a la fonction
    \return valeur de fin
*/
int main(int argc, char** argv)
{
    char result[32];
    char hor_buffer[4];
    char ver_buffer[4];
    char sus_buffer[4];
    char bat_man[4];
    char transmit_data[64];

    uint8_t ver;
    uint8_t hor;
    uint8_t sus;
    uint8_t bat;

    lcd_init();
    adc_init();

    while(1)
    {
        ver = (255-adc_read(PA1));
        hor = 255-adc_read(PA0);
        sus = adc_read(PA3);
        bat = ((adc_read(PA2)-125)*100)/38;

        memory_set(transmit_data, 0, 64);
        string_concat(transmit_data, transmit_data, "AB");
        add_data_to_string(transmit_data, hor_buffer, hor);
        add_data_to_string(transmit_data, ver_buffer, ver);
        add_data_to_string(transmit_data, sus_buffer, sus);
        string_concat(transmit_data, transmit_data, "AC");
        uint8_to_string(bat_man, bat);
        _delay_ms(50);

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
        string_concat(result, result, "NONE");

        lcd_clear_display();
        lcd_write_string(result);
    }
}
