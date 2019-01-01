#!/bin/bash
# @description: This is an example of using the I2C and SPI interfaces
#   to read the ADC value of an SPI photoresistor and then send the value
#   on a I2C PWM LED
#
# @author: Dimitris Tassopoulos <dimtass@gmail.com>
# @project: : http://www.stupid-projects.com/linux-and-the-i2c-and-spi-interface/

# run forever
while true
do
    sleep 1
    # read the I2C photoresistor value
    PTHD=$(i2cget -y 0 0x8 0 w)
    echo "PTHD: \x${PTHD:2:2}\x${PTHD:4:2}"
    # Send the value to the SPI PWM LED
    echo -n -e "\x${PTHD:2:2}\x${PTHD:4:2}" | spi-pipe -d /dev/spidev0.0 -b 2 -n 1
done

