1.bootchart source code is used to build bootchart.jar
2.build steps:
	#sudo apt-get install ant
	#ant
  if build success, bootchart.jar will be placed in bootchart root directory.

3.usage
  for android8 and later
	#adb shell touch /data/bootchart/enabled
    #reboot
    #adb shell pull /data/bootchart/
    #tar -zcf bootchart.tgz /databootchart/*
	#java -jar bootchart.jar bootchart.tgz

	then we will get a bootchart.png
