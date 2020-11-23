#!/bin/bash
sleep 4
#insmode sdio wlan driver
/sbin/insmod /lib/modules/wlan/8189fs.ko

#disable rfkill
/usr/sbin/rfkill unblock wifi

sleep 4
#turn wlan0 up
/sbin/ifconfig wlan0 up

sleep 2

/bin/wpa_supplicant -Dnl80211 -c /etc/wpa_supplicant.conf -i wlan0 &

sleep 4
/sbin/udhcpc -i wlan0
