#!/bin/sh
#
# Called from udev
#
# Attempt to mount any added block devices and umount any removed devices


MOUNT="/bin/mount"
UMOUNT="/bin/umount"
if [ ` expr match "$DEVNAME" "/dev/sdb1" ` -eq 0 ];
then
	logger "udev/mount_plexim.sh" "[$DEVNAME] is ignored"
	exit 0
fi
logger "udev/mount_plexim.sh" "processing [$DEVNAME]"

if [ "$ACTION" = "add" ]; then
	$MOUNT /run/media/sdb1 /www/pages/dav/usb -o bind
fi


if [ "$ACTION" = "remove" ]; then
	$UMOUNT /www/pages/dav/usb
fi
