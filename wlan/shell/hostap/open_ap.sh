#!/bin/bash

ifconfig wlan0 down

sleep 1

iwconfig wlan0 mode monitor

ifconfig wlan0 up

ifconfig wlan0 192.168.0.1

udhcpd /etc/udhcpd.conf

hostapd /etc/hostapd.conf -B &
