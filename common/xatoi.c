/*------------------------------------------------------------------------/
/  Universal string handler for user console interface
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2011, ChaN, all right reserved.
/  Copyright (C) 2014, bvernoux.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

#include "stdint.h"
#include "string.h"
#include "xatoi.h"

/* return the number of character found in string with value including only:
 '0' to '9', 'A' to 'F', 'x', 'b', ' ' */
static int str_value_pos(char **argv, int argv0_len)
{
  char car;
  int i;

  for(i = 0; i < argv0_len; i++)
  {
    car = argv[0][i];
    if( car >= '0' && car <= '9')
      continue;

    if( car >= 'A' && car <= 'F')
      continue;
    
    if( (car == 'x') ||
        (car == 'b') ||
        (car == ':') ||
        (car == ' ')
      )
      continue;
    
    /* Error other char invalid return actual pos */
    return i;
  }

  return i;
}

/*----------------------------------------------*/
/* Get a value of the string                    */
/*----------------------------------------------*/
/*  "123 -5   0x3ff 0b1111 0377  w 123a"
     ^                                  1st call returns 123 and next ptr
         ^                              2nd call returns -5 and next ptr
              ^                         3rd call returns 1023 and next ptr
                    ^                   4th call returns 15 and next ptr
                           ^            5th call returns 255 and next ptr
                                 ^      6th call fails and returns 0
                                   ^    7th call returns 123
*/
int xatoi (     /* 0:Failed, else means number of character used */
  char **argv,    /* Pointer to pointer to the string */
  long *res   /* Pointer to the valiable to store the value */
)
{
  unsigned long val;
  unsigned char c, r, s = 0;
  #define TMP_VAL_STRING_MAX_SIZE (10)
  char tmp_val_string[TMP_VAL_STRING_MAX_SIZE+1];
  char* str[] = { 0, 0 };
  int str_val_pos;
  int argv0_len;
  char* argv0;

  argv0 = &argv[0][0];
  argv0_len = strlen(argv0);
  if(argv0_len > TMP_VAL_STRING_MAX_SIZE)
  {
    return 0;
  }
  str_val_pos = str_value_pos(argv, argv0_len);
  strcpy(tmp_val_string, argv0);
  tmp_val_string[str_val_pos] = 0;
  *str = tmp_val_string;

  *res = 0;
  while ((c = **str) == ' ') (*str)++;  /* Skip leading spaces */

  if (c == '-') {   /* negative? */
    s = 1;
    c = *(++(*str));
  }

  if (c == '0')
  {
    c = *(++(*str));
    switch (c)
    {
      case 'x':   /* hexdecimal */
        r = 16; c = *(++(*str));
      break;

      case 'b':   /* binary */
        r = 2; c = *(++(*str));
      break;

      default:
        if (c <= ' ') return str_val_pos; /* single zero */
        if (c < '0' || c > '9') return 0; /* invalid char */
        r = 8;    /* octal */
      break;
    }
  } else
  {
    if (c < '0' || c > '9') return 0; /* EOL or invalid char */
    r = 10;     /* decimal */
  }

  val = 0;
  while (c > ' ')
  {
    if (c >= 'a') c -= 0x20;
    c -= '0';
    if (c >= 17)
    {
      c -= 7;
      if (c <= 9) return 0; /* invalid char */
    }
    if (c >= r) return 0;   /* invalid char for current radix */
    val = val * r + c;
    c = *(++(*str));
  }
  if (s) val = 0 - val;     /* apply sign if needed */

  *res = val;
  return str_val_pos;
}

