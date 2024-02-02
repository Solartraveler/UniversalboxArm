#pragma once

#include <stdint.h>
#include <stdbool.h>

//sets up the GPIOs, after init, the ESP is not enabled.
void EspInit(void);

//Enables the power supply of the ESP
void EspEnable(void);

//Disables the power supply of the ESP
void EspStop(void);

/* Gets a char if the ESP has send one. 0, if no char has been in the queue.
A binary 0 can not be distinguished from "no data" with this function.
*/
char EspGetChar(void);

/* Sends data to the ESP, since a len is given, this allows sending binary data.
*/
void EspSendData(const uint8_t * data, size_t len);

/*Just gets the data, no matter what the content is, stops when maxResponse or
timeout (in ms) is reached. Returns the number of bytes read.
*/
uint32_t EspGetData(uint8_t * response, size_t maxResponse, uint32_t timeout);

/* Extracts a response from a remote system. responseLen returns the number of bytes read.
Response:
0: ok
1: no response or corrupt response
2: too many data for response buffer. responseLen is filled with the bytes the
   response would have had. In this case the data could be get by EspGetData.
*/
uint32_t EspGetResponseData(uint8_t * response, size_t maxResponse, size_t * responseLen, uint32_t timeout);

/* Sends a command and waits for the answer.
response is always \0 terminated.
Returns:
0: OK was found in response
1: Error was found
2: Nothing was found
*/
uint32_t EspCommand(const char * command, char * response, size_t maxResponse, uint32_t timeout);

/* Returns: 0 success
1: could not start connection
2: could not start sending data
3: could not send data
4: no response
5: requestOut buffer too small
*/
uint32_t EspUdpRequestResponse(const char * domain, uint16_t port, uint8_t * requestIn, size_t requestInLen,
     uint8_t * requestOut, size_t requestOutMax, size_t * requestOutLen);


/* Connects to an access point.
Call after EspSetClient is successful.
Returns true if successful
*/
bool EspConnect(const char * ap, const char * password);

/* Disconnects from an access point.
Returns true if successful
*/
bool EspDisconnect(void);

/* Sets the ESP to client mode.
Call after EspWaitPowerupReady is successful.
Returns true if successful
*/
bool EspSetClient(void);

/* Checks if the ESP is ready after power up. Call always takes 1s.
Call after EspEnable.
Returns true if successful
*/
bool EspWaitPowerupReady(void);

