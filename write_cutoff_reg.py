MYPORT = 3000
MYGROUP_6 = 'ff02::1%eth0'

import time
import struct
import socket
import sys

def main():
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.bind(('', MYPORT))
    # cutoff, 4 bytes in reverse order
    data = bytearray([0x7A, 0xA7, 0x6, 0x23, 0x80, 0x2C, 0x80, 0x00])
    s.sendto(data, (MYGROUP_6, MYPORT))

if __name__ == '__main__':
    main()
