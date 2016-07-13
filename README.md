# ASABE 2016

## Commiting changes on the server
After making some changes in the code folder ~/Public/git/asabe-2016
They must be committed and pushed to the remote code repository on github. 
To do so, open a terminal from the asabe-2016 folder and use the git tool:

    git commit -a -m "some message about the change"
    git push

NOTE: This will prompt you for your username and password for your github account.

## Connecting to the robot
Open the browser and navigate to 192.168.0.1
username and password is 'admin' and 'admin', respectively.
Open the DHCP tab and check the clients list to see the IP address of the robot.
Next, open a terminal and run:

    ssh pi@192.168.0.xxx

This will prompt for a password, which is 'raspberry'. If successful, you have now logged in
to the robot via a terminal shell. The terminal will now show you the filesystem of the Pi.
To pull the latest changes and upload them to the robot, run the following if using the Picker:

    cd ~/asabe-2016/sketches/Pickup_ECU && ./compile

If connect to the Delivery bot, instead upload the Delivery_ECU code:

    cd ~/asabe-2016/sketches/Delivery_ECU && ./compile

To test the commands, you can use the execute_action.py tool. This simply allows you to send 
character commands over serial to the Arduino and print out the response:
 
    python ~/asabe-2016/test/execute_action.py
	
Now you can type in some single letter command (J), or command with a
numeric value following it (F1000), and the robot will perform the
action returning a result string.

## Arduino Commands
### Picker Robot
1. Follow line to end
2. Ball approach (blind forward)
3. Center ball
4. Capture by color
5. Search for the next ball
6. Blind reverse + do 2 to 5
7. After hitting the last ball --> blind turn right 90 deg
8. Forward till line (T)
9. Align to the line right
10. 90 deg rotation
11. reverse till contact
12. transfer balls

### Delivery Robot
1. Follow line till intersection
2. 90 deg rotation right (move forward + turn until see line)
3. Follow line till intersection
4. 90 deg rotation left
5. Follow line until T then receive balls
6. Reverse follow line until the 3rd T
7. Blind reverse
8. Deploy gate and deliver balls
