// bcm2835.c
// bcm2835.c
// C and C++ support for Broadcom BCM 2835 as used in Raspberry Pi
// http://elinux.org/RPi_Low-level_peripherals
// http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
//
// Author: Mike McCauley
// Copyright (C) 2011-2013 Mike McCauley
// $Id: bcm2835.c,v 1.10 2013/03/18 05:57:36 mikem Exp mikem $
// 
// 03/17/2013 : Charles-Henri Hallard (http://hallard.me)
//              Modified Adding some fonctionnalities
//							Added millis() function
//              Added option to use custom Chip Select Pin PI GPIO instead of only CE0 CE1
//              Done a hack to use CE1 by software as custom CS pin because HW does not work
//              Added function to determine PI revision board
//							Added function to set SPI speed (instead of divider for easier look in code)
// 06/29/2013   Incorporated latest version of bcm2825.h done by Mike McCauley
// 
// 08/26/2015	Lorenzo Delana (lorenzo.delana@gmail.com)
//		Use of i2c-2 if BANANAPI macro enabled


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "bcm2835.h"

// This define enables a little test program (by default a blinking output on pin RPI_GPIO_PIN_11)
// You can do some safe, non-destructive testing on any platform with:
// gcc bcm2835.c -D BCM2835_TEST
// ./a.out
//#define BCM2835_TEST

// Pointers to the hardware register bases
volatile uint32_t *bcm2835_gpio = MAP_FAILED;
volatile uint32_t *bcm2835_pwm  = MAP_FAILED;
volatile uint32_t *bcm2835_clk  = MAP_FAILED;
volatile uint32_t *bcm2835_pads = MAP_FAILED;
volatile uint32_t *bcm2835_spi0 = MAP_FAILED;
volatile uint32_t *bcm2835_bsc0 = MAP_FAILED;
volatile uint32_t *bcm2835_bsc1 = MAP_FAILED;
volatile uint32_t *bcm2835_st	= MAP_FAILED;


// This variable allows us to test on hardware other than RPi.
// It prevents access to the kernel memory, and does not do any peripheral access
// Instead it prints out what it _would_ do if debug were 0
static uint8_t debug = 0;

// I2C The time needed to transmit one byte. In microseconds.
//static int i2c_byte_wait_us = 0;

// i2c file descriptor for opeping i2c device
static int i2c_fd = 0;

// SPI Custom Chip Select Pin
static int spi_custom_cs = 0;

// Time for millis function
static unsigned long long epoch ;

//
// Low level register access functions
//

void  bcm2835_set_debug(uint8_t d)
{
    debug = d;
}


// Get raspberry PI model version
int bcm2835_get_pi_version( void ) 
{ 
	int rev = 0;
	char buff[512];
	char * p;
	char * pend;
	
	FILE * fd ;

	// do some clean up
	memset(buff,0,sizeof(buff));

	fd = fopen("/proc/cpuinfo","r");

	// Opened successfully
	if( fd )
	{
		//printf("File opened successfully through fopen()\n");
		
		// parse each line until we the end or we find the good one
		while( fgets(buff, sizeof(buff), fd) != NULL && rev ==0 ) 
		{
			// search 
			if( (strstr(buff, "Revision" )) != NULL )  
			{
				// point to the separator ":" format is has follow
				// Revision        : 000f
				if ( (p = strtok( buff, ":")) != NULL )
				{
					// Ok get value
					if ( (p = strtok( NULL, ":")) != NULL )
					{
						// Revision Version is in hex format so put 0x before the number
						*p   = 'x';
						*--p = '0';
						
						// convert to number
						rev = strtol(p, &pend, 16);
						
						//printf("rev=%d 0x%04x\n", rev, rev);
						
						// not Okay ?
						if ( !*pend )
						{
							rev= 0;
						}
						else
						{
							// Revision 1 or 2 ?
							rev = (rev < 4 ) ? 1 : 2 ;
						}
					}
				}
			}
		}

		// Close the file.
		if(fd) 
		{
			fclose(fd);
		}
	}

	return rev;
} 



