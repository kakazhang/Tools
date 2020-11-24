#!/bin/bash

insmod /lib/modules/usb_f_mass_storage.ko
usleep 100
insmod /lib/modules/g_mass_storage.ko

#delete all functions
rm -rf /sys/kernel/config/usb_gadget/g1/functions/*

#create mass_storage
mkdir /sys/kernel/config/usb_gadget/g1/functions/mass_storage.gs0

#set shared device
echo "/dev/mmcblk1" >> /sys/kernel/config/usb_gadget/g1/functions/mass_storage.gs0/lun.0/file

echo "mass_storage" >> /sys/kernel/config/usb_gadget/g1/configs/b.1/strings/0x409/configuration

rm -rf /sys/kernel/config/usb_gadget/g1/configs/b.1/mass_storage.gs0

ln -s /sys/kernel/config/usb_gadget/g1/functions/mass_storage.gs0 /sys/kernel/config/usb_gadget/g1/configs/b.1/

echo 1 >> /sys/kernel/config/usb_gadget/g1/os_desc/use

echo "ci_hdrc.0" >> UDC
