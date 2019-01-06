#!/bin/bash
# @description: This is an example of using the I2C and SPI modules
#   to read the ADC value of an SPI photoresistor and then send the value
#   on a I2C PWM LED
#
# @author: Dimitris Tassopoulos <dimtass@gmail.com>
# @project: : http://www.stupid-projects.com/linux-and-the-i2c-and-spi-interface/

# run forever
while true
do
    sleep 0.05
    # read the I2C photoresistor value
    PTHD=$(cat /sys/bus/iio/devices/iio\:device0/in_illuminance_raw)
    echo "PTHD: ${PTHD}"
    # Send the value to the SPI PWM LED
    echo ${PTHD} > /sys/bus/spi/devices/spi0.0/leds/ardled-0/brightness
done