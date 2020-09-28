#!/bin/sh

mkdir -p /run/media/mmcblk0p1
mount -t ext4 /dev/mmcblk0p1 /run/media/mmcblk0p1 -o ro
if [ $? -ne 0 ]; then
   /sbin/mkfs.ext4 /dev/mmcblk0p1
   mount /dev/mmcblk0p1 /run/media/mmcblk0p1 -o ro
fi
if [ -d /etc/dropbear ]; then
   if [ ! -d /run/media/mmcblk0p1/etc/dropbear ]; then
   	mount -o remount,rw /dev/mmcblk0p1 /run/media/mmcblk0p1
   	mkdir -p /run/media/mmcblk0p1/etc/dropbear
   fi
   rmdir /etc/dropbear && ln -s /run/media/mmcblk0p1/etc/dropbear /etc
fi

mkdir -p /run/media/mmcblk1p1
mount /dev/mmcblk1p1 /run/media/mmcblk1p1 -o ro

if [ ! -b /dev/sda1 ]; then
   (
   echo o
   echo n
   echo p
   echo 1
   echo 1
   echo  
   echo w
   ) | fdisk -S 32 -H 64 /dev/sda
   /sbin/mkfs.ext4 /dev/sda1
fi

if [ -e /dev/sda1 ]; then
   mkdir -p /run/media/sda1
   mount /dev/sda1 /run/media/sda1 -o ro
fi

if [ -d /run/media/mmcblk1p1/config ]; then
   cd /run/media/mmcblk1p1/config && find . -type f | \
      while read file
      do 
         cat ${file} | tr -d '\r' > /${file} 
      done
fi

if [ ! -f /run/media/mmcblk1p1/config/etc/hostname ]; then 
   echo "rtbox-"`sed -e "s/://g" /sys/class/net/eth0/address` > /etc/hostname
fi

RTBOX3=`cat /sys/class/gpio/gpio497/value`

if [ $RTBOX3 == "1" ] 
then
   cp /var/rtbox/rtbox3.service /etc/avahi/services
else
   cp /var/rtbox/rtbox2.service /etc/avahi/services
fi

