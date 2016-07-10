# Import the necessary packages
from collections import deque
import numpy as np
import cv2, cv
import glob
import sys
import os

# Define the lower and upper boundaries of the "green"
# ball in the HSV color space, then initialize the
# list of tracked points
greenLower = (29, 86, 6)
greenUpper = (90, 255, 255)
orangeLower = (10, 86, 6)
orangeUpper = (25, 255, 255)

fnames = glob.glob(os.path.join(sys.argv[1], "*.jpg"))
for f in fnames:

    # grab an image
    bgr = cv2.imread(f)
    blurred = cv2.GaussianBlur(bgr, (11, 11), 0)
    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)
    green_mask = cv2.inRange(hsv, greenLower, greenUpper)
    green_mask = cv2.erode(green_mask, None, iterations=2)
    green_mask = cv2.dilate(green_mask, None, iterations=2)
    orange_mask = cv2.inRange(hsv, orangeLower, orangeUpper)
    orange_mask = cv2.erode(orange_mask, None, iterations=2)
    orange_mask = cv2.dilate(orange_mask, None, iterations=2)

    ## Hough Circles
    green_circles = cv2.HoughCircles(green_mask, cv.CV_HOUGH_GRADIENT, 1.2, 100)
    orange_circles = cv2.HoughCircles(orange_mask, cv.CV_HOUGH_GRADIENT, 1.2, 100)

    # Green Circles
    if green_circles is not None:
        # convert the (x, y) coordinates and radius of the circles to integers
        green_circles = np.round(green_circles[0, :]).astype("int")
        # loop over the (x, y) coordinates and radius of the circles
        for (x, y, r) in green_circles:
            # draw the circle in the output image, then draw a rectangle
            # corresponding to the center of the circle
            cv2.circle(bgr, (x, y), r, (0, 255, 0), 4)
        # show the output image
        cv2.imshow("output", bgr)
        cv2.waitKey(0)

    ## Contours
    # Find contours in the mask and 
    green_contours = cv2.findContours(green_mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)[-2]
    center = None # initialize the current (x, y) center of the ball
    if len(green_contours) > 0: # only proceed if at least one contour was found
        # Find the largest contour in the mask, then use
        # it to compute the minimum enclosing circle and centroid
        c = max(green_contours, key=cv2.contourArea)
        ((x, y), radius) = cv2.minEnclosingCircle(c)
        M = cv2.moments(c)
        center = (int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"]))
 
        # only proceed if the radius meets a minimum size
        if radius > 10:
            # draw the circle and centroid on the bgr,
            # then update the list of tracked points
            cv2.circle(bgr, (int(x), int(y)), int(radius),
                (0, 255, 255), 2)
            cv2.circle(bgr, center, 5, (0, 0, 255), -1)
    
    # show the bgr to our screen
    cv2.imshow("bgr", bgr)
    key = cv2.waitKey(0)
 
cv2.destroyAllWindows()
