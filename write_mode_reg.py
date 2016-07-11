MYPORT = 3000
MYGROUP_6 = 'ff02::1%eth0'

import time
import struct
import socket
import sys

def main():
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.bind(('', MYPORT))
    mode_reg = 0x400003
    # writes to mode reg 0x2, reverse bit order needed!
    # reg addr, regl len, 3 bytes in reverse order
    # Frequency - last 9 bytes
    # 0x01 - 1200Hz 0x03 - 400 Hz 0x0F - 80Hz, 0x3D - 20Hz, 0x77 - 10Hz, 0x19 - 48Hz, 0x4B - 16Hz 
    # OBSOLETE: data = bytearray([0x7A, 0xA7, 0x7, 0x11, 0x1, 0x3, 0x01, 0x00, 0x40])
    data = bytearray([0x7A, 0xA7, 0x7, 0x11, 0x1, 0x3])
    data += bytearray( struct.pack("I", mode_reg) )
    s.sendto(data, (MYGROUP_6, MYPORT))

if __name__ == '__main__':
    main()
