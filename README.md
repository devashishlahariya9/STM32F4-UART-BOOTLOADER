# STM32F4-UART-BOOTLOADER
A simple Bootloader and Bootloader utility to Flash Code on the MCU using the UART Peripheral.

## 1. Steps To Flash Bootloader:
	1. Create a Project for Bootloader and give it a name of your choice.
	2. In Bootloader Project enable your preferred UART for the Bootloader.
	3. Replace the main.c file with Bootloader's main.c file and edit the Macro to change the UART Peripheral.
	4. Build The Project and Flash the Bootloader on your MCU.

## 2. Steps To Generate Correct Binary for your Application Code:
	1. Create another project for your application.
 	2. Open the Linker Script (Example: STM32F411VETX_FLASH.ld) and Change the FLASH origin to 0x08004000 and set the length to 496K.
  	3. In Core/Src, Open the system_stm32f4xx.c file and uncomment the "USER_VECT_TAB_ADDRESS" macro.
   	4. Edit the "VECT_TAB_OFFSET" adding 0x4000 to it.
	5. In Project Properties, Go to C/C++ Build -> Settings -> MCU Post build outputs and select "Convert to binary file (-O binary)".
 
 ## 3. Steps to Flash your generated Application Binary using the stm32f4-flash-bin app:
 	1. Open a terminal and navigate to the "STM32F4 UART FLASH TOOL" directory.
  	2. Compile the application using command "gcc main.c RS232\rs232.c -IRS232 -Wall -Wextra -o2 -o stm32f4-flash-bin".
   	3. Enter the command ".\stm32f4-flash-bin <COM_PORT_NUMBER> <PATH_TO_APPLICATION_BINARY_FILE>".
	4. Press the Reset button on your MCU and hit enter on the terminal to start flashing the binary.

**NOTE: The Bootloader will be in programming mode for only 1000ms by default, this can be changed by altering the "BOOTLOADER_WAITING_TICKS" macro in main.c**
**NOTE: This Bootloader is specifically designed for the STM32F411E-DISCO Board and is not tested for any other boards, use at your own risk.**
