# ASABE 2016

    ssh pi@192.168.0.xxx
    
    cd asabe-2016/sketches/Picker_ECU
    ./compile
    python ../../test/execute_action.py

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
