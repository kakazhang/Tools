#!/bin/bash

insmod /lib/modules/u_ether.ko
insmod /lib/modules/usb_f_rndis.ko
insmod /lib/modules/usb_f_ecm.ko
insmod /lib/modules/usb_f_ecm_subset.ko

#delete all functions
rm -rf /sys/kernel/config/usb_gadget/g1/functions/*

#create mass_storage
mkdir /sys/kernel/config/usb_gadget/g1/functions/rndis.gs1

echo "rndis" >> /sys/kernel/config/usb_gadget/g1/configs/b.1/strings/0x409/configuration

rm -rf /sys/kernel/config/usb_gadget/g1/configs/b.1/rndis.gs1

ln -s /sys/kernel/config/usb_gadget/g1/functions/rndis.gs1 /sys/kernel/config/usb_gadget/g1/configs/b.1/

echo 1 >> /sys/kernel/config/usb_gadget/g1/os_desc/use

echo "ci_hdrc.0" >> UDC
