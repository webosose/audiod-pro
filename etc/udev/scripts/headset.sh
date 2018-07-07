#!/bin/sh
if [ $1 == "HEADSETMIC_ADDED" ];then
luna-send -n 1 luna://com.webos.service.audio/udev/event '{"event":"headset-mic-inserted"}'
elif [ $1 == "HEADSET_REMOVED" ];then
luna-send -n 1 luna://com.webos.service.audio/udev/event '{"event":"headset-removed"}'
elif [ $1 == "HEADSET_ADDED" ];then
luna-send -n 1 luna://com.webos.service.audio/udev/event '{"event":"headset-inserted"}'
fi

