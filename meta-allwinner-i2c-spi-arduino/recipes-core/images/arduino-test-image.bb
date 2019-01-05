SUMMARY = "Allwinner console image"
LICENSE = "MIT"
AUTHOR = "Dimitris Tassopoulos"

inherit allwinner-image

# Add our custom tools
IMAGE_INSTALL += " \
    i2c-tools \
    spitools \
    fdt \
"