/**
     __   __                 __     __
    |__) /  \ \_/  /\  |  | |  \ | /  \
    |  \ \__/ / \ /~~\ \__/ |__/ | \__/

    Copyright (c) Roxaudio 2012. All rights reserved.
    This Source Code is the Property of Roxaudio inc. and can only be
    used in accordance with Roxaudio's Source Code License Agreement.

	\file lcd.c
	\brief driver pour un affichage LCD piloté par un HD44780
	\author Iouri Savard Colbert
	\date 28 avril 2014

*/

/******************************************************************************
Includes
******************************************************************************/

#include <avr/io.h>
#include "lcd.h"
#include <util/delay.h>


/******************************************************************************
Defines
******************************************************************************/

#define FUNCTION_SET 0b00111000

#define FALLING_EDGE()  CTRL_PORT = clear_bit(CTRL_PORT, E_PIN)
#define RISING_EDGE()   CTRL_PORT = set_bit(CTRL_PORT, E_PIN)
#define COMMAND_MODE()  CTRL_PORT = clear_bit(CTRL_PORT, RS_PIN)
#define DATA_MODE()     CTRL_PORT = set_bit(CTRL_PORT, RS_PIN)

#define MAX_INDEX (LCD_NB_ROW * LCD_NB_COL)

#define BLANK_CHAR (' ')


/******************************************************************************
Static variables
******************************************************************************/

/* LCD */
static uint8_t local_index;
static bool clear_required_flag;


/* Text */
#ifdef LCD_ENABLE_TEXT_MODULE

static char local_data_array[MAX_INDEX];

#endif

/******************************************************************************
Static prototypes
******************************************************************************/

/* hd44780 */
static void clock_data(char data);


/* lcd */
bool shift_local_index(bool foward);
uint8_t index_to_col(uint8_t index);
uint8_t index_to_row(uint8_t index);


/* text */
#ifdef LCD_ENABLE_TEXT_MODULE

void set_cursor_index(uint8_t index);
void insert(uint8_t index, const char* char_array, uint8_t length);
void remove(uint8_t index, uint8_t length);

#endif

/******************************************************************************
Global functions HD44780
******************************************************************************/

void hd44780_init(bool increment, bool cursor, bool blink){

    //On définie la valeur par défaut des ports
    DATA_PORT = FUNCTION_SET;
    CTRL_PORT = clear_bit(CTRL_PORT, RS_PIN);   //command mode
    CTRL_PORT = clear_bit(CTRL_PORT, RW_PIN);   //write mode

	// On change la direction des ports
    DATA_DDR = 0xFF;
    CTRL_DDR = set_bits(CTRL_DDR, (1 << E_PIN) | (1 << RW_PIN) | (1 << RS_PIN));

    //initial wait
    _delay_ms(10);       //10ms

    FALLING_EDGE();

    _delay_ms(5);       //4.1ms /

    RISING_EDGE();

    _delay_ms(5);       //4.1ms /

    FALLING_EDGE();

    _delay_us(100);       //100us /

    RISING_EDGE();

    _delay_us(100);       //100us /

    FALLING_EDGE();

    _delay_us(768);       //100us /

    RISING_EDGE();

    hd44780_set_entry_mode(increment);
    hd44780_set_display_control(TRUE, cursor, blink);
    hd44780_clear_display();
}


void hd44780_clear_display(){

    COMMAND_MODE();

    clock_data(0b00000001);     //Clear Display

	// Cette information n'est nulle part dans la datasheet, mais a plutôt été trouvée
	// par essaie erreur. Une bonne solution pour régler le problème sera de relire le busy
	// flag
	_delay_ms(2);

    DATA_MODE();
}


void hd44780_set_entry_mode(bool increment){

    uint8_t increment_decrement;

    if(increment == TRUE){

        increment_decrement = 0b00000010;
    }

	else{

        increment_decrement = 0b00000000;
    }

    COMMAND_MODE();

    clock_data(0b00000100 | increment_decrement);     //Entry mode set

    DATA_MODE();
}


void hd44780_set_display_control(bool display, bool cursor, bool blink){

    uint8_t dcb = 0;

    if(display == TRUE){

        dcb = set_bit(dcb, 2);
    }

    if(cursor == TRUE){

        dcb = set_bit(dcb, 1);
    }

    if(blink == TRUE){

        dcb = set_bit(dcb, 0);
    }

    COMMAND_MODE();

    clock_data(0b00001000 | dcb);     //Display on/off control

    DATA_MODE();
}


void hd44780_set_cursor_position(uint8_t col, uint8_t row){

    uint8_t address = 0;

    //On commence par ajouter le offset de la ligne
    switch(row){
    case 0:

        address += 0x00;
        break;

    case 1:

        address += 0x40;
        break;
    }

    //Puis on ajoute le offset de la colone
    address += col;

    COMMAND_MODE();

    clock_data(0b10000000 | address);     //Set DDRAM address

    DATA_MODE();
}


