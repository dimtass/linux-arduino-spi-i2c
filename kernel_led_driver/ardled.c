/*
 * ARDLED - Arduino Photoresistor sensor
 *
 * Copyright (c) 2018, Dimitris Tassopoulos.
 *
 * This file is subject to the terms and conditions of GPL v2
 *
 * IIO driver for ARD101PH (7-bit I2C slave address 0x08).
 *
 */
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/spi/spi.h>


#define ARDLED_DRV_NAME "ardled"

struct ardled_data {
	struct led_classdev ldev;
	struct spi_device	*spi;
    char name[SPI_NAME_SIZE];
	struct mutex mutex;
};

static int ardled_set_brightness(struct led_classdev *ldev,
				      enum led_brightness brightness)
{
	struct ardled_data *led = container_of(ldev, struct ardled_data,
						  ldev);
	u16 pwm_value;
    // struct spi_message msg;
    // struct spi_transfer xfer;
	int ret;

	mutex_lock(&led->mutex);
	pwm_value = brightness & 0xfff;
	ret = spi_write(led->spi, &pwm_value, 2);

    // xfer.tx_buf = &pwm_value;
    // xfer.len = sizeof(pwm_value);
    // xfer.bits_per_word = 16;
    // spi_message_init(&msg);
    // spi_message_add_tail(&xfer, &msg);
    // ret = spi_sync(led->spi, &msg);

	mutex_unlock(&led->mutex);

    pr_info("ardled set: %d, ret: %d\n", pwm_value, ret);
    pr_info(
        "bus_num: %d\n"
        "bits_per_word: %d\n"
        "chip_select: %d\n"
        "mode: %d\n"
        "max_speed_hz: %d\n\n",
        led->spi->controller->bus_num,
        led->spi->bits_per_word, led->spi->chip_select, led->spi->mode, led->spi->max_speed_hz
    );

	return ret;
}

static int ardled_probe(struct spi_device *spi)
{
    struct ardled_data	*led;
	int ret;

	led = devm_kzalloc(&spi->dev, sizeof(*led), GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	spi->bits_per_word = 16;
    spi->mode = SPI_MODE_0;

    led->spi = spi;
    snprintf(led->name, sizeof(led->name), "ardled-%d", spi->controller->bus_num);
    mutex_init(&led->mutex);
    led->ldev.name = led->name;
    led->ldev.brightness = LED_OFF;
    led->ldev.max_brightness = 0x3ff;
    led->ldev.brightness_set_blocking = ardled_set_brightness;
    ret = led_classdev_register(&spi->dev, &led->ldev);
    if (ret < 0) {
        dev_err(&spi->dev,
			"couldn't register LED %s\n",
			led->name);
        goto eledcr;
    }

	spi_set_drvdata(spi, led);

    pr_info("Arduino PWM LED %s registered\n", led->name);
	return 0;

eledcr:
	led_classdev_unregister(&led->ldev);

	return ret;
}

static int ardled_remove(struct spi_device *spi)
{
	struct ardled_data *led = spi_get_drvdata(spi);

	led_classdev_unregister(&led->ldev);

	return 0;
}

static const struct spi_device_id ardled_id[] = {
	{ ARDLED_DRV_NAME, 0 },
	{}
};

MODULE_DEVICE_TABLE(spi, ardled_id);

static struct spi_driver ardled_driver = {
	.driver = {
		.name = "ardled",
	},
	.id_table = ardled_id,
	.probe = ardled_probe,
	.remove = ardled_remove,
};
module_spi_driver(ardled_driver);

MODULE_AUTHOR("Dimitris Tasspoulos <dimtass@gmail.com>");
MODULE_AUTHOR("www.stupid-projects.com");
MODULE_DESCRIPTION("Arduino SPI PWM LED driver");
MODULE_LICENSE("GPL v2");