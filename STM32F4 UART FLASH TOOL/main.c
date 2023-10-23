/*
	Author: Devashish Lahariya
	Date: 22-10-2023

	Use at your own risk, the author is not liable for ANY damages caused.

	Compile: gcc main.c RS232\rs232.c -IRS232 -Wall -Wextra -o2 -o stm32f4-flash-bin
	Usage: ./stm32f4-flash-bin <COM_PORT_NUMBER> <PATH_TO_BIN_FILE>
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "RS232/rs232.h"

#define BYTES_TO_SEND_PER_PACKET		1024
#define BOOTLOADER_RECEPTION_START_KEY	0x12345678

typedef struct
{
	uint32_t start_key;
	uint32_t nBytes;
}BOOTLOADER_START_PACKET;

BOOTLOADER_START_PACKET bsp = {0, 0};

uint32_t bytes_to_data(uint8_t* buffer)
{
	uint32_t data = 0;

	data |= (buffer[3]  << 24  & 0x000000FF);
	data |= ((buffer[2] << 16) & 0x00FF0000);
	data |= ((buffer[1] << 8)  & 0x0000FF00);
	data |= ((buffer[0]) 	   & 0xFF000000);

	return data;
}

void delay_us(uint32_t us)
{
	us *= 10;
#ifdef _WIN32
	// Sleep(ms);
	__int64 time1 = 0, time2 = 0, freq = 0;

	QueryPerformanceCounter((LARGE_INTEGER *)&time1);
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

	do
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&time2);
	} while ((time2 - time1) < us);
#else
	usleep(us);
#endif
}

void delay_ms(uint32_t _ms)
{
	int ms_value = _ms * 1000;

	delay_us(ms_value);
}

size_t get_file_size(FILE* _file)
{
    if(_file == NULL)
	{
        printf("File Not Found!\n");

        return -1;
    }
    fseek(_file, 0L, SEEK_END);
    size_t res = ftell(_file);
	fseek(_file, 0L, SEEK_SET);

    return res;
}

void send_bootloader_start_packet(int _comport, uint32_t _start_key, uint32_t _nBytes)
{
	uint8_t* ptr = (uint8_t*)&bsp;

	bsp.start_key = _start_key;
	bsp.nBytes = _nBytes;

	for(int i=0; i<(int)sizeof(bsp); i++)
	{
		RS232_SendByte(_comport, ptr[i]);
		delay_ms(10);
	}
}

void send_bootloader_data_packet(int _comport, uint8_t* _packet_buffer, uint32_t _packet_len)
{
	uint8_t* packet_len_ptr = (uint8_t*)&_packet_len;

	for(int i=0; i<4; i++)
	{
		RS232_SendByte(_comport, packet_len_ptr[i]);
		delay_ms(10);
	}

	for(uint32_t i=0; i<_packet_len; i++)
	{
		RS232_SendByte(_comport, _packet_buffer[i]);
		delay_ms(10);
	}
}

int main(int argc, char** argv)
{
	#if defined(__linux__) || defined(__FreeBSD__)
		system("clear");
	#else
		system("cls");
	#endif

	argc = argc;										//To avoid unneccessary compiler warnings

	char comport_str[5];
	char file_name[1024];

	int comport = 0;
	int bdrate  = 115200;

	char mode[] = {'8', 'N', '1', 0}; 					// *-bits, No parity, 1 stop bit

	if(strcmp(argv[1], "--help") == 0)
	{
		printf("Details:\nThis is a simple command line tool to flash stm32 firmware using UART.\nVersion: 1.0.0\nCreated By: Devashish Lahariya\nUse Command: stm32f4-flash-bin <COM PORT NUMBER> <PATH TO FIRMWARE BINARY FILE>\nExample: stm32f4-flash-bin 3 \"hello.bin\"\n");

		return 0;
	}

	strcpy(comport_str, argv[1]);						//Get the COM Port
	strcpy(file_name,   argv[2]);						//Get the file name

	comport = atoi(argv[1]) - 1;

	if(RS232_OpenComport(comport, bdrate, mode, 0))		//Open The COM Port
	{
		printf("ERROR!! CANNOT OPEN COM PORT!\n");
	}
	else
	{
		printf("OPENING FILE \"%s\"\n", file_name);
		printf("PORT COM%s OPENED SUCCESSFULLY...\n", comport_str);

		FILE* bin_file = fopen(file_name, "rb");

		if(bin_file == NULL)
		{
			printf("ERROR!! FILE NOT FOUND!\n");

			return -1;
		}
		int file_size_bytes = get_file_size(bin_file);

		uint8_t* bin_file_buffer = (uint8_t*)malloc(file_size_bytes + 1);
		bin_file_buffer[file_size_bytes] = '\0';

		uint8_t current_packet_buffer[BYTES_TO_SEND_PER_PACKET];

		float total_packets = (float)file_size_bytes / (float)BYTES_TO_SEND_PER_PACKET;
		if(total_packets - (int)total_packets > 0) total_packets++;

		printf("APP SIZE: %d BYTES\n\n", file_size_bytes);
		printf("FLASHING FIRMWARE...\n");
		send_bootloader_start_packet(comport, BOOTLOADER_RECEPTION_START_KEY, file_size_bytes);
		
		fseek(bin_file, 0L, SEEK_SET);
		fread(bin_file_buffer, 1, file_size_bytes, bin_file);

		uint8_t err_flag = 0;
		uint32_t bytes_sent = 0;
		uint32_t bytes_to_copy = 0;
		uint32_t file_buffer_offset = 0;

		float flash_percentage = 0;

		for(int i=0; i<(int)total_packets; i++)
		{
			if(i != ((int)total_packets - 1)) bytes_to_copy = BYTES_TO_SEND_PER_PACKET;
			else bytes_to_copy = file_size_bytes - bytes_sent;

			memcpy(current_packet_buffer, bin_file_buffer + file_buffer_offset, bytes_to_copy);

			uint8_t resp = 0x00;

			RS232_PollComport(comport, &resp, 1);

			if(resp == 0x01)
			{
				send_bootloader_data_packet(comport, current_packet_buffer, bytes_to_copy);

				file_buffer_offset += bytes_to_copy;
				bytes_sent += bytes_to_copy;
				err_flag = 0;
			}
			else
			{
				err_flag = 1;

				printf("INVALID RESPONSE FROM BOOTLOADER!!\nTERMINATING SESSION!\n");

				break;
			}
			flash_percentage = ((float)bytes_sent / (float)file_size_bytes) * 100;
			printf("%.2f%% COMPLETE...\n", flash_percentage);
		}
		free(bin_file_buffer);
		fclose(bin_file);
		RS232_CloseComport(comport);

		printf("\nPORT COM%s CLOSED...\n\n", comport_str);

		(err_flag == 0) ? printf("FIRMWARE FLASHED SUCCESSFULLY...\n") : printf("CANNOT FLASH FIRMWARE!!\n");
	}
	return 0;
}