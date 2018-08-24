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
  echo -e "Units are the same as the TC units (see man tc)."
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

# Helper function to handle unit conversion
toBytes() {
  echo $1 | awk \
          'BEGIN{IGNORECASE = 1}
          /[0-9]$/{print $1};
          /kbps?$/{printf "%ukb\n", $1; exit 0};
          /mbps?$/{printf "%umb\n", $1; exit 0};
          /gbps?$/{printf "%ugb\n", $1; exit 0};
          /bps?$/{printf "%ub\n", $1};'
}

delete_tc() {
  /sbin/tc qdisc del dev $DEV root 2> /dev/null > /dev/null
}

delete_ipt_rules() {
  iptables -D OUTPUT 1 -m cgroup --cgroup $CGCLASSID 2> /dev/null
  iptables -D POSTROUTING -t mangle -j CONNMARK --save-mark 2> /dev/null
  iptables -D PREROUTING -t mangle -j CONNMARK --restore-mark 2> /dev/null
  iptables -D INPUT -m connmark ! --mark $MARKID -j ACCEPT 2> /dev/null
  iptables -D INPUT -p tcp -m hashlimit --hashlimit-name hl1 --hashlimit-above $(cat /tmp/$config_file_name)/s -j DROP 2> /dev/null
  ip6tables -D OUTPUT 1 -m cgroup --cgroup $CGCLASSID 2> /dev/null
  ip6tables -D POSTROUTING -t mangle -j CONNMARK --save-mark 2> /dev/null
  ip6tables -D PREROUTING -t mangle -j CONNMARK --restore-mark 2> /dev/null
  ip6tables -D INPUT -m connmark ! --mark $MARKID -j ACCEPT 2> /dev/null
  ip6tables -D INPUT -p tcp -m hashlimit --hashlimit-name hl1 --hashlimit-above $(cat /tmp/$config_file_name)/s -j DROP 2> /dev/null
  rm /tmp/$config_file_name 2> /dev/null
}

init_ipt_rules() {
  iptables -I OUTPUT 1 -m cgroup --cgroup $CGCLASSID -j MARK --set-mark $MARKID
  if [ $? -ne 0 ]; then
    return 1
  fi
  iptables -A POSTROUTING -t mangle -j CONNMARK --save-mark
  if [ $? -ne 0 ]; then
    return 1
  fi
  iptables -A PREROUTING -t mangle -j CONNMARK --restore-mark
  if [ $? -ne 0 ]; then
    return 1
  fi
  iptables -A INPUT -m connmark ! --mark $MARKID -j ACCEPT
  if [ $? -ne 0 ]; then
    return 1
  fi
  iptables -A INPUT -p tcp -m hashlimit --hashlimit-name hl1 --hashlimit-above $D_LIMIT/s -j DROP
  if [ $? -ne 0 ]; then
    return 1
  fi
  ip6tables -I OUTPUT 1 -m cgroup --cgroup $CGCLASSID -j MARK --set-mark $MARKID
  if [ $? -ne 0 ]; then
    return 1
  fi
  ip6tables -A POSTROUTING -t mangle -j CONNMARK --save-mark
  if [ $? -ne 0 ]; then
    return 1
  fi
  ip6tables -A PREROUTING -t mangle -j CONNMARK --restore-mark
  if [ $? -ne 0 ]; then
    return 1
  fi
  ip6tables -A INPUT -m connmark ! --mark $MARKID -j ACCEPT
  if [ $? -ne 0 ]; then
    return 1
  fi
  ip6tables -A INPUT -p tcp -m hashlimit --hashlimit-name hl1 --hashlimit-above $D_LIMIT/s -j DROP
  if [ $? -ne 0 ]; then
    return 1
  fi
  # iptables -I OUTPUT 1 -m cgroup --cgroup $CGCLASSID -j MARK --set-mark $MARKID
  # iptables -A POSTROUTING -t mangle -j CONNMARK --save-mark
  # iptables -A PREROUTING -t mangle -j CONNMARK --restore-mark
  # iptables -A INPUT -m connmark ! --mark $MARKID -j ACCEPT
  # iptables -A INPUT -p tcp -m hashlimit --hashlimit-name hl1 --hashlimit-above $D_LIMIT/s -j DROP
  # ip6tables -I OUTPUT 1 -m cgroup --cgroup $CGCLASSID -j MARK --set-mark $MARKID
  # ip6tables -A POSTROUTING -t mangle -j CONNMARK --save-mark
  # ip6tables -A PREROUTING -t mangle -j CONNMARK --restore-mark
  # ip6tables -A INPUT -m connmark ! --mark $MARKID -j ACCEPT
  # ip6tables -A INPUT -p tcp -m hashlimit --hashlimit-name hl1 --hashlimit-above $D_LIMIT/s -j DROP
  echo $D_LIMIT > /tmp/$config_file_name
  return 0
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
    D_LIMIT=$(toBytes $OPTARG)
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

# if [[ "$D_FLAG" -eq "1" ]] && [[ "$(echo $D_LIMIT | tr -dc 'a-zA-Z')" != "kb" ]]; then
#   echo "Download limit (-d) unit should be in kilobyte (kb)"
#   exit 1
if [[ "$U_FLAG" -eq "1" ]] && [[ "$DELAY_FLAG" -eq "1" ]] && [[ "$D_FLAG" -eq "1" ]]; then
	limit_rate_latency_egress $DEV $U_LIMIT $DELAY_LIMIT
  init_ipt_rules
  if [ $? -ne 0 ]; then
    delete_tc
    delete_ipt_rules
    exit 1
  fi
  exit 0
else
  echo "You should provide the three options : -u, -d, -l"
  exit 1
fi
