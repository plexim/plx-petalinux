do_install_append() {
   awk '{sub(/iface eth1 inet dhcp/,"   udhcpc_opts -t 1 -A 5 -O 121\niface eth1 inet dhcp\n   udhcpc_opts -t 1 -A 5 -O 121") } 1' ${D}${sysconfdir}/network/interfaces > /tmp/tmpif
   mv /tmp/tmpif ${D}${sysconfdir}/network/interfaces
}

