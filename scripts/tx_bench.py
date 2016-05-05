#!/usr/bin/env python
  
############################### tx_bench.py ###############################
""" 
Before to launch this test start HydraBus with latest HydraFW and open a console then type
debug test-rx
Then close the console and launch following example benchmark.

Examples
tx_bench.py 64 1000 >> bench.txt
tx_bench.py 128 500 >> bench.txt
tx_bench.py 256 500 >> bench.txt
tx_bench.py 500 500 >> bench.txt
tx_bench.py 512 500 >> bench.txt
tx_bench.py 1000 500 >> bench.txt
tx_bench.py 1008 500 >> bench.txt
tx_bench.py 1024 500 >> bench.txt
tx_bench.py 1280 400 >> bench.txt
tx_bench.py 2000 400 >> bench.txt
tx_bench.py 2048 400 >> bench.txt
tx_bench.py 3000 400 >> bench.txt

""" 

import serial;
import time;
import sys;

def main():
    try:
        serialPort = serial.Serial('COM3',\
                                   115200,
                                   serial.EIGHTBITS,\
                                   serial.PARITY_NONE,
                                   serial.STOPBITS_ONE);
    except:
        print("Couldn't open serial port");
        exit();
 
    size = int(sys.argv[1]);
    num_Packets = int(sys.argv[2]);

    txData=[];
    for i in range(size):
        txData.append('a');

    packet_Length = len(txData);
    text = txData;

    print('packet_Length: %d num_Packets: %d' % (packet_Length, num_Packets));

    byteSent = 0;
    # First time slice
    t1 = time.time()

    for i in range(num_Packets):
        byteSent += serialPort.write(text);

    # Second time slice
    t2 = time.time()
    throughPut = (num_Packets * packet_Length)/(t2-t1);

    time_s = t2-t1;
    throughPut_kb = throughPut/1024;
    kbyteSent = byteSent/1024;
    print("TX Time: %.5f s " % time_s);
    print('TX Bytes/s: %d TX KBytes/s: %.2f' % (throughPut, throughPut_kb));
    print;
"""
    print;
    print('Bytes Sent: %d ' % byteSent);
    print('KBytes Sent: %.2f ' % kbyteSent);
"""

if __name__ == '__main__':
    main()

#!/usr/bin/env python