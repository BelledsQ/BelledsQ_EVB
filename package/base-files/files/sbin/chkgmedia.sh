#########################################################################
# File Name: chkgmedia.sh
# Author: yuehb
# mail: yuehb@bellnett.com
# Created Time: Mon 18 Aug 2014 04:08:36 PM CST
#########################################################################
#!/bin/sh

. /lib/config/uci.sh
LAN_STATUS=$(uci_get network.lan.status)
[ $LAN_STATUS == "lan" ] && {
	killall gmediarender 
	kill `cat /tmp/chkwan2`  
	rm /tmp/chkwan2
}

WAN_IP=$(ifconfig wlan0-1|grep "inet addr")
WAN2_IP=$(ifconfig eth0|grep "inet addr")
WAN_STATUS=$(cat /sys/class/net/wlan0-1/carrier)

GMEADIA=$(uci_get system.gmediarender.gmediarender_mode)
rm /usr/sbin/gmediarender
ln -s /usr/sbin/gmediarender$GMEADIA /usr/sbin/gmediarender
mkfifo /tmp/mpg123_pipe

[ -z "$WAN_IP" -a -z "$WAN2_IP" ] || [ -n "$WAN_IP" -a "$WAN_STATUS" == "0" ] && {
	NAME=$(uci_get airplay.unix)
	LAN_IP=$(uci_get network.lan.ipaddr)

	killall gmediarender
	/etc/init.d/miniupnpd stop
	gmediarender  --ip-address $LAN_IP --friendly-name $NAME --uuid $NAME &
}

