/**
	\file util_29.c
	\brief Header qui defini diverse fonction utilitaire de l'equipe 29
	\author Lucas Mongrain
    \author Temuujin Darkhantsetseg
	\date 18/04/18
*/

/******************************************************************************
Includes
******************************************************************************/
#include "utils.h"
#include "util_29.h"

/******************************************************************************
Definitions des fonctions
******************************************************************************/

void string_concat(char* dst, char* src1, char* src2)
{
    uint32_t index = 0;
    index = string_copy(dst, src1);
    string_copy(&dst[index], src2);
}

void add_data_to_string(char* str, char* data_str, uint8_t data)
{
    uint8_to_string(data_str, data);
    if(data == 0)
    {
        string_concat(str, str, "AD");
    }
    else if(data == 0x61)
    {
        string_concat(str, str, "AA");
    }
    else
    {
        char data_str[2] = {0};
        data_str[0] = data;
        string_concat(str, str, data_str);
    }
}

void memory_set(char* dst, uint8_t byte, uint32_t num)
{
    for(uint32_t i = 0; i < num; i++)
    {
        dst[i] = byte;
    }
}
