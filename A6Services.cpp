#include "Arduino.h"
#include "A6Services.h"
#include <ctype.h>

A6GPRS gsm;
A6GPRS::A6GPRS(){
  cid = 1;
  CIPstatus = IP_STATUS_UNKNOWN;
};
A6GPRS::~A6GPRS(){};

static char *statusnames[] = {"IP INITIAL","IP START","IP CONFIG","IP IND","IP GPRSACT","IP STATUS","TCP/UDP CONNECTING","IP CLOSE","CONNECT OK"};
//const char* const status_names[] PROGMEM = {"IP INITIAL","IP START","IP CONFIG","IP IND","IP GPRSACT","IP STATUS","TCP/UDP CONNECTING","IP CLOSE","CONNECT OK"};

char tempbuf[100];

bool A6GPRS::getIMEI(char imei[])
{
  bool rc;
  gsm.RXFlush();
  gsm.modemPrint(F("AT+EGMR=2,7\r"));
  rc = GetLineWithPrefix("+EGMR:",imei,20,500);
  waitresp("OK\r\n",500);
  return rc;
}

bool A6GPRS::getCIMI(char cimi[])
{
  bool rc;
  gsm.RXFlush();
  gsm.modemPrint(F("AT+CIMI\r"));
  waitresp("\r\n",500);
  rc = GetLineWithPrefix(NULL,cimi,20,500);
  waitresp("OK\r\n",500);
  return rc;
}
bool A6GPRS::getRTC(char rtc[])
{
  bool rc;
  gsm.RXFlush();
  gsm.modemPrint(F("AT+CCLK\r"));
  waitresp("\r\n",500);
  rc = GetLineWithPrefix("+CCLK:",rtc,30,500);
  waitresp("OK\r\n",500);
  return rc;
}
bool A6GPRS::setRTC(char rtc[])
{
  gsm.RXFlush();
  gsm.modemPrint(F("AT+CCLK=\""));
  gsm.modemPrint(rtc);
  gsm.modemPrint("\"\r");
  return waitresp("OK\r\n",500);
}

enum A6GPRS::eCIPstatus A6GPRS::getCIPstatus()
{
  enum eCIPstatus es = IP_STATUS_UNKNOWN;
  gsm.RXFlush();
  gsm.modemPrint(F("AT+CIPSTATUS\r"));
  if (GetLineWithPrefix("+IPSTATUS:",tempbuf,50,1000))
  {
    char *s = tempbuf;  // skip over whitespace
    while (isspace(*s))
      s++;
    int i;
    for (i=0;i<9;i++)
      if (strncmp(s,statusnames[i],strlen(statusnames[i])) == 0)
      {
        es = i;
        break;
      }
  }
  waitresp("OK\r\n",2000);  // trailing stuff
  CIPstatus = es;
  return es;
}

char *A6GPRS::getCIPstatusString(enum eCIPstatus i)
{
  return statusnames[i];
}

char *A6GPRS::getCIPstatusString()
{
  return statusnames[getCIPstatus()];
}

bool A6GPRS::startIP(char apn[],char user[],char pwd[])  // apn, username, password
{
  bool rc = false;
  cid = 1; //gsm.getcid();
  gsm.RXFlush();
  if (CIPstatus != IP_INITIAL)
  {
    gsm.modemPrint(F("AT+CIPCLOSE\r"));
    waitresp("OK\r\n",2000);
  }
  gsm.RXFlush();
  gsm.modemPrint(F("AT+CGATT=1\r"));
  if (waitresp("OK\r\n",10000))
  {
    gsm.RXFlush();
    sprintf(tempbuf,"AT+CGDCONT=%d,\"IP\",\"%s\"\r",cid,apn);
    debugWrite(tempbuf);
    gsm.modemPrint(tempbuf);
    if (waitresp("OK\r\n",1000))
    {
      gsm.RXFlush();
      sprintf(tempbuf,"AT+CGACT=1,%d\r",cid);
      gsm.modemPrint(tempbuf);
      if (waitresp("OK\r\n",1000))
      {
        gsm.RXFlush();
        rc = true;
      }
    }
  }
  gsm.RXFlush();
  return rc;
}

