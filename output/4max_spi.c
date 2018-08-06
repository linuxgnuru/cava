//#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#define CE 0

//MAX7219/MAX7221's memory register addresses:
// See Table 2 on page 7 in the Datasheet
const char NoOp        = 0x00;
const char Digits[8] = {
 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
};
const char DecodeMode  = 0x09;
const char Intensity   = 0x0A;
const char ScanLimit   = 0x0B;
const char ShutDown    = 0x0C;
const char DisplayTest = 0x0F;

const char numOfDevices = 4;

unsigned char ledData[4][8] = {
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }
};

void bitWrite4(int addr, int col, int row, int b)
{
    if (addr < 0 || addr > 3 || col < 0 || col > 7 || row < 0 || row > 7) return;
    ledData[addr][col] ^= (-b ^ ledData[addr][col]) & (1 << row);
}

void bitClear4(int addr, int col, int row)
{
    if (addr < 0 || addr > 3 || col < 0 || col > 7 || row < 0 || row > 7) return;
    ledData[addr][col] ^= (0 ^ ledData[addr][col]) & (1 << row);
}

void bitSet4(int addr, int col, int row)
{
    if (addr < 0 || addr > 3 || col < 0 || col > 7 || row < 0 || row > 7) return;
    ledData[addr][col] ^= (-1 ^ ledData[addr][col]) & (1 << row);
}

_Bool bitRead4(int addr, int col, int row)
{
    return (ledData[addr][col] >> row) & 0x01;
}

unsigned char TxBuffer[1024];
int TxBufferIndex = 0;

void transfer(char c)
{
    TxBuffer[TxBufferIndex] = c;
    TxBufferIndex++;
}

void endTransfer()
{
    wiringPiSPIDataRW(CE, TxBuffer, TxBufferIndex);
    TxBufferIndex = 0;
}

// Writes data to the selected device or does broadcast if device number is 255
void SetData(char adr, char data, char device)
{
    // Count from top to bottom because first data which is sent is for the last device in the chain
    for (int i = numOfDevices; i > 0; i--)
    {
        if ((i == device) || (device == 255))
        {
            transfer(adr);
            transfer(data);
        }
        else // if its not the selected device send the noop command
        {
            transfer(NoOp);
            transfer(0);
        }
    }
    endTransfer();
    //delay(1);
}

// Writes the same data to all devices
void SetData_all(char adr, char data) { SetData(adr, data, 255); } // write to all devices (255 = Broadcast) 

void SetShutDown(char Mode) { SetData_all(ShutDown, !Mode); }
void SetScanLimit(char Digits) { SetData_all(ScanLimit, Digits); }
void SetIntensity(char intense) { SetData_all(Intensity, intense); }
void SetDecodeMode(char Mode) { SetData_all(DecodeMode, Mode); }

void clearAll4() { for (int i = 0; i < 8; i++) SetData_all(Digits[i], 0b00000000); }

void Draw(int addr, int col, int row, _Bool b)
{
    char mydata = 0x0;

    //col = abs(col - 7);
    bitWrite4(addr, col, row, b);
    mydata = ledData[addr][col];
    SetData(Digits[col], mydata, addr + 1);
}

void doGraph4(int addr, int height, int col)
{
    int i;

    if (height == 0)
    {
        for (i = 0; i < 8; i++)
            Draw(addr, i, col, 0);
    }
    else if (height == 8)
    {
        for (i = 0; i < 8; i++)
            Draw(addr, i, col, 1);
    }
    else
    {
        for (i = 7; i > -1; i--)
            Draw(addr, i, col, (i < height));
    }
}

void max4SPI_init()
{
    // Open port for reading and writing
    if (wiringPiSPISetup(CE, 500000) < 0)
    {
        printf("Failed to open SPI port %d! Please try with sudo\n", CE);
        exit(1);
    }
    SetDecodeMode(0); // Disable the decode mode because at the moment i dont use 7-Segment displays
    SetScanLimit(7); // Set the number of digits; start to count at 0
    SetIntensity(2); // Set the intensity between 0 and 15. Attention 0 is not off!
    SetShutDown(0); // Disable shutdown mode
    clearAll4();
    //fprintf(stderr, "[wiggins] -- Init 4 SPI\n");
}

//int maxSPI(int bars_count, int fd, char bar_delim, char frame_delim, int ascii_range, const int const f[200])
//int max4SPI(int bars_count, int fd, int ascii_range, const int const f[200])
int max4SPI(int bars_count, int ascii_range, const int const f[200])
{
    div_t t;
    //char bar_delim = ':';
    //char frame_delim = '\n';
    for (int i = 0; i < bars_count; i++)
    {
        t = div(i, 8);
        int addr = t.quot;
        int col = t.rem;
        int f_ranged = f[i];
        if (f_ranged > ascii_range) f_ranged = ascii_range;
#if 0
        // finding size of number-string in byte
        int bar_height_size = 2; // a number + \0
        if (f_ranged != 0) bar_height_size += floor (log10 (f_ranged));
        char bar_height[bar_height_size];
        snprintf(bar_height, bar_height_size, "%d", f_ranged);
        //fprintf(stderr, "[%s] [%d]", bar_height, f_ranged);
        write(fd, bar_height, bar_height_size - 1);
        write(fd, &bar_delim, sizeof(bar_delim));
        //fprintf(stderr, "[%s]", bar_height);
#endif
        //fprintf(stderr, "[wiggins] -- ascii_range [%d] f_ranged [%d]\n", ascii_range, f_ranged);
        doGraph4(addr, f_ranged, col);
    }
//	write(fd, &frame_delim, sizeof(frame_delim));
    //fprintf(stderr, "\n");
    return 0;
}

