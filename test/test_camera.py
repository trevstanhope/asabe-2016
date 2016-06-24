import cv2, cv
import numpy as np
import sys

FRAME_WIDTH = 320
FRAME_HEIGHT = 240
SATURATION = 0.5
BRIGHTNESS = 0.5
CONTRAST = 0.5
NUM_FLUSH = 30

cam = cv2.VideoCapture(0)
cam.set(cv.CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH)
cam.set(cv.CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)
cam.set(cv.CV_CAP_PROP_SATURATION, SATURATION)
cam.set(cv.CV_CAP_PROP_BRIGHTNESS, BRIGHTNESS)
cam.set(cv.CV_CAP_PROP_CONTRAST, CONTRAST)

for i in range(NUM_FLUSH):
    (s, bgr) = cam.read()
if s:
    cv2.imshow('', bgr)
    cv2.imwrite(sys.argv[1], bgr)
    cv2.waitKey(0)

