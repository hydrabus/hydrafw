#!/usr/bin/env python3
  
############################### tx_bench.py ###############################
""" 
Before to launch this test just reset HydraBus board(to be sure it is in a clean state)

Examples Windows with HydraBus on COM4 (for Linux use /dev/ttyACM0 ...)
python tx_bench.py COM4 1 10000 >> bench_1B.txt
python tx_bench.py COM4 4 10000 >> bench_4B.txt
python tx_bench.py COM4 8 10000 >> bench_8B.txt
python tx_bench.py COM4 16 10000 >> bench_16B.txt
python tx_bench.py COM4 64 5000 >> bench_64B.txt
python tx_bench.py COM4 128 2000 >> bench_128B.txt
python tx_bench.py COM4 256 2000 >> bench_256B.txt
python tx_bench.py COM4 500 2000 >> bench_500B.txt
python tx_bench.py COM4 512 2000 >> bench_512B.txt
python tx_bench.py COM4 1000 1000 >> bench_1000B.txt
python tx_bench.py COM4 1008 1000 >> bench_1008B.txt
python tx_bench.py COM4 1024 1000 >> bench_1024B.txt
python tx_bench.py COM4 1280 800 >> bench_1280B.txt
python tx_bench.py COM4 2000 500 >> bench_2000B.txt
python tx_bench.py COM4 2048 500 >> bench_2048B.txt
python tx_bench.py COM4 3000 400 >> bench_3000B.txt
python tx_bench.py COM4 4096 300 >> bench_4096B.txt

""" 

import serial
import time
import sys

def main():
    
    port = sys.argv[1]
    try:
        serialPort = serial.Serial(port, 115200, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE)
    except:
        print("Couldn't open serial port %s" % port)
        exit()

    size = int(sys.argv[2])
    num_Packets = int(sys.argv[3])

    # Enter special debug test-rx mode
    txData = "debug test-rx\n"
    text = txData.encode('utf-8')
    serialPort.write(text)
    # Wait 0.1s to be sure Hydrabus is entered in debug test-rx mode
    time.sleep(0.1)
    serialPort.flushInput()

    txData = ""
    for i in range(size):
        txData += 'a'

    packet_Length = len(txData)
    text = txData.encode('utf-8')

    print('COM port: %s packet_Length: %d num_Packets: %d' % (port, packet_Length, num_Packets))

    byteSent = 0
    # First time slice
    t1 = time.time()

    for i in range(num_Packets):
        byteSent += serialPort.write(text)

    # Second time slice
    t2 = time.time()
    throughPut = (num_Packets * packet_Length)/(t2-t1)

    time_s = t2-t1
    throughPut_kb = throughPut/1024
    kbyteSent = byteSent/1024
    print("TX Time: %.5f s " % time_s)
    print('TX Bytes/s: %d TX KBytes/s: %.2f' % (throughPut, throughPut_kb))

if __name__ == '__main__':
    main()
