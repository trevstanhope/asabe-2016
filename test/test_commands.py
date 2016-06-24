import serial, sys
dev = sys.argv[1]
s = serial.Serial(dev, 9600)
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
