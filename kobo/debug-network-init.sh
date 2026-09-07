#!/bin/sh
# USB Ethernet gadget for debug access (telnet + ftp, no login), with a
# DHCP server so the PC side gets its IP automatically -- no manual
# `ip addr add` needed.
# Installed as /mnt/onboard/LK8000/kobo/init.sh -- which kobo/rcS sources
# automatically on every boot -- only when built with KOBO_DEBUG_NET=y.
# NEVER enable this for a build given to end users: it opens an
# unauthenticated root shell and full filesystem FTP access to anyone
# who plugs the device into a USB port. It also overwrites any custom
# init.sh a user may already have of their own.
#
# On the PC: the new USB network interface (e.g. `enx...` from `ip link`)
# should get a DHCP lease automatically (NetworkManager does this by
# default for a new wired-style interface). Then:
#   telnet 192.168.2.1          -- root shell (busybox telnetd, no login)
#   ftp 192.168.2.1             -- file transfer, rooted at / (no login,
#                                   full access -- trusted direct USB link only)
# Output of this script goes to LK8000/kobo/init.log for debugging.
#
# arcotg_udc is built into some kernels (e.g. mx6sll-ntx) rather than
# being a loadable module; its insmod is harmlessly skipped below if the
# .ko file doesn't exist. Adjust the module path/name to match your
# platform's /drivers/current/usb/gadget/ if needed.
#
# dnsmasq is bundled to /opt/LK8000/bin (built with the Buildroot SDK,
# see buildroot/configs/kobo_defconfig) rather than being part of the
# stock Kobo firmware's busybox, which has no udhcpd/zcip applet. It's
# built against the bleeding-edge glibc under /opt/LK8000/lib, same as
# LK8000-KOBO itself, so it's launched the same way: via our own bundled
# ld.so directly (it wasn't linked with a custom --dynamic-linker/--rpath,
# so LD_LIBRARY_PATH stands in for that).

{
	insmod /drivers/current/usb/gadget/arcotg_udc.ko 2>/dev/null
	insmod /drivers/current/usb/gadget/g_ether.ko

	ifconfig usb0 192.168.2.1 netmask 255.255.255.0 up

	mkdir -p /dev/pts
	mount -t devpts none /dev/pts

	LD_LIBRARY_PATH=/opt/LK8000/lib /opt/LK8000/lib/ld-linux-armhf.so.3 /opt/LK8000/bin/dnsmasq \
		--interface=usb0 --bind-interfaces --port=0 \
		--dhcp-range=192.168.2.10,192.168.2.50,255.255.255.0,12h \
		--dhcp-leasefile=/tmp/dnsmasq.leases --pid-file=/tmp/dnsmasq.pid

	telnetd -l /bin/sh
	tcpsvd -E 0.0.0.0 21 ftpd -w -A / &
} >> /mnt/onboard/LK8000/kobo/init.log 2>&1
