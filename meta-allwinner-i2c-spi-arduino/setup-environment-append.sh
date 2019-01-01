#!/bin/bash

# This environment setup scrips is not meant to run individually
# and it should be called from another setup-environment script.
# That way this meta layer can apply its custom configurations.
#
# Author: dimitris.tassopoulos@native-instruments.de

REL_PATH=$1

# Override the default maschine bblayers.conf
echo "Replacing bblayers.conf in ${BBPATH}/conf/bblayers.conf meta-allwinner-i2c-spi-arduino"
cp $REL_PATH/conf/bblayers.conf ${BBPATH}/conf/bblayers.conf

if [ -f ${REL_PATH}/conf/machine/${MACHINE}-extra.conf ]; then
	echo -e "Appending extra configuration to the local.conf."
    echo -e "${REL_PATH}/conf/machine/${MACHINE}-extra.conf >> ${BBPATH}/conf/local.conf"
	cat ${REL_PATH}/conf/machine/${MACHINE}-extra.conf >> ${BBPATH}/conf/local.conf
fi

#Get available images for this layer
LIST_IMAGES=$(ls $REL_PATH/recipes-core/images/*.bb | sed s/\.bb//g | sed -r 's/^.+\///' | xargs -I {} echo -e "\t"{})

cat <<EOF

These are the extra images that you can build from this BSP layer:
	${LIST_IMAGES}
EOF

