/*
 * process_cmd.c
 *
 *  Created on: 08-july-2025
 *      Author: Bavatharani
 */


#include "common.h"
#include "cmdlist.h"
#include "process_cmd.h"

uint8_t gbuff[100];
uint8_t gcmdbuff[100];

/*********************************************************************//**
 *  @brief Process the command
 *
 *  This function processes the received command
 *
 *  @param  cbuff[OUT]  : Receive data buffer
 *          len         : Buffer len
 *
 *  @return none
 *
 ************************************************************************/
void process_cmd(uint8_t *cbuff, uint16_t len)
{
  uint8_t index;

  if (cbuff == NULL || len == 0)
    return;

  for (index=0; index < (sizeof(ml) / sizeof(menulist)); index++)
    {
      /* Compare the command string */
      if (0 != strncmp((const char *)cbuff, (const char *)ml[index].cmd,
                       ml[index].cmdstr_len))
        continue;

      /* Call the callback function */
      if (NULL == ml[index].cb)
        continue;

      ml[index].cb(cbuff, len);
      /* Clear the buffer after processing */
      memset(cbuff, 0, len);
      break;
    }
}



/** *******************************************************************//**
 * @brief Check for the number
 *
 *  This function checks whether the character is number or not
 *
 *  @param  ch[IN] : Character
 *
 *
 *  @return TRUE   : Character is a number
 *          FLASE  : Character is not a number
 *
 **************************************************************************/
int8_t checkfornumber(char ch)
{
  if (('1'==ch) || ('2'==ch) || ('3'==ch) ||
      ('4'==ch) || ('5'==ch) || ('6'==ch) ||
      ('7'==ch) || ('8'==ch) || ('9'==ch) || ('0'==ch))
    {
      return TRUE;
    }

  return FALSE;
}


/********************************************************************//**
 * * @brief Check for hex value
 *
 *  This function checks whether the character is hex or not
 *
 *  @param  ch[IN] : Character
 *
 *
 *  @return TRUE   : Character is a hex
 *          FLASE  : Character is not a hex
 *
 ********************************************************************/
int8_t checkforhex(char ch)
{
  if (('0'==ch) || ('1'==ch) || ('2'==ch) ||
      ('3'==ch) || ('4'==ch) || ('5'==ch) ||
      ('6'==ch) || ('7'==ch) || ('8'==ch) ||
      ('9'==ch) || ('A'==ch) || ('B'==ch) ||
      ('C'==ch) || ('D'==ch) || ('E'==ch) ||
      ('F'==ch) || ('a'==ch) || ('b'==ch) ||
      ('c'==ch) || ('d'==ch) || ('e'==ch) ||
      ('f'==ch))
    {
      return TRUE;
    }

  return FALSE;
}