// safe read from peripheral
uint32_t bcm2835_peri_read(volatile uint32_t* paddr)
{
	if (debug)
	{
		printf("bcm2835_peri_read  paddr %08X\n", (unsigned) paddr);
		return 0;
	}
	else
	{
		// Make sure we dont return the _last_ read which might get lost
		// if subsequent code changes to a different peripheral
		uint32_t ret = *paddr;
		uint32_t dummy = *paddr;
		dummy=dummy; // avoid compiler warning
		return ret;
	}
}

// read from peripheral without the read barrier
uint32_t bcm2835_peri_read_nb(volatile uint32_t* paddr)
{
	if (debug)
	{
		printf("bcm2835_peri_read_nb  paddr %08X\n", (unsigned) paddr);
		return 0;
	}
	else
	{
		return *paddr;
	}
}

// safe write to peripheral
void bcm2835_peri_write(volatile uint32_t* paddr, uint32_t value)
{
   if (debug)
    {
	printf("bcm2835_peri_write paddr %08X, value %08X\n", (unsigned) paddr, value);
    }
    else
    {
	// Make sure we don't rely on the first write, which may get
	// lost if the previous access was to a different peripheral.
	*paddr = value;
	*paddr = value;
    }
}

// write to peripheral without the write barrier
void bcm2835_peri_write_nb(volatile uint32_t* paddr, uint32_t value)
{
    if (debug)
    {
	printf("bcm2835_peri_write_nb paddr %08X, value %08X\n",
               (unsigned) paddr, value);
    }
    else
    {
	*paddr = value;
    }
}

// Set/clear only the bits in value covered by the mask
void bcm2835_peri_set_bits(volatile uint32_t* paddr, uint32_t value, uint32_t mask)
{
    uint32_t v = bcm2835_peri_read(paddr);
    v = (v & ~mask) | (value & mask);
    bcm2835_peri_write(paddr, v);
}

//
// Low level convenience functions
//

// Function select
// pin is a BCM2835 GPIO pin number NOT RPi pin number
//      There are 6 control registers, each control the functions of a block
//      of 10 pins.
//      Each control register has 10 sets of 3 bits per GPIO pin:
//
//      000 = GPIO Pin X is an input
//      001 = GPIO Pin X is an output
//      100 = GPIO Pin X takes alternate function 0
//      101 = GPIO Pin X takes alternate function 1
//      110 = GPIO Pin X takes alternate function 2
//      111 = GPIO Pin X takes alternate function 3
//      011 = GPIO Pin X takes alternate function 4
//      010 = GPIO Pin X takes alternate function 5
//
// So the 3 bits for port X are:
//      X / 10 + ((X % 10) * 3)
void bcm2835_gpio_fsel(uint8_t pin, uint8_t mode)
{
    // Function selects are 10 pins per 32 bit word, 3 bits per pin
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPFSEL0/4 + (pin/10);
    uint8_t   shift = (pin % 10) * 3;
    uint32_t  mask = BCM2835_GPIO_FSEL_MASK << shift;
    uint32_t  value = mode << shift;
    bcm2835_peri_set_bits(paddr, value, mask);
}

// Set output pin
void bcm2835_gpio_set(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPSET0/4 + pin/32;
    uint8_t shift = pin % 32;
    bcm2835_peri_write(paddr, 1 << shift);
}

// Clear output pin
void bcm2835_gpio_clr(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPCLR0/4 + pin/32;
    uint8_t shift = pin % 32;
    bcm2835_peri_write(paddr, 1 << shift);
}

// Set all output pins in the mask
void bcm2835_gpio_set_multi(uint32_t mask)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPSET0/4;
    bcm2835_peri_write(paddr, mask);
}

