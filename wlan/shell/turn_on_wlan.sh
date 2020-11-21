#!/bin/bash
#insmode sdio wlan driver
insmod /home/root/driver/rtl8189/8189fs.ko

#disable rfkill
rfkill unblock wifi

#turn wlan0 up
ifconfig wlan0 up

sleep 1

wpa_supplicant -Dnl80211 -c /etc/wpa_supplicant.conf -i wlan0 &
