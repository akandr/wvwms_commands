MYPORT = 3000
MYGROUP_6 = 'ff02::1%eth0'

import time
import struct
import socket
import sys

def main():
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.bind(('', MYPORT))
    data = bytearray([0x7A, 0xA7, 0x2, 0x18, 0x1])
    s.sendto(data, (MYGROUP_6, MYPORT))

if __name__ == '__main__':
    main()