// Clear all output pins in the mask
void bcm2835_gpio_clr_multi(uint32_t mask)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPCLR0/4;
    bcm2835_peri_write(paddr, mask);
}

// Read input pin
uint8_t bcm2835_gpio_lev(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPLEV0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = bcm2835_peri_read(paddr);
    return (value & (1 << shift)) ? HIGH : LOW;
}

// See if an event detection bit is set
// Sigh cant support interrupts yet
uint8_t bcm2835_gpio_eds(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPEDS0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = bcm2835_peri_read(paddr);
    return (value & (1 << shift)) ? HIGH : LOW;
}

// Write a 1 to clear the bit in EDS
void bcm2835_gpio_set_eds(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPEDS0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_write(paddr, value);
}

// Rising edge detect enable
void bcm2835_gpio_ren(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPREN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, value, value);
}
void bcm2835_gpio_clr_ren(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPREN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, 0, value);
}

// Falling edge detect enable
void bcm2835_gpio_fen(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPFEN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, value, value);
}
void bcm2835_gpio_clr_fen(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPFEN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, 0, value);
}

// High detect enable
void bcm2835_gpio_hen(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPHEN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, value, value);
}
void bcm2835_gpio_clr_hen(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPHEN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, 0, value);
}

// Low detect enable
void bcm2835_gpio_len(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPLEN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, value, value);
}
void bcm2835_gpio_clr_len(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPLEN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, 0, value);
}

// Async rising edge detect enable
void bcm2835_gpio_aren(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPAREN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, value, value);
}
void bcm2835_gpio_clr_aren(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPAREN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, 0, value);
}

// Async falling edge detect enable
void bcm2835_gpio_afen(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPAFEN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, value, value);
}
void bcm2835_gpio_clr_afen(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPAFEN0/4 + pin/32;
    uint8_t shift = pin % 32;
    uint32_t value = 1 << shift;
    bcm2835_peri_set_bits(paddr, 0, value);
}

// Set pullup/down
void bcm2835_gpio_pud(uint8_t pud)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPPUD/4;
    bcm2835_peri_write(paddr, pud);
}

// Pullup/down clock
// Clocks the value of pud into the GPIO pin
void bcm2835_gpio_pudclk(uint8_t pin, uint8_t on)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPPUDCLK0/4 + pin/32;
    uint8_t shift = pin % 32;
    bcm2835_peri_write(paddr, (on ? 1 : 0) << shift);
}

// Read GPIO pad behaviour for groups of GPIOs
uint32_t bcm2835_gpio_pad(uint8_t group)
{
    volatile uint32_t* paddr = bcm2835_pads + BCM2835_PADS_GPIO_0_27/4 + group*2;
    return bcm2835_peri_read(paddr);
}

// Set GPIO pad behaviour for groups of GPIOs
// powerup value for al pads is
// BCM2835_PAD_SLEW_RATE_UNLIMITED | BCM2835_PAD_HYSTERESIS_ENABLED | BCM2835_PAD_DRIVE_8mA
void bcm2835_gpio_set_pad(uint8_t group, uint32_t control)
{
    volatile uint32_t* paddr = bcm2835_pads + BCM2835_PADS_GPIO_0_27/4 + group*2;
    bcm2835_peri_write(paddr, control);
}

// Some convenient arduino-like functions
// milliseconds
void bcm2835_delay(unsigned int millis)
{
    struct timespec sleeper;
    
    sleeper.tv_sec  = (time_t)(millis / 1000);
    sleeper.tv_nsec = (long)(millis % 1000) * 1000000;
    nanosleep(&sleeper, NULL);
}

// microseconds
void bcm2835_delayMicroseconds(uint64_t micros)
{
    struct timespec t1;
    uint64_t        start;
	
    // Calling nanosleep() takes at least 100-200 us, so use it for
    // long waits and use a busy wait on the System Timer for the rest.
    start =  bcm2835_st_read();
    
    if (micros > 450)
    {
	t1.tv_sec = 0;
	t1.tv_nsec = 1000 * (long)(micros - 200);
	nanosleep(&t1, NULL);
    }    
  
    bcm2835_st_delay(start, micros);
}

