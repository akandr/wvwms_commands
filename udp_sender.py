MYPORT = 3000
MYGROUP_6 = 'ff02::1%eth0'

import time
import struct
import socket
import sys

def main():

    addrinfo = socket.getaddrinfo(MYGROUP_6, None)[0]
    print(addrinfo)
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
#    s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_JOIN_GROUP, MYGROUP_6)

    s.bind(('', MYPORT))
    while True:
        data = bytearray([0x7A, 0xA7, 0x2, 0x17, 0x1])
        s.sendto(data, (MYGROUP_6, MYPORT))
	time.sleep(10)

if __name__ == '__main__':
    main()