bool A6GPRS::startIP(char apn[])  // apn
{
  return startIP(apn,"","");
}

bool A6GPRS::stopIP()
{
  bool rc = false;
  gsm.RXFlush();
  gsm.modemPrint(F("AT+CIPCLOSE\r"));
  rc = waitresp("OK\r\n",1000);
  return rc;
}

A6GPRS::ePSstate A6GPRS::getPSstate()
{
  ePSstate eps = PS_UNKNOWN;
  gsm.RXFlush();
  gsm.modemPrint(F("AT+CGATT?\r"));  
  if (GetLineWithPrefix("+CGATT:",tempbuf,50,1000))
  {
    if (*tempbuf == '0')
      eps = DETACHED;
    else if (*tempbuf == '1')
      eps = ATTACHED;
  }
  return eps;
}

bool A6GPRS::setPSstate(A6GPRS::ePSstate eps)
{
  bool rc = false;
  gsm.RXFlush();
  switch (eps)
  {
    case DETACHED:
      gsm.modemPrint(F("AT+CGATT=0\r"));
      break;
    case ATTACHED:
      gsm.modemPrint(F("AT+CGATT=1\r"));
      break;
  }
  return waitresp("OK\r\n",2000);
}

bool A6GPRS::getLocalIP(char ip[])
{
  bool rc = false;
  gsm.RXFlush();
  gsm.modemPrint(F("AT+CIFSR\r"));
  GetLineWithPrefix(NULL,ip,20,2000);
  return waitresp("OK\r\n",2000);
}
bool A6GPRS::connectTCPserver(char*path,int port)
{
  bool rc = false;
  CIPstatus = getCIPstatus();
  if (CIPstatus == IP_CLOSE || CIPstatus == IP_GPRSACT)
  {
    sprintf(tempbuf,"AT+CIPSTART=\"TCP\",\"%s\",%d\r",path,port);
    debugWrite(tempbuf);
    gsm.modemPrint(tempbuf);
    if (waitresp("CONNECT OK",10000))
    {
      debugWrite(">>");
      waitresp("OK\r\n",10000);
      rc = true;
    }
  }
  return rc;
}
bool A6GPRS::sendToServer(char msg[])
{
  bool rc = false;
  getCIPstatus();
  if (CIPstatus == CONNECT_OK)
  {
    gsm.modemPrint(F("AT+CIPSEND\r"));
    if (waitresp(">",100))
    {
      gsm.modemPrint(msg);
      gsm.modemWrite(0x1a);
	  txcount += strlen(msg) + 1;
      waitresp("OK\r\n",1000);
      rc = true;
    }
  }
  return rc;
}
bool A6GPRS::sendToServer(char msg[],int length)
{
  bool rc = false;
  getCIPstatus();
  if (CIPstatus == CONNECT_OK)
  {
    gsm.modemPrint(F("AT+CIPSEND="));
    gsm.modemPrint(length);
    gsm.modemPrint(F("\r"));
    if (waitresp(">",100))
    {
      gsm.modemPrint(msg);
	  txcount += length;
      waitresp("OK\r\n",1000);
      rc = true;
    }
  }
  return rc;
}

bool A6GPRS::sendToServer(byte msg[],int length)
{
  bool rc = false;
  char buff[10];
  getCIPstatus();
  if (CIPstatus == CONNECT_OK)
  {
    gsm.modemPrint(F("AT+CIPSEND="));
    gsm.modemPrint(length);
    gsm.modemPrint(F("\r"));
    if (waitresp(">",100))
    {
      for (int i=0;i<length;i++)
      {
        sprintf(buff,"%02X,",msg[i]);
        debugWrite(buff);
        gsm.modemWrite(msg[i]);
		txcount++;
      }
      waitresp("OK\r\n",1000);
      rc = true;
    }
  }
  return rc;
}