// This function is added in order to simulate arduino millis() function
unsigned int bcm2835_millis(void)
{
	struct timeval now;
	unsigned long long ms;    
	
	gettimeofday(&now, NULL);
	
	ms = (now.tv_sec * 1000000 + now.tv_usec) / 1000 ;
	
	return ((uint32_t) (ms - epoch ));
}

//
// Higher level convenience functions
//

// Set the state of an output
void bcm2835_gpio_write(uint8_t pin, uint8_t on)
{
    if (on)
	bcm2835_gpio_set(pin);
    else
	bcm2835_gpio_clr(pin);
}

// Set the state of a all 32 outputs in the mask to on or off
void bcm2835_gpio_write_multi(uint32_t mask, uint8_t on)
{
    if (on)
	bcm2835_gpio_set_multi(mask);
    else
	bcm2835_gpio_clr_multi(mask);
}

// Set the state of a all 32 outputs in the mask to the values in value
void bcm2835_gpio_write_mask(uint32_t value, uint32_t mask)
{
    bcm2835_gpio_set_multi(value & mask);
    bcm2835_gpio_clr_multi((~value) & mask);
}

// Set the pullup/down resistor for a pin
//
// The GPIO Pull-up/down Clock Registers control the actuation of internal pull-downs on
// the respective GPIO pins. These registers must be used in conjunction with the GPPUD
// register to effect GPIO Pull-up/down changes. The following sequence of events is
// required:
// 1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
// to remove the current Pull-up/down)
// 2. Wait 150 cycles ? this provides the required set-up time for the control signal
// 3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
// modify ? NOTE only the pads which receive a clock will be modified, all others will
// retain their previous state.
// 4. Wait 150 cycles ? this provides the required hold time for the control signal
// 5. Write to GPPUD to remove the control signal
// 6. Write to GPPUDCLK0/1 to remove the clock
//
// RPi has P1-03 and P1-05 with 1k8 pullup resistor
void bcm2835_gpio_set_pud(uint8_t pin, uint8_t pud)
{
    bcm2835_gpio_pud(pud);
    delayMicroseconds(10);
    bcm2835_gpio_pudclk(pin, 1);
    delayMicroseconds(10);
    bcm2835_gpio_pud(BCM2835_GPIO_PUD_OFF);
    bcm2835_gpio_pudclk(pin, 0);
}

