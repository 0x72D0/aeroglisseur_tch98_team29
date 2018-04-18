#include "utils.h"
#include "util_29.h"

void string_concat(char* dst, char* src1, char* src2)
{
    uint32_t index = 0;
    index = string_copy(dst, src1);
    string_copy(&dst[index], src2);
}

// ajoute les donnee a transmettre et met la convertion en ascii dans un buffer
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

void replace(char * ptr, char *s, char ch, const char *repl, uint8_t s_len)
{
    int count = 0;
    const char *t;
    for (t = s; *t; t++)
    {
        count += (*t == ch);
    }
    //char *temp = string_length(s);


    uint16_t rlen = string_length(repl);
    /*for (t = s; *t; t++)
    {
        if(*t == ch)
        {
            mem_copy(ptr, repl, rlen);
            ptr += rlen;
        }
        else
        {
            *(ptr++) = *t;
        }
    }*/
    for (int i=0; i < s_len; i++)
    {
        if (s[i] == ch)
        {
            mem_copy(ptr, repl, rlen);
            ptr += rlen;
        }
        else
        {
            *(ptr++) = *(s+i);
        }
    }
}
