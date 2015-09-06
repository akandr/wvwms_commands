MYPORT = 3000
MYGROUP_6 = 'ff02::1%eth0'

import time
import struct
import socket
import sys
import binascii

def main():
    file_size = 0
    f = open("wvwms_config.bin", "rb")
    try:
        byte = f.read(1)
        file_content = byte
        while byte != "":
            byte = f.read(1)
            if byte != "":
                file_content += byte
            file_size+=1
        print "Config file size: "
        print file_size
        print(binascii.hexlify(file_content))
        s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        s.bind(('', MYPORT))
        data = bytearray([0x7A, 0xA7, 0x1+file_size , 0x0D]) + file_content
        s.sendto(data, (MYGROUP_6, MYPORT))

    finally:
        f.close()
if __name__ == '__main__':
    main()