void bcm2835_spi_begin(uint8_t cs)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;

    // Set the SPI0 pins to the Alt 0 function to enable SPI0 access on them
		// except if we need custom Chip Select Pin 
		// printf("bcm2835_spi_begin -> spi_custom_cs = %d \n",cs );
		
		// Do we need custom chip select control or 
		// drive CE1 manually (because CE1 does not work with hardware) 
		if ( cs > BCM2835_SPI_CS_NONE || cs == BCM2835_SPI_CS1 )
		{
			// indicate we will use a custom GPIO port
			spi_custom_cs = cs ;

			// ok hard CE1 not working, drive it manually
			if (cs == BCM2835_SPI_CS1 )
			{
				// Dirty Hack CE1 in now custom Chip Select GPIO 26
				// the real CE1 pin
				spi_custom_cs = RPI_GPIO_P1_26 ; 
				
				bcm2835_gpio_fsel(spi_custom_cs, BCM2835_GPIO_FSEL_OUTP); 
				bcm2835_gpio_write(spi_custom_cs, HIGH);
			}

			// Mask in we use custom CS (not sure it has a real effect)
			bcm2835_peri_set_bits(paddr, BCM2835_SPI_CS_NONE, BCM2835_SPI0_CS_CS);
		}
		// Ok hardware driving of chip select
		else
		{
			// Just in case
			spi_custom_cs = 0 ;
			
			// Mask in the CS bits of CS
			bcm2835_peri_set_bits(paddr, cs, BCM2835_SPI0_CS_CS);
		}		
		
		// Now we can drive the I/O as asked 
		if (spi_custom_cs == 0)
		{
			// Not custom CS, so hardware driven
			bcm2835_gpio_fsel(RPI_GPIO_P1_24, BCM2835_GPIO_FSEL_ALT0); // CE0 
			bcm2835_gpio_fsel(RPI_GPIO_P1_26, BCM2835_GPIO_FSEL_ALT0); // CE1 
		}
		else
		{
			// so set custom CS as output, High level by default
			bcm2835_gpio_fsel(spi_custom_cs, BCM2835_GPIO_FSEL_OUTP); // Custom GPIO
			bcm2835_gpio_write(spi_custom_cs, HIGH);
		}
		
		// Classic pin, hardware driven
    bcm2835_gpio_fsel(RPI_GPIO_P1_21, BCM2835_GPIO_FSEL_ALT0); // MISO
    bcm2835_gpio_fsel(RPI_GPIO_P1_19, BCM2835_GPIO_FSEL_ALT0); // MOSI
    bcm2835_gpio_fsel(RPI_GPIO_P1_23, BCM2835_GPIO_FSEL_ALT0); // CLK
    
    // Set the SPI CS register to the some sensible defaults
    bcm2835_peri_write(paddr, 0); // All 0s
    
    // Clear TX and RX fifos
    bcm2835_peri_write_nb(paddr, BCM2835_SPI0_CS_CLEAR);
}

void bcm2835_spi_end(void)
{  
    // Set all the SPI0 pins back to input
		if (spi_custom_cs == 0)
		{
			bcm2835_gpio_fsel(RPI_GPIO_P1_26, BCM2835_GPIO_FSEL_INPT); // CE1
			bcm2835_gpio_fsel(RPI_GPIO_P1_24, BCM2835_GPIO_FSEL_INPT); // CE0
		}
		else
		{
			bcm2835_gpio_fsel(spi_custom_cs, BCM2835_GPIO_FSEL_INPT); // Custom GPIO
		}
    bcm2835_gpio_fsel(RPI_GPIO_P1_21, BCM2835_GPIO_FSEL_INPT); // MISO
    bcm2835_gpio_fsel(RPI_GPIO_P1_19, BCM2835_GPIO_FSEL_INPT); // MOSI
    bcm2835_gpio_fsel(RPI_GPIO_P1_23, BCM2835_GPIO_FSEL_INPT); // CLK
}


// Drive Custom chip select pin
void bcm2835_spi_setChipSelect(uint8_t level)
{
	// Do this only if we are using custom ChipSelect I/O
	if ( spi_custom_cs > BCM2835_SPI_CS_NONE )
		bcm2835_gpio_write(spi_custom_cs, level);
}


void bcm2835_spi_setBitOrder(uint8_t order)
{
    // BCM2835_SPI_BIT_ORDER_MSBFIRST is the only one suported by SPI0
}

// defaults to 0, which means a divider of 65536.
// The divisor must be a power of 2. Odd numbers
// rounded down. The maximum SPI clock rate is
// of the APB clock
void bcm2835_spi_setClockDivider(uint16_t divider)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CLK/4;
    bcm2835_peri_write(paddr, divider);
}

void bcm2835_spi_setClockSpeed(uint16_t speed)
{
    bcm2835_spi_setClockDivider( speed);
}

void bcm2835_spi_setDataMode(uint8_t mode)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    // Mask in the CPO and CPHA bits of CS
    bcm2835_peri_set_bits(paddr, mode << 2, BCM2835_SPI0_CS_CPOL | BCM2835_SPI0_CS_CPHA);
}

