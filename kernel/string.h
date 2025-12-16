#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stddef.h>

// String length
size_t strlen(const char* str);

// String copy
char* strcpy(char* dest, const char* src);

// String copy with length limit
char* strncpy(char* dest, const char* src, size_t n);

// String compare
int strcmp(const char* s1, const char* s2);

// String compare with length limit
int strncmp(const char* s1, const char* s2, size_t n);

// String concatenate
char* strcat(char* dest, const char* src);

// Find character in string
char* strchr(const char* str, int c);

// Find substring in string
char* strstr(const char* haystack, const char* needle);

// Memory set
void* memset(void* ptr, int value, size_t num);

// Memory copy
void* memcpy(void* dest, const void* src, size_t num);

// Memory move (handles overlapping)
void* memmove(void* dest, const void* src, size_t num);

// Memory compare
int memcmp(const void* ptr1, const void* ptr2, size_t num);

// Integer to string
void itoa(int value, char* str, int base);

// Unsigned integer to string
void utoa(unsigned int value, char* str, int base);

// String to integer
int atoi(const char* str);

// Check if character is digit
int isdigit(int c);

// Check if character is alphabetic
int isalpha(int c);

// Check if character is alphanumeric
int isalnum(int c);

// Check if character is whitespace
int isspace(int c);

// Convert to uppercase
int toupper(int c);

// Convert to lowercase
int tolower(int c);

#endif // STRING_H
