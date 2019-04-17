/*
 * SPI.h
 *
 */

#ifndef SPI_H_
#define SPI_H_

// enum dev_t {TFT, TOUCH, SDCARD};

void SPI_init(void);
uint8_t SPI_rxtx(uint8_t tx);

#endif /* SPI_H_ */