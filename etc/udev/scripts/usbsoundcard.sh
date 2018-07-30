#!/bin/sh
devpath=$2
device_name=${devpath##*/}
cardno=${device_name:4:1}
deviceno=${device_name:6:1}
until [ $(pidof audiod) > /dev/null ] && [ $(pidof pulseaudio) > /dev/null ]
do
sleep 1
done
#Waiting for the ls-hub to come up for the below commands.
sleep 3
if [ $1 == "USB_MIC_ADDED" ];then
luna-send -n 1 luna://com.webos.service.audio/udev/event '{"event":"usb-mic-inserted", "soundcard_no":'$cardno', "device_no":'$deviceno'}'
elif [ $1 == "USB_MIC_REMOVED" ];then
luna-send -n 1 luna://com.webos.service.audio/udev/event '{"event":"usb-mic-removed", "soundcard_no":'$cardno', "device_no":'$deviceno'}'
elif [ $1 == "USB_HEADSET_ADDED" ];then
luna-send -n 1 luna://com.webos.service.audio/udev/event '{"event":"usb-headset-inserted", "soundcard_no":'$cardno', "device_no":'$deviceno'}'
elif [ $1 == "USB_HEADSET_REMOVED" ];then
luna-send -n 1 luna://com.webos.service.audio/udev/event '{"event":"usb-headset-removed", "soundcard_no":'$cardno', "device_no":'$deviceno'}'
fi

