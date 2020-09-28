#!/bin/sh

echo firmware > /sys/class/remoteproc/remoteproc0/firmware
/usr/bin/run_autostart.pl
