/*
 * ARD101PH - Arduino Photoresistor sensor
 *
 * Copyright (c) 2018, Dimitris Tassopoulos.
 *
 * This file is subject to the terms and conditions of GPL v2
 *
 * IIO driver for ARD101PH (7-bit I2C slave address 0x08).
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>

#include <linux/mutex.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define ARD101PH_DRV_NAME "ard101ph"
#define ARD101PH_REG_DATA		0x00

struct ard101ph_data {
	struct i2c_client *client;
	struct mutex mutex;
};

static const struct iio_chan_spec ard101ph_channels[] = {
	{
		.type	= IIO_LIGHT,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	}
};

static int ard101ph_init(struct ard101ph_data *data)
{
	int ret;

	/* power on */
	ret = i2c_smbus_read_word_data(data->client, ARD101PH_REG_DATA);
	if (ret < 0)
		return ret;

	pr_info("ard101ph_init: %d\n", ret);
	return 0;
}

static int ard101ph_read_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *chan, int *val, int *val2,
		long mask)
{
	struct ard101ph_data *data = iio_priv(indio_dev);
	int ret;

	mutex_lock(&data->mutex);
	switch (chan->type) {
	case IIO_LIGHT:
		*val = ret = i2c_smbus_read_word_data(data->client, ARD101PH_REG_DATA);
        if (ret < 0)
            dev_err(&data->client->dev,
                "failed to read ADC%d value\n", *val);
		ret = IIO_VAL_INT;
		break;
	default:
		break;
	}
	mutex_unlock(&data->mutex);

	pr_info("ard101ph_read_raw: %d\n", *val);
	return ret;
}
EXPORT_SYMBOL(ard101ph_read_raw);

static const struct iio_info ard101ph_info = {
	.driver_module	= THIS_MODULE,
	.read_raw	= ard101ph_read_raw,
};

static int ard101ph_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct ard101ph_data *data;
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	i2c_set_clientdata(client, indio_dev);
	data->client = client;

	ret = ard101ph_init(data);
	if (ret < 0)
		goto err;

	mutex_init(&data->mutex);

	indio_dev->dev.parent = &client->dev;
	indio_dev->channels = ard101ph_channels;
	indio_dev->num_channels = ARRAY_SIZE(ard101ph_channels);
	indio_dev->name = ARD101PH_DRV_NAME;
	indio_dev->modes = INDIO_DIRECT_MODE;
	
	indio_dev->info = &ard101ph_info;

	ret = iio_device_register(indio_dev);
	if (ret < 0)
		goto err;

	pr_info("ARD101PH module loaded...\n");
	return 0;

err:
	return ret;
}

static int ard101ph_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);

	iio_device_unregister(indio_dev);

	return 0;
}

static const struct i2c_device_id ard101ph_id[] = {
	{ ARD101PH_DRV_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ard101ph_id);

static struct i2c_driver ard101ph_driver = {
	.driver = {
		.name	= ARD101PH_DRV_NAME,
	},
	.probe		= ard101ph_probe,
	.remove		= ard101ph_remove,
	.id_table	= ard101ph_id,
};

module_i2c_driver(ard101ph_driver);

MODULE_AUTHOR("Dimitris Tasspoulos <dimtass@gmail.com>");
MODULE_AUTHOR("www.stupid-projects.com");
MODULE_DESCRIPTION("ARD101PH ambient light photo sensor driver");
MODULE_LICENSE("GPL v2");