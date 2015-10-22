MYPORT = 3000
MYGROUP_6 = 'ff02::1%eth0'

import time
import struct
import socket
import sys

def main():
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.bind(('', MYPORT))
    fh = open('wvwms_config.bin', 'rb')
    config = bytearray(fh.read());
    data = bytearray([0x7A, 0xA7, 0x1+len(config), 0x0D])
    data = data + config
    s.sendto(data, (MYGROUP_6, MYPORT))

if __name__ == '__main__':
    main()
