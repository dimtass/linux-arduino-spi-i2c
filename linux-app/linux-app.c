/*
 * Copyright (C) MIT.
 *
 * Description: This is an example of using the I2C and SPI interfaces
 * 	to read the ADC value of an SPI photoresistor and then send the value
 * 	on a I2C PWM LED. You can find more info here:
 * 	http://www.stupid-projects.com/linux-and-the-i2c-and-spi-interface/
 *
 * Author: Dimitris Tassopoulos <dimtass@gmail.com>
 *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define POLLING_MS	50
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define STR_MAX_SIZE 32

#define DEBUG
#ifdef DEBUG
#define TRACE(X) do { printf X ;} while(0)
#else
#define TRACE(x)
#endif

/** I2C **/
#define I2C_ADDR 0x08

struct i2c_dev {
	char 	device[STR_MAX_SIZE];
	int		fd;
	int		addr;
};

int i2c_init(char * dev)
{
	int fd;
	if ((fd = open(dev, O_RDWR)) < 0) {
	    /* ERROR HANDLING: you can check errno to see what went wrong */
	    perror("Failed to open the i2c bus");
	    exit(1);
	}
	return fd;
}

uint16_t i2c_read_adc(int i2c_addr, int i2c_fd)
{
	uint16_t resp = 0;
	uint8_t i2c_buf[5];

	if (ioctl(i2c_fd, I2C_SLAVE, i2c_addr) < 0) {
		perror("Failed to acquire bus access and/or talk to slave.\n");
		exit(1);
		/* ERROR HANDLING; you can check errno to see what went wrong */
	}
	/* read data */
	if (read(i2c_fd,i2c_buf,2) != 2) {
		/* ERROR HANDLING: i2c transaction failed */
		printf("Failed to read from the i2c bus (%d).\n", i2c_fd);
	} else {
		resp = (i2c_buf[1] << 8) | i2c_buf[0];
	}
	return resp;
}

/** SPI **/
struct spi_dev {
	char		device[STR_MAX_SIZE];
	int			fd;
	uint8_t		mode;
	uint8_t		bits;
	uint32_t	speed;
};

int spi_init(struct spi_dev* dev)
{
	dev->fd = open(dev->device, O_RDWR);
	if (dev->fd < 0){
		perror("can't open device");
		exit(1);
	}

	/*
	 * spi mode
	 */
	if (ioctl(dev->fd, SPI_IOC_WR_MODE, &dev->mode)) {
		perror("can't set spi mode");
		exit(1);
	}

	/*
	 * bits per word
	 */
	if (ioctl(dev->fd, SPI_IOC_WR_BITS_PER_WORD, &dev->bits) == -1){
		perror("can't set bits per word");
		exit(1);
	}

	/*
	 * max speed hz
	 */
	if (ioctl(dev->fd, SPI_IOC_WR_MAX_SPEED_HZ, &dev->speed) == -1){
		perror("can't set max speed hz");
		exit(1);
	}

	printf("SPI mode: %d\n", dev->mode);
	printf("SPI bits per word: %d\n", dev->bits);
	printf("SPI max speed: %d Hz (%d KHz)\n", dev->speed, dev->speed/1000);

	return dev->fd;
}

int spi_send16(struct spi_dev * dev, uint16_t data)
{
	uint8_t tx[2] = {0};
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.len = ARRAY_SIZE(tx),
		.delay_usecs = 0,
		.speed_hz = dev->speed,
		.bits_per_word = dev->bits,
	};
	tx[0] = (data >> 8) & 0xFF;
	tx[1] = data & 0xFF;

	if (ioctl(dev->fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
		perror("can't send spi message");
		exit(1);
	}
	return tr.len;
}

void print_usage(int argc, char** argv)
{
	printf(
		"Usage:\n"
		"%s [I2C DEV] [SPI DEV]\n"
		"	[I2C DEV]: the I2C device/bus that the photoresistor is connected (e.g. /dev/i2c-0)\n"
		"	[SPI DEV]: the SPI device/bus that the PWM LED is connected (e.g. /dev/spidev0.0)\n\n",
		argv[0]
	);
}


int main(int argc, char** argv) {

	print_usage(argc, argv);

	/* Create the I2C data */
	struct i2c_dev i2c = {
		.fd = -1,
		.addr = I2C_ADDR
	};
	strncpy(i2c.device, argv[1], STR_MAX_SIZE);

	struct spi_dev spi = {
		.fd = -1,
		.mode = 0,
		.bits = 8,
		.speed = 1000000,
	};
	strncpy(spi.device, argv[2], STR_MAX_SIZE);
	printf("Application started\n"); /* prints Hello World */

	/* init i2c */
	i2c.fd = i2c_init(i2c.device);

	/* init spidev */
	spi_init(&spi);

	/* loop forever */
	while (1) {
		/* read ADC value from the photoresistor via I2C */
		uint16_t adc_value = i2c_read_adc(i2c.addr, i2c.fd);
		/* send the PWM LED value via SPI */
		spi_send16(&spi, adc_value);
		TRACE((":  %d\n", adc_value));
		/* sleep */
		usleep(POLLING_MS * 1000);
	}

	/* clean up */
	close(i2c.fd);
	close(spi.fd);

    return 0;
}


