#!/bin/sh

while /bin/true; do
   FPGA_TEMP=`cat /sys/bus/iio/devices/iio:device0/in_temp0_ps_temp_raw`
   TEMP=`awk "BEGIN{
      t=(${FPGA_TEMP} - 36058) * 7.771514892 / 1000;
      print t;
   }"`
   echo $TEMP
   sleep 1
done
