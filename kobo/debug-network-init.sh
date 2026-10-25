#!/bin/sh
# USB Ethernet gadget for debug access (telnet + ftp, no login), with a
# DHCP server so the PC side gets its IP automatically -- no manual
# `ip addr add` needed. Also brings up the onboard Bluetooth radio, for
# testing LK8000's BLE: GATT sensor support (USE_BLE=y) -- see the
# BLUETOOTH section below.
# Installed as /mnt/onboard/LK8000/kobo/init.sh -- which kobo/rcS sources
# automatically on every boot -- only when built with KOBO_DEBUG_NET=y.
# NEVER enable this for a build given to end users: it opens an
# unauthenticated root shell and full filesystem FTP access to anyone
# who plugs the device into a USB port. It also overwrites any custom
# init.sh a user may already have of their own.
#
# On the PC: the new USB network interface (e.g. `enx...` from `ip link`)
# should get a DHCP lease automatically (NetworkManager does this by
# default for a new wired-style interface). Then, using either the fixed
# IP or, once the PC's mDNS resolver (avahi/systemd-resolved on Linux,
# Bonjour on Windows, built-in on macOS) has seen the device's
# announcement, lk8000.local:
#   telnet lk8000.local          -- root shell (busybox telnetd, no login)
#   ftp lk8000.local             -- file transfer, rooted at / (no login,
#                                    full access -- trusted direct USB link only)
# Output of this script goes to LK8000/kobo/init.log for debugging.
#
# arcotg_udc is built into some kernels (e.g. mx6sll-ntx) rather than
# being a loadable module; its insmod is harmlessly skipped below if the
# .ko file doesn't exist. Adjust the module path/name to match your
# platform's /drivers/current/usb/gadget/ if needed.
#
# dnsmasq and avahi-daemon are bundled to /opt/LK8000/bin (built with the
# Buildroot SDK, see buildroot/configs/kobo_defconfig) rather than being
# part of the stock Kobo firmware's busybox, which has no udhcpd/zcip/mDNS
# applet. Both are built against the bleeding-edge glibc under
# /opt/LK8000/lib, same as LK8000-KOBO itself, so they're launched the
# same way: via our own bundled ld.so directly (neither was linked with a
# custom --dynamic-linker/--rpath, so LD_LIBRARY_PATH stands in for that).

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

	# avahi-daemon: announces the device as lk8000.local over mDNS on
	# usb0 (config: kobo/avahi-daemon.conf, installed to
	# /opt/LK8000/etc/avahi-daemon.conf). --no-drop-root is needed
	# because there's no 'avahi' user in the stock Kobo's /etc/passwd for
	# it to setuid to (this whole environment is already running as root
	# anyway, being a single-user busybox system) -- but even with
	# --no-drop-root, avahi-daemon's make_runtime_dir() still does an
	# unconditional getpwnam()/getgrnam() lookup (just to chown its
	# runtime dir), so the 'avahi' user/group must still exist in
	# /etc/passwd/group or startup fails outright; a nominal, unused
	# entry is enough since we never actually setuid to it. Confirmed on
	# real hardware: its compiled-in runtime dir is /run/avahi-daemon,
	# not /var/run/avahi-daemon (this device's busybox has /var/run but
	# no /run at all -- `mkdir -p` creates the missing parent too).
	# No -D/-s here (daemonize+syslog): there's no syslogd on this
	# device, which would silently swallow startup errors -- run it the
	# same way dbus-daemon/bluetoothd below do, backgrounded with `&` so
	# stderr keeps going to this script's own init.log redirection.
	grep -q "^avahi:" /etc/passwd || echo "avahi:x:150:150:avahi:/run/avahi-daemon:/bin/false" >> /etc/passwd
	grep -q "^avahi:" /etc/group || echo "avahi:x:150:" >> /etc/group
	mkdir -p /run/avahi-daemon
	LD_LIBRARY_PATH=/opt/LK8000/lib /opt/LK8000/lib/ld-linux-armhf.so.3 /opt/LK8000/bin/avahi-daemon \
		--file=/opt/LK8000/etc/avahi-daemon.conf --no-drop-root &

	telnetd -l /bin/sh
	tcpsvd -E 0.0.0.0 21 ftpd -w -A / &

	# --- BLUETOOTH: bring up the onboard BLE radio, for testing LK8000's
	# BLE: GATT sensor support. Not needed for USE_BLE=y itself (that just
	# needs a working hci0 + bluetoothd already present, however they got
	# there) -- this is what actually makes that true on a stock Kobo,
	# which doesn't power on or attach its Bluetooth chip on its own.
	# Adapted from
	# https://github.com/F5OEO/LK8000/commit/b710899db44ee9fdf196db105df5819815b4ca1c
	# (kobo/ble.sh), minus its bluealsa/bluetoothctl-connect audio-pairing
	# bits, which are for BT audio and unrelated to BLE GATT sensors.
	#
	# Confirmed on a real Kobo Clara BW (model string "SN-N506", which
	# this Netronix platform code treats identically to the Clara 2E):
	# the chip is a Marvell/NXP one despite a leftover, unused
	# /etc/firmware/BCM4345C0.hcd on the device -- hciattach's plain "any"
	# type (no vendor firmware upload step) is what actually works, not
	# "bcm43xx". Add further `elif [ ... ]` branches here for other
	# models/chips as they get confirmed working; unmatched models fall
	# through with no Bluetooth support, same as today.
	#
	# gattlib's underlying GDBus defaults to a socket path
	# ("/run/dbus/...") that doesn't match where this stock dbus-daemon
	# actually listens ("/var/run/dbus/...") -- LK8000 itself works
	# around that (see DBUS_SYSTEM_BUS_ADDRESS in
	# Comm/Bluetooth/GattlibBackend.cpp), so it's not needed here too;
	# dbus-daemon is only started here because bluetoothd needs it
	# running and nothing else on a stock Kobo starts it.
	for i in /var/run/dbus /var/lib/dbus; do
		mkdir -p $i
	done
	/bin/dbus-uuidgen > /var/lib/dbus/machine-id
	/bin/dbus-daemon --system &

	insmod /drivers/mx6sll-ntx/wifi/sdio_bt_pwr.ko

	model=`dd if=/dev/mmcblk0 bs=8 count=1 skip=64 2>/dev/null`
	if [ "`expr substr "$model" 1 7`" = SN-N418 ] ; then # Libra 2
		/sbin/rtk_hciattach -n -s 115200 ttymxc1 rtk_h5 &
	elif [ "`expr substr "$model" 1 7`" = SN-N506 ] ; then # Clara 2E / Clara BW
		insmod /drivers/mx6sll-ntx/wifi/mlan.ko
		insmod /drivers/mx6sll-ntx/wifi/moal.ko mod_para=nxp/wifi_mod_para_sd8987.conf
		insmod /drivers/mx6sll-ntx/wifi/sdio_wifi_pwr.ko
		/sbin/hciattach -n ttymxc1 any 1500000 flow -t 20 &
	fi

	sleep 5
	hciconfig hci0 up

	/libexec/bluetooth/bluetoothd -n -d > /mnt/onboard/LK8000/kobo/bluetoothd.log 2>&1 &
} >> /mnt/onboard/LK8000/kobo/init.log 2>&1
