#!/bin/sh

export QT_QPA_PLATFORM=rtbox_oled
export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=/dev/input/by-id/display-buttons:grab=1 

exec display-qt-test.bin
