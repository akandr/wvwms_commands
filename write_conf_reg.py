MYPORT = 3000
MYGROUP_6 = 'ff02::1%eth0'

import time
import struct
import socket
import sys

def main():
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.bind(('', MYPORT))

    # byte 23 chop
    # byte 22 acx
    # bytes 21-16 0s
    # bytes 15-08 Channel select (we need 1)
    # byte 7 burn -burnout current (we need 0)
    # byte 6 reference detect (0)
    # byte 5 - keep 0
    # byte 4 - input buffer, keep 1
    # byte 3 - 0 - unipolar, 1 - bipolar (1)
    # bytes 2-0 gain (000 - 5V, 111 39 mV)
    conf_reg = 0xC00117

    # data = bytearray([0x7A, 0xA7, 0x7, 0x11, 0x2, 0x3, 0x17, 0x01, 0x80])
    data = bytearray([0x7A, 0xA7, 0x7, 0x11, 0x2, 0x3])
    data += bytearray(struct.pack("I", conf_reg))
    s.sendto(data, (MYGROUP_6, MYPORT))

if __name__ == '__main__':
    main()
