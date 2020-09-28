#!/bin/sh

if [ "$IFACE" = "lo" ]; then exit 0; fi

case "$METHOD" in
	dhcp)
		true
	;;
	
	static)
		echo -n "$IFACE: static address $IF_ADDRESS" | displog.sh
	;;

	*)
		true
	;;
esac

exit 0

