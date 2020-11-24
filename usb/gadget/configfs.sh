#!/bin/bash
/sbin/insmod /lib/modules/libcomposite.ko
usleep 100
#mount configfs filesystem
/bin/mount -t configfs none /sys/kernel/config
#create g1 in usb_gadget
/bin/mkdir /sys/kernel/config/usb_gadget/g1

#set idVendor idProduct
/bin/echo 0x2CA3 >> /sys/kernel/config/usb_gadget/g1/idVendor
/bin/echo 0x0130 >> /sys/kernel/config/usb_gadget/g1/idProduct
/bin/echo 0x0200 >> /sys/kernel/config/usb_gadget/g1/bcdUSB

#create string dir
/bin/mkdir /sys/kernel/config/usb_gadget/g1/strings/0x409
/bin/echo "2014040920201008" >> /sys/kernel/config/usb_gadget/g1/strings/0x409/serialnumber
/bin/echo "tang" >> /sys/kernel/config/usb_gadget/g1/strings/0x409/manufacturer
/bin/echo "river" >> /sys/kernel/config/usb_gadget/g1/strings/0x409/product

#create config
/bin/mkdir /sys/kernel/config/usb_gadget/g1/configs/b.1
/bin/mkdir /sys/kernel/config/usb_gadget/g1/configs/b.1/strings/0x409

