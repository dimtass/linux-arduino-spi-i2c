#!/bin/bash
# @description: This is an example of using the I2C and SPI interfaces
#   to read the ADC value of an SPI photoresistor and then send the value
#   on a I2C PWM LED
#
# @author: Dimitris Tassopoulos <dimtass@gmail.com>
# @project: : http://www.stupid-projects.com/linux-and-the-i2c-and-spi-interface/

#spi-config -d /dev/spidev0.0 -m 0 -l 0 -b 8 -s 1000000
# run forever
while true
do
    sleep 0.05
    # read the I2C photoresistor value
    PTHD=$(i2cget -y 1 0x8 0 w)
    echo "PTHD: 0x${PTHD:2:2}${PTHD:4:2}"
    # Send the value to the SPI PWM LED
    echo -n -e "\x${PTHD:2:2}\x${PTHD:4:2}" | spi-pipe -d /dev/spidev0.0 -b 2 -n 1 -s 1000000
done

