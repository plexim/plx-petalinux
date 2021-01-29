#!/bin/sh

FAN_DEV=$(ls /sys/bus/i2c/devices/0-0048/hwmon*)
echo 3000 > /sys/class/hwmon/${FAN_DEV}/fan1_target
echo 2 > /sys/class/hwmon/${FAN_DEV}/pwm1_enable

# Current calibration
if [ -d /sys/bus/i2c/devices/0-0040/hwmon ]; then
  echo 20000 > /sys/bus/i2c/devices/0-0040/hwmon/hwmon0/shunt1_resistor
  echo 20000 > /sys/bus/i2c/devices/0-0040/hwmon/hwmon0/shunt2_resistor
fi

# Disable Digital Outs

ZYNQ_BASE=`cd /sys/class/gpio && grep zynq_gpio gpiochip*/label | sed -e "s/gpiochip\([0-9]*\).*/\1/"`
echo $((ZYNQ_BASE+91)) > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio$((ZYNQ_BASE+91))/direction
echo 1 > /sys/class/gpio/gpio$((ZYNQ_BASE+91))/value

# Digital Outs

for i in `seq $((ZYNQ_BASE+54)) $((ZYNQ_BASE+85))`
do
  echo $i > /sys/class/gpio/export
  echo out > /sys/class/gpio/gpio${i}/direction
  echo 0 > /sys/class/gpio/gpio${i}/value
done

# LEDs

LED_BASE=`cd /sys/class/gpio && grep 41200000 gpiochip*/label | sed -e "s/gpiochip\([0-9]*\).*/\1/"`
for i in `seq $LED_BASE $((LED_BASE+3))`
do
  echo $i > /sys/class/gpio/export
  echo out > /sys/class/gpio/gpio${i}/direction
  echo 0 > /sys/class/gpio/gpio${i}/value
done

# Turn on "Power" LED

echo 1 > /sys/class/gpio/gpio$((LED_BASE+3))/value

# LED read-back

for i in `seq 1008 1011`
do
  echo $i > /sys/class/gpio/export
  echo in > /sys/class/gpio/gpio${i}/direction
done

# Analog Power
POWER_BASE=`cd /sys/class/gpio && grep -r 41250000 gpiochip* | sed -e "s/gpiochip\([0-9]*\).*/\1/"`
echo $POWER_BASE > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio$POWER_BASE/direction
echo 1 > /sys/class/gpio/gpio$POWER_BASE/value


# I2C gpios for SFP+ ports - RateSelect and TXDisable
SFP_RS_BASE=`cd /sys/bus/i2c/devices/0-0021/gpio && ls -d gpiochip* | sed -e "s/gpiochip\([0-9]*\).*/\1/"`
for i in `seq $SFP_RS_BASE $((SFP_RS_BASE+11))`
do
  echo $i > /sys/class/gpio/export
  echo out > /sys/class/gpio/gpio${i}/direction
  echo 0 > /sys/class/gpio/gpio${i}/value
done

# Digital in pull up/down

if [ -e /sys/bus/i2c/devices/0-0074/gpio ]; then
  PULLUPDOWN_BASE=`cd /sys/bus/i2c/devices/0-0074/gpio && ls -d gpiochip* | sed -e "s/gpiochip\([0-9]*\).*/\1/"`
  for i in `seq $PULLUPDOWN_BASE $((PULLUPDOWN_BASE+31))`
  do
    echo $i > /sys/class/gpio/export
    echo in > /sys/class/gpio/gpio${i}/direction
  done
fi


# Switch on Ready LED when using static IP addresses

STATIC_CONF=`grep -E "^iface eth0" /etc/network/interfaces | grep static`

if [ "a$STATIC_CONF" != "a" ]; then
   echo 1 > /sys/class/gpio/gpio$((LED_BASE+2))/value
fi

# Invert HOLD signals for DACs on 1.0 boards
BOARD_REV=`dd if=/sys/bus/i2c/devices/0-0052/eeprom bs=1 skip=2 count=4 2>/dev/null| hexdump -vx | head -n 1 | awk '{ print $2*1000 + $3 }'`
if [ ${BOARD_REV} -eq 1000 ]; then
  poke 0x43c0000c 0x01;
fi

# Configure ADCs for 1.2 boards and newer
echo $((POWER_BASE+1)) > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio$((POWER_BASE+1))/direction
if [ ${BOARD_REV} -ge 1002 ]; then
  echo 1 > /sys/class/gpio/gpio$((POWER_BASE+1))/value
  # CAN termination
  echo $((SFP_RS_BASE+12)) > /sys/class/gpio/export
  echo $((SFP_RS_BASE+13)) > /sys/class/gpio/export
  echo out > /sys/class/gpio/gpio$((SFP_RS_BASE+12))/direction
  echo out > /sys/class/gpio/gpio$((SFP_RS_BASE+13))/direction
fi


# Copy version information to RAM
/etc/init.d/versionInit.pl

