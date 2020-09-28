#!/bin/sh

if [ "$IFACE" = "lo" ]; then exit 0; fi

case "$METHOD" in
	dhcp)
		echo -n "$IFACE: waiting for IP address ..." | displog.sh
	;;
	
	static)
		echo -n "$IFACE: setting static IP address ..." | displog.sh
	;;

	*)
		true
	;;
esac

exit 0