// Writes (and reads) a single byte to SPI
uint8_t bcm2835_spi_transfer(uint8_t value)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    volatile uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;

		// Custom chip select LOW
		bcm2835_spi_setChipSelect(LOW);

    // This is Polled transfer as per section 10.6.1
    // BUG ALERT: what happens if we get interupted in this section, and someone else
    // accesses a different peripheral? 
    // Clear TX and RX fifos
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

    // Set TA = 1
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

    // Maybe wait for TXD
    while (!(bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_TXD))
	delayMicroseconds(10);

    // Write to FIFO, no barrier
    bcm2835_peri_write_nb(fifo, value);

    // Wait for DONE to be set
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE))
	delayMicroseconds(10);

    // Read any byte that was sent back by the slave while we sere sending to it
    uint32_t ret = bcm2835_peri_read_nb(fifo);

    // Set TA = 0, and also set the barrier
    bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);

		// Custom chip select HIGH
		bcm2835_spi_setChipSelect(HIGH);

    return ret;
}

// Writes (and reads) an number of bytes to SPI
void bcm2835_spi_transfernb(char* tbuf, char* rbuf, uint32_t len)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    volatile uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;

		// Custom chip select LOW
		bcm2835_spi_setChipSelect(LOW);

    // This is Polled transfer as per section 10.6.1
    // BUG ALERT: what happens if we get interupted in this section, and someone else
    // accesses a different peripheral? 

    // Clear TX and RX fifos
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

    // Set TA = 1
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

    uint32_t i;
    for (i = 0; i < len; i++)
    {
	// Maybe wait for TXD
	while (!(bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_TXD))
	    delayMicroseconds(10);

	// Write to FIFO, no barrier
	bcm2835_peri_write_nb(fifo, tbuf[i]);

	// Wait for RXD
	while (!(bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_RXD))
	    delayMicroseconds(10);

	// then read the data byte
	rbuf[i] = bcm2835_peri_read_nb(fifo);
    }
    // Wait for DONE to be set
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE))
	delayMicroseconds(10);

    // Set TA = 0, and also set the barrier
    bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);

		// Custom chip select HIGH
		bcm2835_spi_setChipSelect(HIGH);
	
}

// Writes an number of bytes to SPI
void bcm2835_spi_writenb(char* tbuf, uint32_t len)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    volatile uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;

		// Custom chip select LOW
		bcm2835_spi_setChipSelect(LOW);

    // This is Polled transfer as per section 10.6.1
    // BUG ALERT: what happens if we get interrupted in this section, and someone else
    // accesses a different peripheral?

    // Clear TX and RX fifos
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

    // Set TA = 1
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

    uint32_t i;
	for (i = 0; i < len; i++)
	{
		// Maybe wait for TXD
		while (!(bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_TXD))
			;

		// Write to FIFO, no barrier
		bcm2835_peri_write_nb(fifo, tbuf[i]);
	}

    // Wait for DONE to be set
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE))
    	;

    // Set TA = 0, and also set the barrier
    bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);

		// Custom chip select HIGH
		bcm2835_spi_setChipSelect(HIGH);
}

// Writes (and reads) an number of bytes to SPI
// Read bytes are copied over onto the transmit buffer
void bcm2835_spi_transfern(char* buf, uint32_t len)
{
    bcm2835_spi_transfernb(buf, buf, len);
}

void bcm2835_spi_chipSelect(uint8_t cs)
{
	// All is done now in bcm2835_spi_begin()

}

void bcm2835_spi_setChipSelectPolarity(uint8_t cs, uint8_t active)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;

		// only valid for no custom CS
		if (cs <= BCM2835_SPI_CS_NONE)
		{
			uint8_t shift = 21 + cs;
			
			// Mask in the appropriate CSPOLn bit
			bcm2835_peri_set_bits(paddr, active << shift, 1 << shift);
		}
}

