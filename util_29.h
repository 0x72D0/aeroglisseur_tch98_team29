#ifndef UTIL29_H_INCLUDED
#define UTIL29_H_INCLUDED


void string_concat(char* dst, char* src1, char* src2);
void add_data_to_string(char* str, char* data_str, uint8_t data);
void memory_set(char* dst, uint8_t byte, uint32_t num);
void replace(char* dest, char* s, char ch, const char *repl, uint8_t s_len);
#endif