void hd44780_shift_cursor(hd44780_shift_e shift){

    uint8_t right_left;

    if(shift == HD44780_SHIFT_RIGHT){

        right_left = 0b00000100;
    }

    else{

        right_left = 0b00000000;
    }

    COMMAND_MODE();

    clock_data(0b00010000 | right_left);     //Cursor or display shift

    DATA_MODE();
}


void hd44780_write_char(unsigned char character){

    DATA_MODE();
    clock_data(character);
}


/******************************************************************************
Global functions LCD
******************************************************************************/

void lcd_init(void){

    hd44780_init(TRUE, TRUE, FALSE);

    local_index = 0;
	clear_required_flag = FALSE;
}


void lcd_clear_display(){

    hd44780_clear_display();

    local_index = 0;
}


void lcd_set_cursor_position(uint8_t col, uint8_t row){

    if((col >= 0) && (col < LCD_NB_COL) && (row >= 0) && (row < LCD_NB_ROW)){

        hd44780_set_cursor_position(col, row);

        local_index = col + row * LCD_NB_COL;
    }
}


void lcd_shift_cursor(lcd_shift_e shift){

    switch(shift){
    case LCD_SHIFT_RIGHT:

        shift_local_index(TRUE);

        break;

    case LCD_SHIFT_LEFT:

        shift_local_index(FALSE);

        break;

    case LCD_SHIFT_UP:

		// Si on est sur la ligne du haut
        if(index_to_row(local_index) <= 0){

			// On se rend à la fin moins une ligne
            local_index += (MAX_INDEX - LCD_NB_COL);

			clear_required_flag = TRUE;
        }

        else{

			// On recule d'une ligne
            local_index -= LCD_NB_COL;
        }

        break;

    case LCD_SHIFT_DOWN:

		// Si on est rendu à la dernière ligne
        if(index_to_row(local_index) >= LCD_NB_ROW - 1){

			// On ne garde que le numéro de colone (donc sa ramène sur la première ligne)
            local_index %= LCD_NB_COL;

			clear_required_flag = TRUE;
        }

        else{

			// On avance d'une ligne
            local_index += LCD_NB_COL;
        }

        break;

	case LCD_SHIFT_END:

		local_index = ((index_to_row(local_index) + 1) * LCD_NB_COL) - 1;

		break;

	case LCD_SHIFT_START:

		local_index = index_to_row(local_index) * LCD_NB_COL;

		break;

	case LCD_SHIFT_TOP:

		// On ne garde que le numéro de colone (donc sa ramène sur la première ligne)
		local_index %= LCD_NB_COL;

		break;

	case LCD_SHIFT_BOTTOM:

		// On se rend à la fin moins une ligne
		local_index += (MAX_INDEX - LCD_NB_COL);

		break;
	}

    hd44780_set_cursor_position(index_to_col(local_index), index_to_row(local_index));
}


void lcd_write_char(char character){

    bool unsynced;

	// Si il s'agit d'un des 32 premier caractères ascii, on s'attend à un contrôle
	// plutôt que l'affichage d'un caractère
	if(character < 0x20){

		switch (character){
		case '\n':	// 0x0A	new line
			lcd_shift_cursor(LCD_SHIFT_DOWN);
			break;
		case '\r':
			lcd_shift_cursor(LCD_SHIFT_START);
			break;
		}
	}

	else{

		if(clear_required_flag == TRUE){

			hd44780_clear_display();
			//hd44780_set_cursor_position(index_to_col(local_index), index_to_row(local_index));
			clear_required_flag = FALSE;
		}


		hd44780_write_char(character);
		unsynced = shift_local_index(TRUE);

		if(unsynced == TRUE){

			hd44780_set_cursor_position(index_to_col(local_index), index_to_row(local_index));
		}
	}
}


void lcd_write_string(const char* string){

    uint8_t index = 0;

    while(string[index] != '\0'){

        lcd_write_char(string[index]);

        index++;
    }
}


/** Text *********************************************************************/

#ifdef LCD_ENABLE_TEXT_MODULE

void text_init(void){

    uint8_t i;

    lcd_init(FALSE);

    for(i = 0; i < LCD_NB_ROW * LCD_NB_COL; i++){

        local_data_array[i] = BLANK_CHAR;
    }
}


void text_clear_display(void){

    uint8_t i;

    lcd_clear_display();

    for(i = 0; i < MAX_INDEX; i++){

        local_data_array[i] = BLANK_CHAR;
    }
}


void text_set_cursor_position(uint8_t col, uint8_t row){

    lcd_set_cursor_position(col, row);
}


void text_shift_cursor(lcd_shift_e shift){

    lcd_shift_cursor(shift);
}


void text_write_char(char character){

    uint8_t local_index_backup;

    local_index_backup = local_index;

    insert(local_index, &character, 1);

    local_index_backup++;

    /* Si le curseur est passé la dernière position */
    if(local_index_backup >= MAX_INDEX){

        text_line_break();
    }

    else{

        set_cursor_index(local_index_backup);
    }
}


