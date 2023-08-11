#! /bin/bash

ip link add veth1 type veth peer name veth2
ifconfig veth2 192.168.201.1 netmask 255.255.255.0 up
ifconfig veth2 add 2001::1234/64
ifconfig veth1 up
ethtool --offload veth2 tx off
ethtool --offload veth1 tx off

service ntp start

# sysctl net.ipv4.ip_forward=1
iptables -F
iptables -t nat -F
iptables -t nat -A POSTROUTING -s 192.168.201.0/24 -o eth0 -j MASQUERADE
iptables -A FORWARD -d 192.168.201.0/24 -o veth2 -j ACCEPT
iptables -A FORWARD -s 192.168.201.0/24 -j ACCEPT

#Debug
#ip link
#ip addr
#iptables -t nat -L -n
