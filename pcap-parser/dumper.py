import subprocess
import os
import sys
import time

def runAndWait(name, target):
    filename = 's_' + name + '_' + str(time.time()).split('.')[0] + '.pcap'
    path = os.path.join(target, filename)
    subprocess.run(['tcpdump', '-G', '5', '-W', '1', '-w', path, '-i', 'wlan0'])
    os.rename(path, path.replace('s_', 'r_'))

name = sys.argv[1]
target = sys.argv[2]

while True:
    runAndWait(name, target)