void text_line_break(void){

    uint8_t local_index_backup;
    uint8_t offset;
    char space_array[LCD_NB_COL];

    uint8_t i;

    /* Si le curseur est sur la dernière ligne ou même sur la troisième ligne virtuelle */
    if(index_to_row(local_index) >= LCD_NB_ROW - 1){

        local_index_backup = local_index;

        /* On commence par supprimer la première ligne */
        remove(0, LCD_NB_COL);

        set_cursor_index(local_index_backup - LCD_NB_COL);
    }

    if(local_index < LCD_NB_COL){

        offset = LCD_NB_COL - index_to_col(local_index);

        /* On rempli l'array d'espace */
        for(i = 0; i < offset; i++){

            space_array[i] = '\n';
        }

        /* On insert cet array */
        insert(local_index, space_array, offset);

        set_cursor_index(LCD_NB_COL);
    }
}


void text_del_last_char(void){

    uint8_t local_index_backup;
    uint8_t length;

    local_index_backup = local_index;
    length = 1;

    while((local_index > 0) && (local_data_array[local_index - 1] == '\n')){

        local_index--;
        length++;
    }

    /* Si aucun sauts de lignes n'ont été découverts */
    if(local_index_backup == local_index){

        /* Il faut quand même supprimer un caractère */
        local_index--;
    }

    /* On met à jour notre référence */
    local_index_backup = local_index;

    remove(local_index, length);

    set_cursor_index(local_index_backup);
}


void text_del_current_char(void){

    uint8_t local_index_backup;
    uint8_t length;

    local_index_backup = local_index;
    length = 1;

    while((local_index + length < MAX_INDEX) && (local_data_array[local_index + length] == '\n')){

        length++;
    }

    remove(local_index, length);

    set_cursor_index(local_index_backup);
}


void text_write_string(const char* string){

    uint8_t local_index_backup;
    uint8_t index = 0;
    uint8_t i = 0;

    while(string[i] != '\0'){

        /* Si on trouve un line break */
        if(string[i] == '\n'){

            local_index_backup = local_index;

            insert(local_index, &(string[index]), i - index);

            set_cursor_index(local_index_backup + i);

            text_line_break();

            index = i + 1;
        }

        i++;
    }

    local_index_backup = local_index;

    insert(local_index, &(string[index]), i - index);

    set_cursor_index(local_index_backup + i);
}


#endif


/******************************************************************************
Static functions
******************************************************************************/

/* hd44780 */
void clock_data(char data){

    DATA_PORT = data;

    _delay_us(50);

    FALLING_EDGE();

    _delay_us(50);

    RISING_EDGE();
}


/* lcd */

uint8_t index_to_col(uint8_t index){

    return index % LCD_NB_COL;
}


uint8_t index_to_row(uint8_t index){

    return index / LCD_NB_COL;
}

bool shift_local_index(bool foward){

    uint8_t previous_row;

    previous_row = index_to_row(local_index);

    /* Si on est dans le sens foward */
    if(foward == TRUE){

        /* Si on est à la fin */
        if(local_index >= MAX_INDEX - 1){

			local_index = 0;
        }

        else{

            local_index++;
        }
    }

    /* Si on est dans le sens décrémental foward ou incrémental backward*/
    else{

        /* Si on est au début */
        if(local_index <= 0){

            local_index = MAX_INDEX - 1;
        }

        else{

            local_index--;
        }
    }

    /* Si la row actuelle ne correspond pas à l'ancienne il va falloir manuellement
    déplacer le curseur */
    return (previous_row != index_to_row(local_index));
}


/* text */
#ifdef LCD_ENABLE_TEXT_MODULE

void set_cursor_index(uint8_t index){

	if((index >= 0) && (index < MAX_INDEX)){

		hd44780_set_cursor_position(index % LCD_NB_COL, index / LCD_NB_COL);

		local_index = index;
	}
}

void insert(uint8_t index, const char* char_array, uint8_t length){

    /* Très exceptionellement, le type doit être signé pour empêcher la dernière
    boucle for de spinner à l'infini */
    int8_t i;

    if(index + length < MAX_INDEX){

        set_cursor_index(MAX_INDEX - 1);

        lcd_set_entry_mode(FALSE);

        for(i = (MAX_INDEX - 1); i >= index; i--){

            /* Décalage des caractères */
            if(i >= index + length){

                local_data_array[i] = local_data_array[i - length];
            }

            /* Insertion des nouveaux caractères */
            else{

                local_data_array[i] = char_array[i - index];
            }

            lcd_write_char(local_data_array[i]);
        }

        lcd_set_entry_mode(TRUE);
    }
}


void remove(uint8_t index, uint8_t length){

    uint8_t i;

    set_cursor_index(index);

    /* Décalage des caractères */
    for(i = index; i < MAX_INDEX; i++){

        if(i < MAX_INDEX - length){

            local_data_array[i] = local_data_array[i + length];
        }

        else{

            local_data_array[i] = BLANK_CHAR;
        }

        lcd_write_char(local_data_array[i]);
    }
}

#endif