int bcm2835_i2c_begin(void)
{
	int fd ;

#if BANANAPI
	if ((fd = open ("/dev/i2c-2", O_RDWR)) < 0)
#else
	if ((fd = open (bcm2835_get_pi_version() == 1 ? "/dev/i2c-0":"/dev/i2c-1" , O_RDWR)) < 0)
#endif
		return fd;
		
	// Set i2c descriptor
	i2c_fd = fd;

	return i2c_fd ;
		
}

void bcm2835_i2c_end(void) 
{
	// close i2c bus
	if ( i2c_fd )
	{
		close(i2c_fd);
		i2c_fd = 0;
	}
}

int bcm2835_i2c_setSlaveAddress(uint8_t addr)
{
	if (!i2c_fd)
		return (-1);

	// Set I2C Device Address
	return (ioctl (i2c_fd, I2C_SLAVE, addr) );
			
}

// set I2C clock divider by means of a baudrate number
void bcm2835_i2c_set_baudrate(uint32_t baudrate)
{
  volatile uint32_t* paddr = bcm2835_bsc1 + BCM2835_BSC_DIV/4;
	
	uint32_t divider;
	
	// use 0xFFFE mask to limit a max value and round down any odd number
	divider = (BCM2835_CORE_CLK_HZ / baudrate) & 0xFFFE;
	
  bcm2835_peri_write(paddr, divider);
}

// Writes an number of bytes to I2C
int bcm2835_i2c_write(const char * buf, uint32_t len)
{
	int reason = -1	;
	
	if (!i2c_fd)
		return (-1);

	// Do simple use of I2C smbus command regarding number of bytes to transfer
	// Write 1 byte
	if (len == 2)
		reason = i2c_smbus_write_byte_data(i2c_fd, buf[0], buf[1]);
	// Write 1 word
	else if (len == 3)
		reason = i2c_smbus_write_word_data(i2c_fd, buf[0], (buf[2]<<8) | buf[1] );
	// Write bulk data
	else 
		reason = i2c_smbus_write_i2c_block_data(i2c_fd, buf[0], len-1, (const __u8 *) &buf[1]);
		
	usleep(1);

	return ( reason );

}

// Read an number of bytes from I2C
// to do
uint8_t bcm2835_i2c_read(char* buf, uint32_t len)
{
    uint8_t reason = 0;

    return reason;
}

// Read the System Timer Counter (64-bits)
uint64_t bcm2835_st_read(void)
{
    volatile uint32_t* paddr;
    uint64_t st;
    paddr = bcm2835_st + BCM2835_ST_CHI/4;
    st = bcm2835_peri_read(paddr);
    st <<= 32;
    paddr = bcm2835_st + BCM2835_ST_CLO/4;
    st += bcm2835_peri_read(paddr);
    return st;
}

// Delays for the specified number of microseconds with offset
void bcm2835_st_delay(uint64_t offset_micros, uint64_t micros)
{
    uint64_t compare = offset_micros + micros;

    while(bcm2835_st_read() < compare)
	;
}

// Allocate page-aligned memory.
void *malloc_aligned(size_t size)
{
    void *mem;
    errno = posix_memalign(&mem, BCM2835_PAGE_SIZE, size);
    return (errno ? NULL : mem);
}

// Map 'size' bytes starting at 'off' in file 'fd' to memory.
// Return mapped address on success, MAP_FAILED otherwise.
// On error print message.
static void *mapmem(const char *msg, size_t size, int fd, off_t off)
{
    void *map = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, off);
    if (MAP_FAILED == map)
	fprintf(stderr, "bcm2835_init: %s mmap failed: %s\n", msg, strerror(errno));
    return map;
}

static void unmapmem(void **pmem, size_t size)
{
    if (*pmem == MAP_FAILED) return;
    munmap(*pmem, size);
    *pmem = MAP_FAILED;
}

