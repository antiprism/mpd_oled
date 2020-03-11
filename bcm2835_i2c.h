/*
   Original author:  Mike McCauley (mikem@airspayce.com)
   Original Project: https://www.airspayce.com/mikem/bcm2835/
   License: GPL3 - https://www.gnu.org/licenses/gpl-3.0.html
*/

// Defines for BCM2835_I2C_H
#ifndef BCM2835_I2C_H
#define BCM2835_I2C_H

#include "bcm2835.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Start I2C operations.
/// Forces RPi I2C pins P1-03 (SDA) and P1-05 (SCL)
/// to alternate function ALT0, which enables those pins for I2C interface.
/// You should call bcm2835_i2c_alt_end() when all I2C functions are complete to
/// return the pins to their default functions \sa  bcm2835_i2c_alt_end()
/// \param[in] i2c_bus The I2C i2c bus number.
int bcm2835_i2c_alt_begin(int i2c_bus);

/// End I2C operations.
/// I2C pins P1-03 (SDA) and P1-05 (SCL)
/// are returned to their default INPUT behaviour.
void bcm2835_i2c_alt_end(void);

/// Sets the I2C slave address.
/// \param[in] addr The I2C slave address.
int bcm2835_i2c_alt_setSlaveAddress(uint8_t addr);

/// Sets the I2C clock divider by converting the baudrate parameter to
/// the equivalent I2C clock divider. ( see \sa bcm2835_i2c_alt_setClockDivider)
/// For the I2C standard 100khz you would set baudrate to 100000
/// The use of baudrate corresponds to its use in the I2C kernel device
/// driver. (Of course, bcm2835 has nothing to do with the kernel driver)
void bcm2835_i2c_alt_set_baudrate(uint32_t baudrate);

/// Transfers any number of bytes to the currently selected I2C slave.
/// (as previously set by \sa bcm2835_i2c_alt_setSlaveAddress)
/// \param[in] buf Buffer of bytes to send.
/// \param[in] len Number of bytes in the buf buffer, and the number of bytes to
/// send. \return i2c smbus command return code
int bcm2835_i2c_alt_write(const char *buf, uint32_t len);

/// Transfers any number of bytes from the currently selected I2C slave.
/// (as previously set by \sa bcm2835_i2c_alt_setSlaveAddress)
/// \param[in] buf Buffer of bytes to receive.
/// \param[in] len Number of bytes in the buf buffer, and the number of bytes to
/// received. \return reason see \ref bcm2835I2CReasonCodes
uint8_t bcm2835_i2c_alt_read(char *buf, uint32_t len);

/// @}

#ifdef __cplusplus
}
#endif

#endif // BCM2835_I2C_H

