/*
 * This source file provides services for the physical manipulation of the serial stream
 * to and from the modem
 */
#include "Arduino.h"
#include "A6Modem.h"
#include <TimerOne.h>

void HW_SERIAL_EVENT();
char comm_buf[COMM_BUF_LEN];  // communication buffer +1 for 0x00 termination
int inlevel, outlevel;        // data in comm_buf

bool A6GPRSDevice::begin(long baudrate)
{
  bool rc = false;
  HW_SERIAL.begin(A6_BAUDRATE);
  inlevel = outlevel = 0;
  //inSetup = true;
  enableDebug = false;
  HWReset();
  if (waitresp("+CREG: 1\r\n",20000))
  {
    HW_SERIAL.print(F("ATE0\r"));
    rc = waitresp("OK\r\n",1000);
  }
  return rc;
}

bool A6GPRSDevice::begin()
{
  return begin(A6_BAUDRATE);
}

A6GPRSDevice::A6GPRSDevice(){
	rxcount = 0;
	txcount = 0;
}

A6GPRSDevice::~A6GPRSDevice(){
}
void push(char c)
{
//  DebugWrite(c);
  comm_buf[inlevel++] = c;
  if (inlevel == COMM_BUF_LEN)  // handle wrap around
    inlevel = 0;
}

char A6GPRSDevice::pop()
{
  char c;
  if (inlevel == outlevel)
    c = -1;
  else
  {
    c = comm_buf[outlevel++];
	DebugWrite(c);
    if (outlevel == COMM_BUF_LEN)  // handle wrap around
      outlevel = 0;
    if (inlevel == outlevel)
      inlevel = outlevel = 0;
  }
  return c;
}

void A6GPRSDevice::ModemWrite(char c)
{
  HW_SERIAL.write(c);
}

/*
 * Check incoming buffer for desired string, return true if found in time else false
 * Only use this version during setup time as serialEvent not yet available
 */
bool A6GPRSDevice::waitresp(char const *response_string,int32_t timeout)
{
  int lengthtotest = strlen(response_string);
  char *nextChar = response_string;
  bool started = false;
  uint32_t TimeOut = millis() + timeout;
  while (TimeOut > millis() && lengthtotest>0)
  {
//    if (inSetup && HW_SERIAL.available())
  //    push(HW_SERIAL.read());
    HW_SERIAL_EVENT();
    // get next char from buffer, if no match discard, if match decrement lengthtotest & get next character
    char c = pop();
    if (c != -1)
    {
      if (c == *nextChar)
      {
 //       DebugWrite('[');
        lengthtotest--;
        nextChar++;
        started = true;
      }
      else if (started)
      {
 //       DebugWrite(']');
        lengthtotest = strlen(response_string);
        nextChar = response_string;
        started = false;
      }
    }
  }
  return TimeOut > millis();  // finished before time is up
}

bool A6GPRSDevice::GetLineWithPrefix(char const *px,char *outbuf, int bufsize,int32_t timeout)
{
  int lengthtotest = strlen(px);
  char *nextChar = px;
  uint32_t TimeOut = millis() + timeout;
  enum epx {BEFORE_PREFIX,IN_PREFIX,AFTER_PREFIX} eepx = BEFORE_PREFIX;
  bool alldone = false;
  if (px == NULL)
  {
    eepx = AFTER_PREFIX;
  }
  while (TimeOut > millis() && !alldone)
  {
    HW_SERIAL_EVENT();
    // get next char from buffer, if no match discard, if match decrement lengthtotest & get next character
    char c = pop();
    if (c!= -1)
    {
      switch (eepx)
      {
        case BEFORE_PREFIX:
          if (c == *nextChar)
          {
            eepx = IN_PREFIX;
            lengthtotest--;
            nextChar++;
          }
          break;
        case IN_PREFIX:
          if (c == *nextChar)
          {
            lengthtotest--;
            nextChar++;
            if (lengthtotest == 0)
              eepx = AFTER_PREFIX;
          }
          else
          {
            eepx = BEFORE_PREFIX;
            nextChar = px;
            lengthtotest = strlen(px);
          }
          break;
        case AFTER_PREFIX:
          *outbuf++ = c;
          bufsize--;
          if (c == 0x0d || c == 0x0a || c == 0 || bufsize == 0)
          {
            *outbuf = 0; // add end marker
            alldone = true;
          }
          break;
      }
    }
  } 
  return TimeOut > millis();  // still have time left 
}

void A6GPRSDevice::RXFlush()
{
  volatile char c;
  while (HW_SERIAL.available())
    c = HW_SERIAL.read();
  inlevel = outlevel = 0;
}

void HW_SERIAL_EVENT() {
  while (HW_SERIAL.available())
    push((char)HW_SERIAL.read());
}
