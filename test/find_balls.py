# Import the necessary packages
from collections import deque
import numpy as np
import cv2, cv
import glob
import sys
import os

RADIUS_MIN = 20
RADIUS_MAX = 160

# Define the lower and upper boundaries of the "green"
# ball in the HSV color space, then initialize the
# list of tracked points
greenLower = (29, 64, 32)
greenUpper = (90, 255, 255)
orangeLower = (5, 64, 32)
orangeUpper = (40, 255, 255)

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
    # Green Circles
    green_circles = cv2.HoughCircles(green_mask, cv.CV_HOUGH_GRADIENT, 1.2, 100)
    if green_circles is not None:
        green_circles = np.round(green_circles[0, :]).astype("int") # convert the (x, y) coordinates and radius of the circles to integers
        for (x, y, r) in green_circles: # loop over the (x, y) coordinates and radius of the circles
            cv2.circle(bgr, (x, y), r, (0, 255, 0), 4)
        cv2.imshow("output", bgr)
        cv2.waitKey(0)
    # Orange Circles
    orange_circles = cv2.HoughCircles(orange_mask, cv.CV_HOUGH_GRADIENT, 1.2, 100)
    if orange_circles is not None:
        orange_circles = np.round(orange_circles[0, :]).astype("int") # convert the (x, y) coordinates and radius of the circles to integers
        for (x, y, r) in orange_circles: # loop over the (x, y) coordinates and radius of the circles
            cv2.circle(bgr, (x, y), r, (0, 255, 0), 4)
        cv2.imshow("output", bgr)
        cv2.waitKey(0)

    ## Contours
    # Green Contours
    green_contours = cv2.findContours(green_mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)[-2]
    center = None # initialize the current (x, y) center of the ball
    if len(green_contours) > 0: # only proceed if at least one contour was found
        # Find the largest contour in the mask, then use
        # it to compute the minimum enclosing circle and centroid
        for c in green_contours:
            k = cv2.isContourConvex(c)
            approx = cv2.approxPolyDP(c,0.01*cv2.arcLength(c,True),True)
            area = cv2.contourArea(c)
            if ((len(approx) > 8) & (area > 30) ):
                ((x, y), radius) = cv2.minEnclosingCircle(c)
                M = cv2.moments(c)
                center = (int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"]))
                if (radius > RADIUS_MIN) and (radius < RADIUS_MAX): # only proceed if the radius meets a minimum size
                    cv2.circle(bgr, (int(x), int(y)), int(radius), (0, 255, 0), 2)
                    cv2.circle(bgr, center, 5, (0, 0, 255), -1)
    # Orange Contours
    orange_contours = cv2.findContours(orange_mask.copy(), cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)[-2]
    center = None # initialize the current (x, y) center of the ball
    if len(orange_contours) > 0: # only proceed if at least one contour was found
        # Find the largest contour in the mask, then use
        # it to compute the minimum enclosing circle and centroid
        for c in orange_contours:
            k = cv2.isContourConvex(c)
            print k
            ((x, y), radius) = cv2.minEnclosingCircle(c)
            M = cv2.moments(c)
            center = (int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"]))        
            if (radius > RADIUS_MIN) and (radius < RADIUS_MAX): # only proceed if the radius meets a minimum size
                cv2.circle(bgr, (int(x), int(y)), int(radius), (0, 255, 255), 2)
                cv2.circle(bgr, center, 5, (0, 0, 255), -1)
    blank = np.zeros((480,640), np.uint8)
    orange = np.dstack((blank, orange_mask, orange_mask))
    green = np.dstack((blank, green_mask, blank))
    cv2.imshow("bgr", np.hstack((bgr, orange + green)))
    key = cv2.waitKey(0)
 
cv2.destroyAllWindows()
