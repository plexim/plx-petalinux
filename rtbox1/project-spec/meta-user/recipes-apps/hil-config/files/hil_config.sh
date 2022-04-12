#!/bin/sh

mkdir /media/mmcblk0p1
/bin/mount /dev/mmcblk0p1 /media/mmcblk0p1 -o ro
if [ -d /media/mmcblk0p1/config ]; then
   cd /media/mmcblk0p1/config && find . -type f | \
      while read file
      do 
         cat ${file} | tr -d '\r' > /${file} 
      done
fi

mkdir /media/usb

#allow full access from webserver to mounted USB disks
usermod -a -G disk www-data

if [ ! -f /media/mmcblk0p1/config/etc/hostname ]; then 
   echo "rtbox-"`sed -e "s/://g" /sys/class/net/eth0/address` > /etc/hostname
fi

mkdir /media/nand
if [ ! -c /dev/ubi0 ]; then
   /usr/sbin/ubiattach /dev/ubi_ctrl -m 0
   if [ $? -ne 0 ]; then
      /usr/sbin/ubiformat /dev/mtd0 -y
      /usr/sbin/ubiattach /dev/ubi_ctrl -m 0
   fi
fi
if [ ! -c /dev/ubi0_0 ]; then
   /usr/sbin/ubimkvol /dev/ubi0 -N nand -S 4012
   /usr/sbin/mkfs.ubifs /dev/ubi0_0
fi

/bin/mount -t ubifs /dev/ubi0_0 /media/nand -o ro
if [ -d /etc/dropbear ]; then
   if [ ! -f /media/nand/etc/dropbear/dropbear_rsa_host_key ]; then
   	mount -o remount,rw /media/nand
   	mkdir -p /media/nand/etc/dropbear
        /usr/sbin/dropbearkey -t rsa -f /media/nand/etc/dropbear/dropbear_rsa_host_key
        mount -o remount,ro /media/nand
   fi
   rmdir /etc/dropbear && ln -s /media/nand/etc/dropbear /etc
fi

if [ -d /sys/bus/i2c/devices/i2c-1 ]
then
   mv /www/pages/index.html.rtbox1 /www/pages/index.html
else
   mv /www/pages/index.html.rtboxce /www/pages/index.html
fi

