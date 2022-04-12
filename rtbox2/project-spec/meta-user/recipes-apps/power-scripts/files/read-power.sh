#!/bin/sh

DIR=/sys/class/hwmon/hwmon1


calc(){ awk "BEGIN { print $*}"; }


while /bin/true; do
        v1=`cat $DIR/in1_input`
        i1=`cat $DIR/curr1_input`

        v2=`cat $DIR/in2_input`
        i2=`cat $DIR/curr2_input`

        v3=`cat $DIR/in3_input`
        i3=`cat $DIR/curr3_input`

        p1=$(calc "$v1*$i1/1.e6")
        p2=$(calc "$v2*$i2/1.e6")
        p3=$(calc "$v3*$i3/1.e6")
        p=$(calc "$p1+$p2+$p3")

        #echo $p1 $p2 $p3 $p
        printf "%5.1f %5.1f %5.1f %10.1f\n" $p1 $p2 $p3 $p

        sleep 1
done
