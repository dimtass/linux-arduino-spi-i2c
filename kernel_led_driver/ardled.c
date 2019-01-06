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
#include <linux/init.h>
#include <linux/device.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>

#define ARDLED_DRV_NAME "ardled"
#define ARDCOMB_MAX_SPI_FREQ_HZ		1000000

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
	int ret;
	struct spi_device *spi = led->spi;

	mutex_lock(&led->mutex);
	pwm_value = cpu_to_be16(brightness & 0xfff);
	ret = spi_write(spi, &pwm_value, sizeof(pwm_value));

	if (ret < 0) {
		dev_err(&spi->dev, "failed to set brightness (%d)\n", ret);
		goto _exit_set;
	}

    pr_info("ardled set: %d, ret: %d\n", pwm_value, ret);
    // pr_info(
    //     "bus_num: %d\n"
    //     "bits_per_word: %d\n"
    //     "chip_select: %d\n"
    //     "mode: %d\n"
    //     "max_speed_hz: %d\n\n",
    //     led->spi->controller->bus_num,
    //     led->spi->bits_per_word, led->spi->chip_select, led->spi->mode, led->spi->max_speed_hz
    // );

_exit_set:
	mutex_unlock(&led->mutex);
	return ret;
}
EXPORT_SYMBOL(ardled_set_brightness);

static int ardled_probe(struct spi_device *spi)
{
    struct ardled_data	*led;
	int ret;

	led = kzalloc(sizeof(*led), GFP_KERNEL);
	if (!led)
		return -ENOMEM;

    led->spi = spi;
    snprintf(led->name, sizeof(led->name), "ardled-%d", spi->controller->bus_num);
    mutex_init(&led->mutex);
	mutex_lock(&led->mutex);
    led->ldev.name = led->name;
    led->ldev.brightness = LED_OFF;
    led->ldev.max_brightness = 0x3ff;
    led->ldev.brightness_set_blocking = ardled_set_brightness;
    ret = led_classdev_register(&spi->dev, &led->ldev);
	mutex_unlock(&led->mutex);
    if (ret < 0) {
        dev_err(&spi->dev,
			"couldn't register LED %s\n",
			led->name);
        goto eledcr;
    }

	spi_set_drvdata(spi, led);
	ret = spi_setup(spi);
	if (ret < 0) {
		pr_info("Error spi_setup: %d\n", ret);
		goto eledcr;
	}

    pr_info("Arduino PWM LED %s registered\n", led->name);
	return 0;

eledcr:
	kfree(led);
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
	{ "ardled", 0 },
	{}
};
MODULE_DEVICE_TABLE(spi, ardled_id);

static struct spi_driver ardled_driver = {
	.driver = {
		.name = ARDLED_DRV_NAME,
	},
	.probe = ardled_probe,
	.remove = ardled_remove,
	.id_table = ardled_id,
};
module_spi_driver(ardled_driver);

MODULE_AUTHOR("Dimitris Tasspoulos <dimtass@gmail.com>");
MODULE_AUTHOR("www.stupid-projects.com");
MODULE_DESCRIPTION("Arduino SPI PWM LED driver");
MODULE_LICENSE("GPL v2");