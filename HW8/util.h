#ifndef __UTIL_H__
#define __UTIL_H__

int str_to_number(char *str);
void number_to_str(int number, char *str);
int char_number_split(char *str, char *char_str, char *num_str);
void report_error();
void clear_stdin_buff();

#endif