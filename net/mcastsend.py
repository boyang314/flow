#!/usr/bin/python

from sys import argv
if len(argv) != 4:
    print argv[0], '<if_ip> <mcast_group> <mcast_port>'
    exit(1)

if_ip = argv[1]
mcast_group = argv[2]
mcast_port = int(argv[3])

from socket import *
sock = socket(AF_INET, SOCK_DGRAM)
sock.setsockopt(IPPROTO_IP, IP_MULTICAST_IF, inet_aton(if_ip))
sock.setsockopt(IPPROTO_IP, IP_MULTICAST_TTL, 2)

from time import sleep
while 1:
  sleep(1)
  sock.sendto("hello world!", (mcast_group, mcast_port))

