#!/bin/sh

display_message()
{
        local ip=$1
        local m0="RT Box is ready."
        local m1="Box name: $(hostname)"
        local m2="IP: $ip"

        echo -n -e "/PLX_IMPORTANT_MESSAGE/$m0\n$m1\n$m2" | displog.sh
        echo -n "$m1" | displog.sh
	echo -n "$m2" | displog.sh
}


last_seen_ip=""
while true; do

        up_ip=$( \
		{ cat /sys/class/net/eth0/operstate | grep -q '^up'; } && \
			{ ip -4 addr show dev eth0 | awk '/inet /{gsub("/[0-9]+", ""); print $2}' | head -n 1; } \
	)

        if [ ! -z "$up_ip" ] && [ "$up_ip" != "$last_seen_ip" ]; then
		display_message "$up_ip"
		last_seen_ip="$up_ip"
	fi

        [ -f /sys/devices/jailhouse/cells/1/state ] && \
		{ cat /sys/devices/jailhouse/cells/1/state | grep -q '^running'; } && \
			exit

        sleep 1
done

