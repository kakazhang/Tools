#!/bin/bash
#insmode sdio wlan driver
/sbin/insmod /lib/modules/8189fs.ko

#disable rfkill
/usr/sbin/rfkill unblock wifi

/bin/sleep 1
#turn wlan0 up
/sbin/ifconfig wlan0 up

/bin/sleep 1

/bin/wpa_supplicant -Dnl80211 -c /etc/wpa_supplicant.conf -i wlan0 &

/bin/sleep 1
/sbin/udhcpc -i wlan0
