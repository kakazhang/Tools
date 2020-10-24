#!/bin/bash

adb root
adb remount
adb pull /data/bootchart/
cd bootchart
tar -zcf bootchart.tgz *
cd ../
cp bootchart/bootchart.tgz .
java -jar bootchart.jar bootchart.tgz
