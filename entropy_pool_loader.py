# Loads entropy into the system pool
# cat /dev/ttyUSB0 > /dev/random does work but this script also increments the entropy pool value
# based on the example from https://unix.stackexchange.com/users/8250/lekensteyn
import fcntl, select, struct
with open('/dev/ttyUSB0', 'rb') as hw, open('/dev/random') as rnd:
    while True:
        d = hw.read(512)
        fcntl.ioctl(rnd, 0x40085203, struct.pack('ii', 4 * len(d), len(d)) + d)
        select.select([], [rnd], [])
