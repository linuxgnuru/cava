#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

unsigned char data[8] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

void bitWrite(int col, int row, int b) { data[col] ^= (-b ^ data[col]) & (1 << row); }

void bitClear(int col, int row) { data[col] ^= (0 ^ data[col]) & (1 << row); }

void bitSet(int col, int row) { data[col] ^= (-1 ^ data[col]) & (1 << row); }

void writeMaxByte(unsigned char addr, unsigned char dat)
{
  unsigned char td[2];

  td[0] = addr;
  td[1] = dat;
  wiringPiSPIDataRW(0, td, 2);
}

void writeMax()
{
  for (int i = 0; i < 8; i++) writeMaxByte(i + 1, data[i]);
}

void clearMax()
{
  for (int i = 0; i < 8; i++) writeMaxByte(i + 1, 0x0);
}

void Init_MAX7219()
{
  writeMaxByte(0x09, 0x00);
  writeMaxByte(0x0a, 0x03);
  writeMaxByte(0x0b, 0x07);
  writeMaxByte(0x0c, 0x01);
  writeMaxByte(0x0f, 0x00);
  clearMax();
}

void doGraph(int c, int r)
{
  c = abs(c - 7);
  for (int i = 0; i < 8; i++)
  {
    if (r == 0)
      data[c] = 0x0;
    else
      bitWrite(c, i, (i < r));
    writeMax();
  }
}

void maxSPI_init()
{
  if (wiringPiSetup() == -1)
  {
    printf("Error trying to setup wiringPi\n");
    exit(1);
  }
  wiringPiSPISetup(0, 500000);
  Init_MAX7219();
  fprintf(stderr, "[wiggins] -- Init SPI\n");
}

int maxSPI(int bars_count, int fd, char bar_delim, char frame_delim, int ascii_range, const int const f[200])
{
  //fprintf(stderr, "[wiggins] -- bars_count: %d ascii_range: %d\n", bars_count, ascii_range);
    for (int i = 0; i < bars_count; i++)
    {
        int f_ranged = f[i];
        if (f_ranged > ascii_range) f_ranged = ascii_range;
        //if (f_ranged > 8) f_ranged = 8;
        // finding size of number-string in byte
        //int bar_height_size = 2; // a number + \0
        //if (f_ranged != 0) bar_height_size += floor (log10 (f_ranged));
        //char bar_height[bar_height_size];
        //snprintf(bar_height, bar_height_size, "%d", f_ranged);
  //fprintf(stderr, "[wiggins] -- f_ranged: %d\n", f_ranged);
        doGraph(i, f_ranged);
        //write(fd, bar_height, bar_height_size - 1);
        //write(fd, &bar_delim, sizeof(bar_delim));
    }
	//write(fd, &frame_delim, sizeof(frame_delim));
    return 0;
}

