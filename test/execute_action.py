import serial, sys
dev = sys.argv[1]
for 
for dev in ['/dev/ttyACM0', '/dev/ttyUSB0']:
    for i in range(attempts):
        try:
            s = Serial(dev + str(i), 9600, timeout=2)
            time.sleep(wait)
            break
        except Exception as e:
            self.pretty_print('CTRL', 'Error: %s' % str(e))

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