// Initialise this library.
int bcm2835_init(void)
{
	struct timeval tv ;
	
    if (debug) 
    {
	bcm2835_pads = (uint32_t*)BCM2835_GPIO_PADS;
	bcm2835_clk = (uint32_t*)BCM2835_CLOCK_BASE;
	bcm2835_gpio = (uint32_t*)BCM2835_GPIO_BASE;
	bcm2835_pwm = (uint32_t*)BCM2835_GPIO_PWM;
	bcm2835_spi0 = (uint32_t*)BCM2835_SPI0_BASE;
	bcm2835_bsc0 = (uint32_t*)BCM2835_BSC0_BASE;
	bcm2835_bsc1 = (uint32_t*)BCM2835_BSC1_BASE;
	bcm2835_st   = (uint32_t*)BCM2835_ST_BASE;
	return 1; // Success
    }
    int memfd = -1;
    int ok = 0;
    // Open the master /dev/memory device
    if ((memfd = open("/dev/mem", O_RDWR | O_SYNC) ) < 0) 
    {
	fprintf(stderr, "bcm2835_init: Unable to open /dev/mem: %s\n",
		strerror(errno)) ;
	goto exit;
    }
	
    // GPIO:
    bcm2835_gpio = mapmem("gpio", BCM2835_BLOCK_SIZE, memfd, BCM2835_GPIO_BASE);
    if (bcm2835_gpio == MAP_FAILED) goto exit;

    // PWM
    bcm2835_pwm = mapmem("pwm", BCM2835_BLOCK_SIZE, memfd, BCM2835_GPIO_PWM);
    if (bcm2835_pwm == MAP_FAILED) goto exit;

    // Clock control (needed for PWM)
    bcm2835_clk = mapmem("clk", BCM2835_BLOCK_SIZE, memfd, BCM2835_CLOCK_BASE);
    if (bcm2835_clk == MAP_FAILED) goto exit;
    
    bcm2835_pads = mapmem("pads", BCM2835_BLOCK_SIZE, memfd, BCM2835_GPIO_PADS);
    if (bcm2835_pads == MAP_FAILED) goto exit;
    
    bcm2835_spi0 = mapmem("spi0", BCM2835_BLOCK_SIZE, memfd, BCM2835_SPI0_BASE);
    if (bcm2835_spi0 == MAP_FAILED) goto exit;

    // I2C
    bcm2835_bsc0 = mapmem("bsc0", BCM2835_BLOCK_SIZE, memfd, BCM2835_BSC0_BASE);
    if (bcm2835_bsc0 == MAP_FAILED) goto exit;

    bcm2835_bsc1 = mapmem("bsc1", BCM2835_BLOCK_SIZE, memfd, BCM2835_BSC1_BASE);
    if (bcm2835_bsc1 == MAP_FAILED) goto exit;

    // ST
    bcm2835_st = mapmem("st", BCM2835_BLOCK_SIZE, memfd, BCM2835_ST_BASE);
    if (bcm2835_st == MAP_FAILED) goto exit;

    ok = 1;

exit:
    if (memfd >= 0)
        close(memfd);

    if (!ok)
	bcm2835_close();
	
  gettimeofday (&tv, NULL) ;
	epoch = (tv.tv_sec * 1000000 + tv.tv_usec) / 1000 ;

    return ok;
}

// Close this library and deallocate everything
int bcm2835_close(void)
{
    if (debug) return 1; // Success
    unmapmem((void**) &bcm2835_gpio, BCM2835_BLOCK_SIZE);
    unmapmem((void**) &bcm2835_pwm,  BCM2835_BLOCK_SIZE);
    unmapmem((void**) &bcm2835_clk,  BCM2835_BLOCK_SIZE);
    unmapmem((void**) &bcm2835_spi0, BCM2835_BLOCK_SIZE);
    unmapmem((void**) &bcm2835_bsc0, BCM2835_BLOCK_SIZE);
    unmapmem((void**) &bcm2835_bsc1, BCM2835_BLOCK_SIZE);
    unmapmem((void**) &bcm2835_st,   BCM2835_BLOCK_SIZE);
    return 1; // Success
}    
