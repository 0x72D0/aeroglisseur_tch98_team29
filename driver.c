/**
	\file  driver.c
	\brief Code source de fonctions qui pilotent directement du mat�riel

	\author *********************************************************
	\author ***                                                   ***
	\author ***               VOS NOMS ICI                        ***
	\author ***                                                   ***
	\author *********************************************************
	\date XXXXX
*/

/******************************************************************************
Includes
******************************************************************************/

#include <avr/io.h>
#include "driver.h"


/******************************************************************************
D�finitions de fonctions
******************************************************************************/

void adc_init(void){

	// Configuration des broches utilis�es du port A en entr�e (Entre PA0 et PA7)
    DDRA = set_bits(DDRA, 0);

	// S�lectionner la r�f�rence de tension: la tension d'alimentation (AVCC)
    ADMUX = clear_bit(ADMUX, REFS1);
    ADMUX = set_bit(ADMUX, REFS0);

	// Choisir le format du r�sultat de conversion: shift � gauche pour que
	// les 8 MSB se retrouvent dans le registre ADCH
	ADMUX = set_bit(ADMUX, ADLAR);

	// Choisir un facteur de division d'horloge (64) afin que l'horloge ait
	// une fr�quence entre 50kHz et 200kHz. Donc 8MHz/64 = 125kHz.
    ADCSRA = set_bit(ADCSRA, ADPS1);
    ADCSRA = set_bit(ADCSRA, ADPS2);
    ADCSRA = clear_bit(ADCSRA, ADPS0);

	// Activer le CAN
    ADCSRA = set_bit(ADCSRA, ADEN);
}

uint8_t adc_read(uint8_t pin_name){
    // Choisir l'entr�e analogique (broche) � convertir
    if(pin_name == PA0)
    {
        ADMUX = clear_bit(ADMUX, 1);
        ADMUX = clear_bit(ADMUX, 0);
    }
    else if(pin_name == PA1)
    {
        ADMUX = clear_bit(ADMUX, 1);
        ADMUX = set_bit(ADMUX, 0);
    }
    else if(pin_name == PA2)
    {
        ADMUX = set_bit(ADMUX, 1);
        ADMUX = clear_bit(ADMUX, 0);
    }
    else if(pin_name == PA3)
    {
        ADMUX = set_bit(ADMUX, 1);
        ADMUX = set_bit(ADMUX, 0);
    }

	// D�marrage d'une conversion
    ADCSRA = set_bit(ADCSRA, ADSC);
	// Attente de la fin de la conversion
    while(read_bit(ADCSRA, ADSC) != 0);
	// Lecture et renvoie du r�sultat
    return ADCH;
}

void servo_init(void){


	// Configuration des broches de sortie
    DDRD = set_bit(DDRD, PD5);

	// Configuration du compteur et du comparateur
    //TCCR1A = set_bits(TCCR1A, COM1A1 | WGM11);
    TCCR1A = set_bit(TCCR1A, WGM11);
    TCCR1A = set_bit(TCCR1A, COM1A1);

    TCCR1A = clear_bit(TCCR1A, COM1A0);
    TCCR1B = set_bit(TCCR1B, WGM13);
    TCCR1B = set_bit(TCCR1B, WGM12);

	// Configuration de la valeur maximale du compteur (top) � 20000
    ICR1 = 20000;
	// Initialiser la valeur du compteur � 0
    TCNT1 = 0;

	// D�marrer le compteur et fixer un facteur de division de fr�quence � 8
    TCCR1B = clear_bit(TCCR1B, CS12);
    TCCR1B = clear_bit(TCCR1B, CS10);
    TCCR1B = set_bit(TCCR1B, CS11);

}

void servo_set_a(uint8_t joystick_value)
{
    uint32_t temp = joystick_value;
    temp = ((temp*1400)/255)+800;
    OCR1A = (uint16_t)temp;
}

void servo_set_b(uint8_t angle){

	// Modification du rapport cyclique du PWM du servomoteur. Min = 600, Max = 2550

}

void pwm_init(){

	// Configuration des broches de sortie (met PB4 en entree, particularite du circuit)
    DDRB = clear_bit(DDRB, PB4);
    DDRB = set_bit(DDRB, PB3);

    // Configuration du compteur et du comparateur
    //uint8_t conf = WGM00 | WGM01;
    //uint8_t start = CS01 | CS00;
    TCCR0 = set_bit(TCCR0, WGM00);
    TCCR0 = set_bit(TCCR0, WGM01);
    //TCCR0 = set_bits(TCCR0, conf);
    //TCCR0 = clear_bits(TCCR0, ~conf & 0b01001000);
	// Demarrer le compteur et fixer un facteur de division de frequence e 1024
    TCCR0 = set_bit(TCCR0, CS00);
    TCCR0 = set_bit(TCCR0, CS02);
    TCCR0 = clear_bit(TCCR0, CS01);
    //TCCR0 = clear_bits(TCCR0, ~start & 0b00000011);

    DDRD = set_bit(DDRD, PD7);

    TCCR2 = set_bit(TCCR2, WGM20);
    TCCR2 = set_bit(TCCR2, WGM21);

    TCCR2 = set_bit(TCCR2, CS20);
    TCCR2 = set_bit(TCCR2, CS22);
    TCCR2 = set_bit(TCCR2, CS21);

}

void pwm_set_a(uint8_t duty){

	// Pour avoir un duty de 0, il faut eteindre le PWM et directement piloter la sortie e 0
	if(duty == 0){

		//Mettre 0 dans la broche PD7 (OC2) du port
		PORTB = clear_bit(PORTB, PB3);

		//Desactive le comparateur
		TCCR0 = clear_bit(TCCR0, COM01);
	}

	else{
		// Modification du rapport cyclique du PWM
        OCR0 = duty;

		//Active le comparateur
		TCCR0 = set_bit(TCCR0, COM01);
	}
}


void pwm_set_b(uint8_t duty){

	// Pour avoir un duty de 0, il faut eteindre le PWM et directement piloter la sortie e 0
	if(duty == 0){

		//Mettre 0 dans la broche PB3 (OC0) du port
		PORTD = clear_bit(PORTD, PD7);

		//Desactive le comparateur
		TCCR2 = clear_bit(TCCR2, COM21);
	}

	else{
		// Modification du rapport cyclique du PWM
        OCR2 = duty;
		//Active le comparateur
		TCCR2 = set_bit(TCCR2, COM21);
	}
}

void joystick_button_init(void){

}


bool joystick_button_read(void){

}
