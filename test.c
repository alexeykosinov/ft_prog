#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "ftd2xx.h"

#pragma comment(lib, "ftd2xx.lib")

#define BUFFER_SIZE 16

FT_HANDLE ftHandle;
FT_STATUS ftStatus;

int f_size = 0;

unsigned char* file_buffer;
unsigned char* tx_buffer;

void chunk_data(unsigned char* rx_buffer, unsigned char* tx_buffer, int* ptr_f_size, int* ptr_f_buf, int* ptr_pos) {

    int buf_size = BUFFER_SIZE;

    int buf_size_temp = *ptr_f_buf;
    int file_size_temp = *ptr_f_size;
    int pos_index = *ptr_pos;

    if (file_size_temp >= buf_size) {
        memcpy(tx_buffer, rx_buffer, buf_size);
        pos_index += buf_size;
        buf_size_temp = buf_size;
        file_size_temp = file_size_temp -= buf_size;
    }
    else {
        memcpy(tx_buffer, rx_buffer, file_size_temp);
        buf_size_temp = file_size_temp;
        file_size_temp = 0;
    }

    *ptr_f_buf = buf_size_temp;
    *ptr_f_size = file_size_temp;
    *ptr_pos = pos_index;
}

int FT_Init(void) {

	unsigned long InTransferSize = 32768;


	FT_STATUS ftStatus = FT_Open(0, &ftHandle);


	if (!FT_SUCCESS(ftStatus)) {
		printf("[ FTDI     ] Unable to open USB device\n");
		return 1;
	}
	else{
		printf("[ FTDI     ] Connection established\n");
	}

	FT_SetBitMode(ftHandle, 0, 0x40); 					// Single Channel Synchronous 245 FIFO Mode
	FT_SetLatencyTimer(ftHandle, 16); 					// Таймаут буфера приема, используемый для сброса оставшихся данных из буфера приема
	FT_SetUSBParameters(ftHandle, InTransferSize, 0);	// Устанавливает размер запроса передачи USB, должно быть кратно 64 байта (от 64 байт до 65536 байт)
	FT_SetResetPipeRetryCount(ftHandle, 100);			// Максимальное количество попыток драйвера сбросить канал (pipe), на котором произошла ошибка
	FT_SetFlowControl(ftHandle, FT_FLOW_RTS_CTS, 0, 0);	// Required to avoid data loss, see appnote "an_130_ft2232h_used_in_ft245 synchronous fifo mode.pdf"
	FT_SetTimeouts(ftHandle, 500, 0);					// Устанавливает таймауты чтения и записи устройства
	FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX); 		// Функция вычищает буферы приема и передачи устройства

	if (ftStatus == FT_OK) {
		printf("[ FTDI     ] Necessary settings are set\n");
	}
	else {
		printf("[ FTDI     ] USB setup error\n");
		return 1;
	}

    return 0;
}


void progress_bar(char label[], int step, int total) {

    const int pwidth = 64;

    int width = pwidth - strlen(label);
    int pos = (step * width) / total;
    int percent = (step * 100) / total;

    printf("%s", label);
    printf(" %3d%%\r", 100 - percent);
}


int main() {

    if (FT_Init() != 0) {
        return 0;
    }

	printf("[ C-2-Prog ] Bootloader \n");

    unsigned char s[50];
    printf("[ C-2-Prog ] Enter the path of the file: ");
    scanf_s("%49[^\n]%*c", s, sizeof(s));

    /* Открываем файл */
    FILE* file;
    fopen_s(&file, s, "rb");

    if (file == NULL) {
        printf("[ C-2-Prog ] Error opening file\n");
        _getch();
        exit(-3);
    }

    fseek(file, 0, SEEK_END);                           // определяем
    f_size = ftell(file);                               // размер
    fseek(file, 0, SEEK_SET);                           // файла
    file_buffer = (char*)malloc(sizeof(char) * f_size); // выделяем память под файл

    size_t result = fread(file_buffer, sizeof(char), f_size, file); // считываем файл в буфер
    if (result != f_size) exit(3);

    /* *********** */

    printf("[ C-2-Prog ] File size: %d B\n", f_size);

    printf("[ C-2-Prog ] %d B chunks: %d + %d B\n", BUFFER_SIZE, f_size / BUFFER_SIZE, f_size % BUFFER_SIZE);

    tx_buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE); // ведяляем память размера BUFFER_SIZE

    int p = (int)file_buffer; // сохраним начальный адрес массива, что бы потом его подчистить

    int* file_size = (int*)f_size; // указатель на объём файла
    int* buf_size = NULL; // указатель на объём буфера
    int buf_size_val = 0;


    if (f_size < BUFFER_SIZE) printf("[ C-2-Prog ] File size is too small\n"); // если файл меньше минимального объёма ( BUFFER_SIZE )


    DWORD BytesWritten;

    DWORD RxBytes;
    DWORD TxBytes;
    DWORD EventDWord;

    while (file_size > 0) {

        //ftStatus = FT_GetStatus(ftHandle, &RxBytes, &TxBytes, &EventDWord);

        //printf("[ FTDI     ] Bytes in RX/TX queue: %d/%d\n", RxBytes, TxBytes);


        //if (ftStatus == FT_OK && (TxBytes == 0)) {

            //printf("[ FTDI     ] Done with status: %d\n", ftStatus);

            chunk_data(file_buffer, tx_buffer, (int*)&file_size, (int*)&buf_size, (int*)&file_buffer);

            //printf("[ C-2-Prog ] Data read: %d\n", (int) buf_size);
            //printf("[ C-2-Prog ] Data left: %d\n", (int) file_size);

            buf_size_val = (int)buf_size;

            progress_bar("[ C-2-Prog ] Programming: ", (int)file_size, f_size);
            //for (int i = 0; i < buf_size_val; i++) {
            //    printf("%x  ", tx_buffer[i]);
            //}
            //printf("\n");
            //ftStatus = FT_Write(ftHandle, tx_buffer, buf_size_val, &BytesWritten);

            //if (ftStatus != FT_OK) {
            //    printf("[ FTDI     ] Write data is failed, code: %d\n", ftStatus);
            //    return 0;
            //}

        //}
        //else {
        //    printf("[ FTDI     ] Error with code: %d\n", ftStatus);
        //}

    }

    _getch();

    /* прибрать за собой */
    fclose(file);
    free(p);
    free(tx_buffer);

    FT_Close(ftHandle);
}