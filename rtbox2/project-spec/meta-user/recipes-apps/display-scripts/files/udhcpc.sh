#!/bin/sh

case "$1" in
	deconfig)
		true
	;;
	renew)
		true
	;;
	bound)
		echo -n "$interface: got address $ip" | displog.sh
	;;
	*)
		true
	;;
esac

