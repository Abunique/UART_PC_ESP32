"# UART_PC_ESP32"
TASK : Receive string data from PC serial and measure the data rate per second

Controller in use : ESP32
DEVBoard : ESP32 Wroom
Language : C

Task explanation : 

1. RTOS tasks for RX and TX are created with Rx being in high priority

2. Tx task that suspends and resume post receiving data

3. Timer with 1 second interval, measures the data transmitted and received per second

4. Memory segment in use : FLASH(SPIFFS- Serial peripheral - flash system)

5. Rx task : receives data and stores them in a text file 'TEST.txt' which is located in the non volatile Flash space

6. Tx task : opens the file in flash using SPIFFS and transmits the read data

7. SPIFFS flash usage: a partition table is created for making partion in the flash memory and names storage : spiffs(same is used as tag name in the function calls)

8. python script for transmitting the data from the PC terminal is made

9. TTL converter is used for communicating between the UART2 of the ESP32 and PC.

![Firmware Architecture](C:\Users\Abinaya\workspace\UART_FLASH\NYMBLE_FIRMWARE_TASK.png)
