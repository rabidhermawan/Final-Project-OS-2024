#include "std_lib.h"

int div(int a, int b) {
  int res = 0;
  while (a >= b) {
    a -= b;
    res++;
  }
  return res;
}

int mod(int a, int b) {
  while (a >= b) a -= b;
  return a;
}

void memcpy(byte* dst, byte* src, unsigned int size) {
  unsigned int i;
  for (i = 0; i < size; i++) dst[i] = src[i];
}

unsigned int strlen(char* str) {
  unsigned int len = 0;
  while (str[len] != '\0') len++;
  return len;
}

bool strcmp(char* str1, char* str2) {
  unsigned int i = 0;
  while (str1[i] != '\0' && str2[i] != '\0') {
    if (str1[i] != str2[i]) return false;
    i++;
  }
  return str1[i] == str2[i];
}

void strcpy(char* dst, char* src) {
  unsigned int i = 0;
  while (src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

void clear(byte* buf, unsigned int size) {
  unsigned int i;
  for (i = 0; i < size; i++) buf[i] = 0;
}

void splitSentence(char* input, char* output1, char* output2, char splitter) {
  int i, j, inputLen, currentLen;
  inputLen = strlen(input); 
  j = currentLen = 0;

  for (i = 0; i < inputLen; ++i) {
    if (input[i] == splitter) {
      output1[currentLen] = '\0';
      j++;
      currentLen = 0;
      continue;
    }
    if (!j)
      output1[currentLen++] = input[i];
    else 
      output2[currentLen++] = input[i];
  }
  output2[currentLen] = '\0';
}
