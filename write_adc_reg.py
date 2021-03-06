MYPORT = 3000
MYGROUP_6 = 'ff02::1%eth0'

import time
import struct
import socket
import sys

def main():
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.bind(('', MYPORT))
    # writes to config reg 0x2, reverse bit order needed!
    # reg addr, regl len, 3 bytes in reverse order
    data = bytearray([0x7A, 0xA7, 0x7, 0x11, 0x1, 0x3, 0x1F, 0x00, 0x40])
    s.sendto(data, (MYGROUP_6, MYPORT))

if __name__ == '__main__':
    main()
