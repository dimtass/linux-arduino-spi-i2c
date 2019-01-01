Linux and the I2C and SPI interface
----

## Hardware
For testing I've used the [Arduino nano](https://store.arduino.cc/arduino-nano)
to emulate two devices, one I2C photoresistor sensor and one SPI PWM LED.
These devices, for example, could be two different attiny85; but as the
atmega368p is power enough for the task, we can use the same chip to emulate
both.

Also, I'm using the [nanopi-neo](http://wiki.friendlyarm.com/wiki/index.php/NanoPi_NEO)
for the Linux hardware as it's a low spec board with an allwinner H3,
it's really cheap and common to find and it has both an I2C and SPI interface.

## Connection
These are the connections between the `Nanopi-neo` and the `Arduino nano`.

Signal | Arduino | Nanopi-neo
-|-|-
/SS | D10 | 24 (SPI0_CS)
MOSI | D11 | 19 (SPI0_MISO)
MISO | D12 | 21 (SPI0_MOSI)
SCK | D13 | 23 (SPI0_CLK)
SDA | A4 | 3 (I2C0_SDA)
SCL | A5 | 5 (I2C0_SCL)

> For the `Nanopi-neo` pinout header you can have a look [here](http://wiki.friendlyarm.com/wiki/index.php/NanoPi_NEO)

## Yocto build
```sh
mkdir -p yocto/sources
cd yocto/sources
git clone --depth 1 -b sumo git://git.yoctoproject.org/poky
git clone --depth 1 -b sumo git@github.com:openembedded/meta-openembedded.git
git clone --depth 1 git@bitbucket.org:dimtass/meta-allwinner-hx.git
mv ../../meta-allwinner-i2c-spi-arduino .
cp meta-allwinner-hx/scripts/setup-environment.sh .
cp meta-allwinner-i2c-spi-arduino/flash_sd.sh .
cd ..
```

Now you should be in the `yocto/` folder.
```sh
MACHINE=nanopi-neo source ./setup-environment.sh build
bitbake arduino-test-image
bitbake -c populate_sdk arduino-test-image
```

This might take a lot of time depending your build system. In the end you get an SDK
to build the kernel modules and the SD image. To copy the image to the SD card you
can either run the `flash_sd.sh` like that:

```sh
MACHINE=nanopi-neo ./flash_sd.sh /dev/sdX
```

or use the bmap-tool. You need to have the bmap-tool install to flash the image.
To install it just run this:
```sh
sudo apt install bmap-tools
```

Or get the last version from [here](https://github.com/intel/bmap-tools).

## Connect via SSH
By default the Yocto image doesn't enable the eth0 interface; therefore,
you need to connect a USB-to-uart interface to the board in order to have
access.

> The default `root` has an empty password.

Then you need to connect the board to your router and then run this command to get
an IP via DHCP:

```sh
udhcpc -i eth0
```

Then you need to wait for a few seconds and you will see something like this:
```sh
udhcpc: started, v1.27.2
udhcpc: sending discover
udhcpc: sending select for 192.168.0.33
udhcpc: lease of 192.168.0.33 obtained, lease time 864000
/etc/udhcpc.d/50default: Adding DNS 192.168.0.1
```

That means that now `eth0` has the `192.168.0.33` address; therefore you can
connect via ssh like this:

```sh
ssh root@192.168.0.33
```


## Bash script
To test the project with the bash script you need to copy the script
to the remote device and then run it. For example:

```sh
scp bash-script-example.sh root@192.168.0.33:/home/root
```

Then from the target side run this:
```sh
./bash-script-example.sh
```

## User space app
From the user space you can have access to the devices without the need of
special drivers, by just using the Linux API that exposes the I2C interface.
In case of the SPI a driver is needed though, but that comes with the OS
and you don't have to write your own and it's called `spidev`. In the Yocto
image the proper kernel flags are enable and also the `spidev` is registered
in the OS by loading the `sun8i-h3-spi-spidev` device-tree overlay in
`/boot/allwinnerEnv.txt`.

To build the application, just cd in the `linux-app` folder and build it with
the SDK.

```sh
cd linux-app
source /opt/poky/2.5.1/environment-setup-armv7vehf-neon-poky-linux-gnueabi
$CC linux-app.c -o linux-app
```

And then `scp` the executable to the target and run it.
```sh
scp linux-app root@192.168.0.33:/home/root
```

And on the target:
```sh
cd /home/root
./linux-app
```

Then you'll see that the app will sample the I2C light sensor every 150ms and
update the SPI PWM LED.

```sh
Application started
SPI mode: 0
SPI bits per word: 8
SPI max speed: 1000000 Hz (1000 KHz)
:  907
:  908
:  909
...
```

## Kernel drivers
You can build a kernel driver for the I2C sensor and the SPI PWM LED.

#### IIO driver
The IIO subsystem is created to support various ADC, DAC and other sensor
devices. In our case the arduino I2C device is a light sensor, althoug a
very simple one. To build and test the driver do the following on your dev
workstation:

```sh
source /opt/poky/2.5.1/environment-setup-armv7vehf-neon-poky-linux-gnueabi
cd /opt/poky/2.5.1/sysroots/armv7vehf-neon-poky-linux-gnueabi/usr/src/kernel
make silentoldconfig scripts
```

Then go back to the project folder and:
```sh
cd kernel_iio_driver
source /opt/poky/2.5.1/environment-setup-armv7vehf-neon-poky-linux-gnueabi
KERNEL_SRC="/opt/poky/2.5.1/sysroots/armv7vehf-neon-poky-linux-gnueabi/usr/src/kernel" make
```

This will create the iio module and now you need to copy it to the target
```sh
scp ard101ph.ko root@192.168.0.33:/home/root
```

Now on the target run these commands:
```sh
insmod ard101ph.ko
echo ard101ph 0x08 > /sys/bus/i2c/devices/i2c-0/new_device
cat /sys/bus/iio/devices/iio\:device1/in_illuminance_raw
```

Depending on your board, the `i2c` bus and `iio` device might be different.
Now every time you run this:
```sh
cat /sys/bus/iio/devices/iio\:device1/in_illuminance_raw
```

You get the current ADC value from the arduino sensor.

To unload the module and driver run this:
```sh
echo 0x08 > /sys/bus/i2c/devices/i2c-0/delete_device
rmmod ard101ph
```

