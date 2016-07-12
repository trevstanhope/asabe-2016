import serial, sys
 
for dev in ['/dev/ttyACM0', '/dev/ttyUSB0']:
    for i in range(5):
        try:
            s = serial.Serial(dev + str(i), 9600, timeout=2)
            time.sleep(wait)
            break
        except Exception as e:
            print str(e)

while True:
    try:
        c = raw_input('Enter command: ')
        s.write(c)
	while True:
	    try:
	    	r = s.readline()
		print r
	    except:
		break
    except KeyboardInterrupt:
        break
