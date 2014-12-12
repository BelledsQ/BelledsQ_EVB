#########################################################################
# File Name: chk_wan2.sh
# Author: yuehb
# mail: yuehb@bellnett.com
# Created Time: Fri 15 Aug 2014 02:50:15 PM CST
#########################################################################
#!/bin/sh

IFNAME="eth0"
KEEPALIVE_SERVER=$1
INTERVAL=5

[ -e "/tmp/chkwan2" ] && exit 0
echo $$ > /tmp/chkwan2

[ -z $KEEPALIVE_SERVER  ] && KEEPALIVE_SERVER="www.baidu.com"
[ -z $IFNAME  ] && echo "IFNAME needed!" && exit 1

#. /lib/config/uci.sh
#NAME=$(uci_get airplay.unix)
#WWAN_IP=$(uci_get_state network.wan.ipaddr )

while true
do
#COUNT=$( ping -I $IFNAME $KEEPALIVE_SERVER -c 9  -q|grep "received"|cut -d ',' -f 2|cut -c 2-2 )
COUNT=$(cat /sys/class/net/eth0/carrier)

[ ! -e "/var/run/miniupnpd.pid" ] && {
	/etc/init.d/miniupnpd start
	sed '1s/^.*$/ext_ifname=eth0/'  -i  /var/etc/miniupnpd.conf
	/etc/init.d/miniupnpd stop
	/usr/sbin/miniupnpd -f /var/etc/miniupnpd.conf

}

if [ "${COUNT}" \> "0" ];then
	: 
	#echo $COUNT received >/tmp/chkwan2
else
	#echo "no packets received" > /tmp/chkwan2
	#kill `cat /var/run/udhcpc-eth0.pid`
	killall udhcpc
	#sed '1s/^.*$/ext_ifname=wlan0-1/'  -i  /var/etc/miniupnpd.conf
	#/usr/sbin/miniupnpd -f /var/etc/miniupnpd.conf
	#killall gmediarender
	#gmediarender  --ip-address $WWAN_IP   --friendly-name $NAME --uuid $NAME &
	/sbin/chkgmedia.sh
	rm -f  /tmp/chkwan2
	exit 0
fi

sleep $INTERVAL
done
