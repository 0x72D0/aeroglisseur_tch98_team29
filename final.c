#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "driver.h"
#include "utils.h"
#include "lcd.h"

int main(int argc, char** argv)
{
    char str[16];
    uint8_t joystick_value;
    uint8_t adc;
    adc_init();
    lcd_init();
    pwm_init();
    servo_init();

    while(1)
    {
        adc = adc_read(PA0);
        if(adc <= 131) //valeur initial de potentiometre
        {
            adc = 0;
        }
        adc *= 2;
        pwm_set_a(adc);

        joystick_value = adc_read(PA1);
        servo_set_a(joystick_value);
        uint16_to_string(str, OCR1A);
        lcd_clear_display();
        lcd_write_string(str);
        _delay_ms(100);
    }
}
