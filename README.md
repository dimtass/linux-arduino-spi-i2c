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

The photo-resistor I'm using is the `VT33A603/2` and also I'm using an orange
LED.s

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

## Preparing the build environment
You'll need to build some modules and code for this project, therefore you
need first to build the Yocto SDK as explained before and the install it in
its default location. Therefore, after running the `populate_sdk` task with
bitbake, you'll find the sdk in the `build/tmp/deploy/sdk` folder. To install
it then run this command to your workstation:
```sh
./build/tmp/deploy/sdk/poky-glibc-x86_64-arduino-test-image-armv7vehf-neon-toolchain-2.5.1.sh
```

This will install the SDK in the `/opt/poky/2.5.1/` folder.

From now on to build any code you need to open the terminal and run these
commands:
```sh
source /opt/poky/2.5.1/environment-setup-armv7vehf-neon-poky-linux-gnueabi
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

The `$CC` variable is the path of the gcc toolchain of the SDK, so be aware
to use that and not just `gcc`.

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
Instead of running on the user-space you can build a kernel module driver for
the I2C sensor and the SPI PWM LED. I'll briefly explain how to do that, but
you need to have some knowledge already about building out-of-tree kernel
modules.

To be able to build the modules with the SDK you first need to build some
tools inside the SDK. You need to do that only once, so run these commands:

```sh
source /opt/poky/2.5.1/environment-setup-armv7vehf-neon-poky-linux-gnueabi
cd /opt/poky/2.5.1/sysroots/armv7vehf-neon-poky-linux-gnueabi/usr/src/kernel
make
silentoldconfig scripts
```

#### IIO driver
The IIO subsystem is created to support various ADC, DAC and other sensor
devices. In our case the arduino I2C device is a light sensor, althoug a
very simple one. To build the driver go back to the main repo folder and
run these commands:

```sh
cd kernel_iio_driver
source /opt/poky/2.5.1/environment-setup-armv7vehf-neon-poky-linux-gnueabi
export KERNEL_SRC="/opt/poky/2.5.1/sysroots/armv7vehf-neon-poky-linux-gnueabi/usr/src/kernel"
make
```

This will create the iio module and now you need to copy it to the target
```sh
scp ard101ph.ko root@192.168.0.33:/home/root
```

Now on the target it's easy to test an I2C module like this:
```sh
cd /home/root
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

To permanately install the module and load it on boot, you need to
compile the device-tree overlay of the module and make it load during
boot. To do so, on your workstation run:

```sh
dtc -I dts -O dtb -o sun8i-h3-i2c-ard101ph.dtbo sun8i-h3-i2c-ard101ph.dts
scp sun8i-h3-i2c-ard101ph.dtbo root@192.168.0.33:/boot/overlay
```

And then on the target board edit the `/boot/allwinnerEnv.txt` and add the
overlay like this:
```sh
overlays=sun8i-h3-i2c0 sun8i-h3-i2c-ard101ph
```

Now add the module with the rest modules and enable it.
```sh
mv ard101ph.ko /lib/modules/$(uname -r)
depmod -a
```

And then reboot.

Now the module is automatically loaded and you can obtain the ADC value as
previously shown.

#### SPI LED driver
Likewise you need to build the driver module and the device-tree overlay and
make it load during boot.

To build the module run this on the main repo folder on your workstation:
```sh
cd kernel_led_driver
source /opt/poky/2.5.1/environment-setup-armv7vehf-neon-poky-linux-gnueabi
export KERNEL_SRC="/opt/poky/2.5.1/sysroots/armv7vehf-neon-poky-linux-gnueabi/usr/src/kernel"
make
```

This should build the module. Now copy it on the target:
```sh
cp ardled.ko root@192.168.0.33:/home/root
```

In case of SPI you can't just load the module and work, so you need to build
the device-tree. To do that:
```sh
dtc -I dts -O dtb -o sun8i-h3-spi-ardled.dtbo sun8i-h3-spi-ardled.dts
scp sun8i-h3-spi-ardled.dtbo root@192.168.0.33:/boot/overlay
```

And then run these commands on the target board:
```sh
cd /home/root
mv ardled.ko /lib/modules/$(uname -r)
depmod -a
```

Finally edit the `/boot/allwinnerEnv.txt` on the target and edit
the `overlays` line like this:
```sh
overlays=sun8i-h3-i2c0 sun8i-h3-i2c-ard101ph sun8i-h3-spi-ardled
```

And the reboot. After the reboot you should be able to write PWM
values (0-1023) to the LED of the arduino using this command:
```sh
echo 520 > /sys/bus/spi/devices/spi0.0/leds/ardled-0/brightness
```

In case that the module is not automatically loaded (check with
`lsmod` and search for `ardled`), then you can manually load it
with this command:

```sh
modprobe ardled
```

Then run the commands and unload with this command:
```sh
modprobe -r ardled
```

## Bash scripts
To test the project with the bash scripts you need to copy the scripts
to the remote device and then run it. In this case, you have two scripts
the one is using the spidev and i2c-tools and the other is based on the
loaded kernel modules.

First copy the modules to the target:

```sh
cd bash-scripts
scp *.sh root@192.168.0.33:/home/root
```

#### spidev bash script
To test this script you need to follow the above guide and run the spidev
module. Then from the target terminal run this:
```sh
cd /home/root
./bash-spidev-example.sh
```

#### module bash script
To test this script you need to follow the above guide to build and run
the kernel modules and then you can test with this command:

```sh
cd /home/root
./bash-modules-example.sh
```