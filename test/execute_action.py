import serial, sys, time
 
for dev in ['/dev/ttyACM', '/dev/ttyUSB']:
    for i in range(5):
        try:
            s = serial.Serial(dev + str(i), 9600, timeout=2)
            time.sleep(0.1)
            break
        except Exception as e:
            print str(e)

time.sleep(1)
while True:
    try:
        c = raw_input('Enter command: ')
        s.write(c)
        try:
            r = s.readline()
            if r is not '': print r
        except Exception as e:
            break
    except KeyboardInterrupt:
        break
    except Exception as e:
        print str(e)
