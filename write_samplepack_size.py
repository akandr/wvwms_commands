MYPORT = 3000
MYGROUP_6 = 'ff02::1%eth0'

import time
import struct
import socket
import sys

def main():
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.bind(('', MYPORT))
    samplepack_size = 3
    data = bytearray([0x7A, 0xA7, 0x6, 0x30])
    data+= bytearray( struct.pack("I", samplepack_size) )
    s.sendto(data, (MYGROUP_6, MYPORT))

if __name__ == '__main__':
    main()
