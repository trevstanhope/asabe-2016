import cv2, cv
import numpy as np
import sys

FRAME_WIDTH = 320
FRAME_HEIGHT = 240
SATURATION = 1.0
BRIGHTNESS = 0.5
CONTRAST = 0.5
NUM_FLUSH = 60
TALL_THRESHOLD = 220 # pixels
try:
    FILENAME = sys.argv[1]
    bgr=cv2.imread(FILENAME)
except:
    cam = cv2.VideoCapture(0)
    cam.set(cv.CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH)
    cam.set(cv.CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)
    cam.set(cv.CV_CAP_PROP_SATURATION, SATURATION)
    cam.set(cv.CV_CAP_PROP_BRIGHTNESS, BRIGHTNESS)
    cam.set(cv.CV_CAP_PROP_CONTRAST, CONTRAST)
    
    for i in range(NUM_FLUSH):
        (s, bgr) = cam.read()
    if not s:
        exit(1)        

# Blur image
bgr=cv2.medianBlur(bgr,5)
detected_areas = [(0,0,0,0,0)] * 3

## Green
"""
Green detection needs a high-pass to eliminate low saturation detection which is both light and dark
due to the HSV transform of cv2
"""
greenlow = np.array([30,30,0]) 
greenhigh = np.array([100,255,255])
hsv = cv2.cvtColor(bgr,cv2.COLOR_BGR2HSV)
mask = cv2.inRange(hsv,greenlow,greenhigh)
cv2.imshow('',mask)
cv2.waitKey(0)
kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(3,3))
kernel2 = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(2,2))
opening = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
closing = cv2.morphologyEx(opening, cv2.MORPH_CLOSE, kernel)
ret,thresh = cv2.threshold(closing,127,255,0)
contours, hierarchy = cv2.findContours(thresh,cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
areas = [cv2.contourArea(c) for c in contours]
max_index = np.argmax(areas) # Find the index of the largest contour
cnt = contours[max_index]
x,y,w,h = cv2.boundingRect(cnt)
detected_areas[0] = (x, y, w, h)

## Yellow
"""
Needs high pass to remove low saturation yellow-white
"""
yellowlow = np.array([15,128,128])
yellowhigh = np.array([40, 255, 255])
hsv = cv2.cvtColor(bgr,cv2.COLOR_BGR2HSV)
mask = cv2.inRange(hsv,yellowlow,yellowhigh)
cv2.imshow('',mask)
cv2.waitKey(0)
kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(3,3))
kernel2 = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(2,2))
opening = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
closing = cv2.morphologyEx(opening, cv2.MORPH_CLOSE, kernel)
ret,thresh = cv2.threshold(closing,127,255,0)
contours, hierarchy = cv2.findContours(thresh,cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
areas = [cv2.contourArea(c) for c in contours] 
max_index = np.argmax(areas) # Find the index of the largest contour
cnt = contours[max_index]
x,y,w,h = cv2.boundingRect(cnt)
detected_areas[1] = (x, y, w, h)

## Brown
brownlow = np.array([0,0,0])
brownhigh = np.array([25,255,150])
brownlow1 = np.array([140,0,0])
brownhigh1 = np.array([180,255,150])
hsv = cv2.cvtColor(bgr,cv2.COLOR_BGR2HSV)
mask1 = cv2.inRange(hsv,brownlow,brownhigh)
mask2 = cv2.inRange(hsv,brownlow1,brownhigh1)
mask3 = mask1 + mask2
cv2.imshow('', mask3)
cv2.waitKey(0)
kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(3,3))
kernel2 = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(2,2))
opening = cv2.morphologyEx(mask3, cv2.MORPH_OPEN, kernel)
closing = cv2.morphologyEx(opening, cv2.MORPH_CLOSE, kernel)
ret,thresh = cv2.threshold(closing,127,255,0)
contours, hierarchy = cv2.findContours(thresh,cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
areas = [cv2.contourArea(c) for c in contours] # Find the index of the largest contour
max_index = np.argmax(areas)
cnt = contours[max_index]
x,y,w,h = cv2.boundingRect(cnt)
detected_areas[2] = (x, y, w, h)

## Check Largest
areas = [w*h for (x, y, w, h) in detected_areas]
print areas
max_area = max(areas)
i = np.argmax(areas)
(x, y, w, h) = detected_areas[i]
if i == 0:
    c = (0,255,0)
    color = 'green'
elif i == 1:
    c = (0,255,255)
    color = 'yellow'
elif i == 2:
    c = (0,87,115)
    color = 'brown'
else:
    exit(1)
if h > TALL_THRESHOLD:
    height = 'tall'
else:
    height = 'short'
print height, color
cv2.rectangle(bgr,(x,y),(x+w,y+h), c, 2) # Draw the rectangle
cv2.imshow("Show",bgr)
cv2.waitKey()
cv2.destroyAllWindows()

