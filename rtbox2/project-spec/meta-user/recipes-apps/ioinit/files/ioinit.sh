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
MEASURE_HWMON=`ls /sys/bus/i2c/devices/0-0040/hwmon`
echo 20000 > /sys/bus/i2c/devices/0-0040/hwmon/${MEASURE_HWMON}/shunt1_resistor
echo 20000 > /sys/bus/i2c/devices/0-0040/hwmon/${MEASURE_HWMON}/shunt2_resistor
if [ -d /sys/bus/i2c/devices/0-0041/hwmon ]; then
  MEASURE_HWMON2=`ls /sys/bus/i2c/devices/0-0041/hwmon`
  echo 20000 > /sys/bus/i2c/devices/0-0040/hwmon/${MEASURE_HWMON}/shunt3_resistor
  echo 20000 > /sys/bus/i2c/devices/0-0041/hwmon/${MEASURE_HWMON2}/shunt1_resistor
  echo 20000 > /sys/bus/i2c/devices/0-0041/hwmon/${MEASURE_HWMON2}/shunt2_resistor
  echo 20000 > /sys/bus/i2c/devices/0-0041/hwmon/${MEASURE_HWMON2}/shunt3_resistor
fi

# Fan
echo 2 > /sys/bus/i2c/devices/0-0048/hwmon/hwmon0/pwm1_enable
echo 3000 > /sys/bus/i2c/devices/0-0048/hwmon/hwmon0/fan1_target
if [ -d /sys/bus/i2c/devices/0-004b/hwmon/hwmon1 ]; then
   echo 2 > /sys/bus/i2c/devices/0-004b/hwmon/hwmon1/pwm1_enable
   echo 800 > /sys/bus/i2c/devices/0-004b/hwmon/hwmon1/fan1_target
fi

# RT Box 3 detection
echo $((ZYNQ_BASE+81)) > /sys/class/gpio/export

# eth1 / eth2
ip link set eth1 up
phy-reset eth1
ip link set eth1 down
ip link set eth2 up
phy-reset eth2
ip link set eth2 down

