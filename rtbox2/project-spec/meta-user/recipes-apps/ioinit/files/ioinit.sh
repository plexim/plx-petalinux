#!/bin/sh

# Digital Outs

ZYNQ_BASE=`cd /sys/class/gpio && grep -r zynqmp_gpio gpiochip* | sed -e "s/gpiochip\([0-9]*\).*/\1/"`
ZYNQ_BASE=$((ZYNQ_BASE+78))
for i in `seq $((ZYNQ_BASE)) $((ZYNQ_BASE+63))`
do
  echo $i > /sys/class/gpio/export
  echo out > /sys/class/gpio/gpio${i}/direction
  echo 0 > /sys/class/gpio/gpio${i}/value
done

# Voltages
POWER_BASE=`cd /sys/bus/i2c/devices/0-0074/gpio && ls -d gpiochip* | sed -e "s/gpiochip\([0-9]*\).*/\1/"`
echo $POWER_BASE > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio$POWER_BASE/direction 
echo 1 > /sys/class/gpio/gpio$POWER_BASE/value 
echo $((POWER_BASE+1)) > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio$((POWER_BASE+1))/direction 
echo 1 > /sys/class/gpio/gpio$((POWER_BASE+1))/value

# Current calibration
echo 20000 > /sys/bus/i2c/devices/0-0040/hwmon/hwmon1/shunt1_resistor
echo 20000 > /sys/bus/i2c/devices/0-0040/hwmon/hwmon1/shunt2_resistor

# Fan
echo 2 > /sys/class/hwmon/hwmon0/pwm1_enable

# RT Box 3 detection
echo $((ZYNQ_BASE+81)) > /sys/class/gpio/export

# eth1 / eth2
ip link set eth1 up
phy-reset eth1
ip link set eth1 down
ip link set eth2 up
phy-reset eth2
ip link set eth2 down

