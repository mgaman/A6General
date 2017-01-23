#ifndef A6DEVICE_H
#define A6DEVICE_H

#define HW_SERIAL Serial1
#define HW_SERIAL_EVENT serialEvent1

#define A6_BAUDRATE 115200
#define COMM_BUF_LEN 200  // make big enough to hold all incoming data

class A6GPRSDevice
{
  public:
    A6GPRSDevice();
    ~A6GPRSDevice();
    virtual bool begin();
    virtual bool begin(long baud_rate);
    void ModemWrite(char);
    bool waitresp(char const *response_string,int32_t timeout);
    bool GetLineWithPrefix(char const *px,char *outbuf, int l,int32_t timeout);
    // weak methods are helpers - implement only if you really want them
    void HWReset() __attribute__((weak));   // may be implemented by the caller 
    void DebugWrite(int) __attribute__((weak));
    void DebugWrite(uint16_t) __attribute__((weak));
    void DebugWrite(char) __attribute__((weak));
    void DebugWrite(char*) __attribute__((weak));
    void DebugWrite(const __FlashStringHelper*) __attribute__((weak));
    void RXFlush();  // clear all data in input buffer
    bool enableDebug;
    char pop();       // get data from buffer, -1 if none available
 //   void push(char);   // add incoming data to buffer
	int32_t rxcount,txcount;
  private:
//    char comm_buf[COMM_BUF_LEN];  // communication buffer +1 for 0x00 termination
  //  int inlevel, outlevel;        // data in comm_buf
};
#endif

