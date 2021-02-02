/*
   Original author:  Mike McCauley (mikem@airspayce.com)
   Original Project: https://www.airspayce.com/mikem/bcm2835/
   License: GPL3 - https://www.gnu.org/licenses/gpl-3.0.html

   08/26/2015 Lorenzo Delana (lorenzo.delana@gmail.com)
              Use of i2c-2 if BANANAPI macro enabled
   11/03/2020 Adrian Rossiter <adrian@antiprism.com>
              Extract I2C functons and use as alternative to current
              release functions.
*/

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "bcm2835.h"
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// i2c file descriptor for opeping i2c device
static int i2c_fd = 0;

int bcm2835_i2c_alt_begin(int i2c_bus)
{
  int fd;
  const size_t dev_name_sz = 32;
  char dev_name[dev_name_sz];
  snprintf(dev_name, dev_name_sz, "/dev/i2c-%d", i2c_bus);


  if ((fd = open(dev_name, O_RDWR)) < 0)
    return fd;

  // Set i2c descriptor
  i2c_fd = fd;

  return i2c_fd;
}

void bcm2835_i2c_alt_end(void)
{
  // close i2c bus
  if (i2c_fd) {
    close(i2c_fd);
    i2c_fd = 0;
  }
}

int bcm2835_i2c_alt_setSlaveAddress(uint8_t addr)
{
  if (!i2c_fd)
    return (-1);

  // Set I2C Device Address
  return (ioctl(i2c_fd, I2C_SLAVE, addr));
}

// set I2C clock divider by means of a baudrate number
void bcm2835_i2c_alt_set_baudrate(uint32_t baudrate)
{
  volatile uint32_t *paddr = bcm2835_bsc1 + BCM2835_BSC_DIV / 4;

  uint32_t divider;

  // use 0xFFFE mask to limit a max value and round down any odd number
  divider = (BCM2835_CORE_CLK_HZ / baudrate) & 0xFFFE;

  bcm2835_peri_write(paddr, divider);
}

// Writes an number of bytes to I2C
int bcm2835_i2c_alt_write(const char *buf, uint32_t len)
{
  int reason = -1;

  if (!i2c_fd)
    return (-1);
/*
  // Do simple use of I2C smbus command regarding number of bytes to transfer
  // Write 1 byte
  if (len == 2)
    reason = i2c_smbus_write_byte_data(i2c_fd, buf[0], buf[1]);
  // Write 1 word
  else if (len == 3)
    reason = i2c_smbus_write_word_data(i2c_fd, buf[0], (buf[2] << 8) | buf[1]);
  // Write bulk data
  else
    reason = i2c_smbus_write_i2c_block_data(i2c_fd, buf[0], len - 1,
                                            (const __u8 *)&buf[1]);
*/
  ///////////
  if (write(i2c_fd, buf, len) != (ssize_t)len) {
        /* ERROR HANDLING: i2c transaction failed */
        fprintf(stderr, "Failed to write to the i2c bus.\n");
  }
  ////////////
  usleep(1);

  return (reason);
}

// Read an number of bytes from I2C
// to do
uint8_t bcm2835_i2c_alt_read(char *buf, uint32_t len)
{
  (void)buf; // suppress warning for unused
  (void)len; // suppress warning for unused
  uint8_t reason = 0;

  return reason;
}

