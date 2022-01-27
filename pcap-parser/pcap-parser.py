import subprocess
import os
import sys
import time

def runAndWait(inputFile1, outputFile):
    subprocess.run(['./pcap-parser', inputFile1, outputFile])

def processFile(inputFile, source, target):
    print(inputFile)
    print(source)
    print(target)

    srcFile = os.path.join(source, inputFile)

    print(srcFile)

    jsonFile = os.path.join(target, inputFile.replace('r_', '').replace('.pcap', '.json'))

    print(jsonFile)

    runAndWait(srcFile, jsonFile)

    print("JSON processing...")

    renamed = srcFile.replace('r_', 'p_')
    os.rename(srcFile, renamed)

    print("Src file is renamed.")

source = sys.argv[1]
target = sys.argv[2]

while True:
    files = os.listdir(source)
    for f in files:
        if f.startswith("r_"):
            processFile(f, source, target)

    time.sleep(60)