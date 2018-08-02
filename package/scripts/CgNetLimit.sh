#!/bin/bash

DEV=$(ip route show | head -1 | awk '{print $5}')
BASEDIR=$(dirname $(readlink -f $0))
CGCLASSID=0x10010
MARKID=42
config_file_name=tmp_iptables_rule

# Help
display_help() {
  echo -e "Usage : $0 \e[4mLIMITS\e[24m [\e[4mOPTIONS\e[24m]"
  echo -e "Set bandwhith and add latency for a net_cls cgroup."
  echo -e ""
  echo -e "\e[1m\e[4mLIMITS\e[0m:"
  echo -e "\e[1m-u\e[0m       Limit upload speed."
  echo -e "\e[1m-d\e[0m       Limit download speed."
  echo -e "\e[1m-l\e[0m       Add latency (egress latency)."
  echo -e ""
  echo -e "\e[1m\e[4mOPTIONS\e[0m:"
  echo -e "\e[1m-c\e[0m       Specifies a cgroup classid (value in net_cls.classid), default : $CGCLASSID."
  echo -e "\e[1m-i\e[0m       Specifies an interface name, default : $DEV."
  echo -e "\e[1m-m\e[0m       Specifies a MarkID which will be applied to the packets, default : $MARKID."
  echo -e "\e[1m-x\e[0m       Remove all limits."
  echo -e "\e[1m-h\e[0m       Show this help."
}

delete_tc() {
  /sbin/tc qdisc del dev $DEV root 2> /dev/null > /dev/null
}

delete_ipt_rules() {
  iptables -D OUTPUT 1 -m cgroup --cgroup $CGCLASSID
  iptables -D POSTROUTING -t mangle -j CONNMARK --save-mark
  iptables -D PREROUTING -t mangle -j CONNMARK --restore-mark
  iptables -D INPUT -m connmark ! --mark $MARKID -j ACCEPT
  iptables -D INPUT -p tcp -m hashlimit --hashlimit-name hl1 --hashlimit-above $(cat /tmp/$config_file_name)/s -j DROP
  rm /tmp/$config_file_name
}

init_ipt_rules() {
  iptables -I OUTPUT 1 -m cgroup --cgroup $CGCLASSID -j MARK --set-mark $MARKID
  iptables -A POSTROUTING -t mangle -j CONNMARK --save-mark
  iptables -A PREROUTING -t mangle -j CONNMARK --restore-mark
  iptables -A INPUT -m connmark ! --mark $MARKID -j ACCEPT
  iptables -A INPUT -p tcp -m hashlimit --hashlimit-name hl1 --hashlimit-above $D_LIMIT/s -j DROP
  echo $D_LIMIT > /tmp/$config_file_name
}

limit_rate_latency_egress() { # $1 : interface, $2 : rate, $3 : latency
  /sbin/tc qdisc del dev $1 root 2> /dev/null > /dev/null
  /sbin/tc qdisc add dev $1 root handle 1: htb
  /sbin/tc class add dev $1 parent 1: classid 1:10 htb rate $2 ceil $2
  /sbin/tc qdisc add dev $1 parent 1:10 handle 2: netem delay $3
  /sbin/tc filter add dev $1 parent 1: handle $MARKID fw classid 1:10
}

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

while getopts ":hd:u:l:c:m:i:x" option
do
  case $option in
	u)
		U_FLAG=1
    U_LIMIT=$OPTARG
		;;
	d)
		D_FLAG=1
    D_LIMIT=$OPTARG
		;;
  l)
    DELAY_FLAG=1
    DELAY_LIMIT=$OPTARG
    ;;
  c)
    CGCLASSID=$OPTARG
    ;;
  m)
    MARKID=$OPTARG
    ;;
  i)
    DEV=$OPTARG
    ;;
  h)
    display_help
    exit 2
    ;;
  x)
    delete_tc
    delete_ipt_rules
    exit 0
    ;;
  :)
    echo "The '$OPTARG' options requires an argument" >&2
    exit 1
    ;;
  \?)
    echo "'$OPTARG' : invalid option check help page"
    exit 1
    ;;
  esac
done

if [[ "$D_FLAG" -eq "1" ]] && [[ "$(echo $D_LIMIT | tr -dc 'a-zA-Z')" != "kb" ]]; then
  echo "Download limit (-d) unit should be in kilobyte (kb)"
  exit 1
elif [[ "$U_FLAG" -eq "1" ]] && [[ "$DELAY_FLAG" -eq "1" ]] && [[ "$D_FLAG" -eq "1" ]]; then
	limit_rate_latency_egress $DEV $U_LIMIT $DELAY_LIMIT
  init_ipt_rules
  exit 0
else
  echo "You should provide the three options : -u, -d, -l"
  exit 1
fi
