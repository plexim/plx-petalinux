do_install_append() {
   awk '{sub(/iface eth1 inet dhcp/,"   udhcpc_opts -t 1 -A 5\nauto eth3\n#iface eth3 inet dhcp\n#   udhcpc_opts -t 1 -A 5") } 1' ${D}${sysconfdir}/network/interfaces > /tmp/tmpif
   mv /tmp/tmpif ${D}${sysconfdir}/network/interfaces
}

