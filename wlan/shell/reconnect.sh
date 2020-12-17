#!/bin/bash

rmmod 8189fs

killall hostapd

killall udhcpd

turn_on_wlan.sh &
