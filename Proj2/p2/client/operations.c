#include "operations.h"

#include <string.h>
#include <stdio.h>

void int_to_buffer(unsigned int num, char* buffer) {

  sprintf(buffer, "%u", num);
  size_t length = strlen(buffer);

  if (length < 4) {
    memset(buffer + length, '\0', 4 - length);
  }
}