#!/usr/bin/env python
"""
Server for 
McGill University
ASABE 2016
"""

__author__ = 'Trevor Stanhope'
__version__ = '0.1'

# Libraries
import json
import ast
import cherrypy
import os
import sys
import numpy as np
from datetime import datetime, timedelta
from cherrypy.process.plugins import Monitor
from cherrypy import tools
from bson import json_util
import zmq
import cv2, cv
import pygtk
pygtk.require('2.0')
import gtk
import matplotlib.pyplot as mpl
import time
from random import randint

# Configuration
try:
    CONFIG_PATH = sys.argv[1]
except Exception as err:
    print "NO CONFIGURATION FILE GIVEN"
    exit(1)

# CherryPy3 server
class Server:
    
    ## Initialize
    def __init__(self, config_path):
        
        # Configuration
        self.load_config(config_path)
        
        # Initializers
        self.__init_zmq__()
        self.__init_tasks__()
        self.__init_statemachine__()
        self.__init_gui__()

    ## Useful Functions
    def pretty_print(self, task, msg):
        """ Pretty Print """ 
        date = datetime.strftime(datetime.now(), '%d/%b/%Y:%H:%M:%S')
        print('[%s] %s\t%s' % (date, task, msg))
    def load_config(self, config_path):
        """ Load Configuration """
        self.pretty_print('CONFIG', 'Loading Config File')
        with open(config_path) as config:
            settings = json.loads(config.read())
            for key in settings:
                try:
                    getattr(self, key)
                except AttributeError as error:
                    setattr(self, key, settings[key])
       
    ## ZMQ Functions
    def __init_zmq__(self):      
        if self.VERBOSE: self.pretty_print('ZMQ', 'Initializing ZMQ')
        try:
            self.context = zmq.Context()
            self.socket = self.context.socket(zmq.REP)
            self.socket.bind(self.ZMQ_HOST)
        except Exception as error:
            self.pretty_print('ZMQ', str(error))
    def receive_request(self):
        if self.VERBOSE: self.pretty_print('ZMQ', 'Receiving request')
        try:
            packet = self.socket.recv()
            request = json.loads(packet)
            return request
        except Exception as error:
            self.pretty_print('ZMQ', 'Error: %s' % str(error))
    def send_response(self, action):
        """ Send Response """
        if self.VERBOSE: self.pretty_print('ZMQ', 'Sending Response to Robot')
        try:
            response = {
                'type' : 'response',
                'action' : action
                }
            dump = json.dumps(response)
            self.socket.send(dump)
            self.pretty_print('ZMQ', 'Response: %s' % str(response))
            return response
        except Exception as error:
            self.pretty_print('ZMQ', str(error))   
    
    ## Statemachine Functions
    def __init_statemachine__(self):
        self.bgr = cv2.imread(self.GUI_CAMERA_IMAGE)
        self.running = False
        self.row_num = 0
        self.plant_num = 0
        self.pass_num = 0
        self.samples_num = 0
        self.at_plant = 0
        self.at_end = 0
        self.start_time = time.time()
        self.end_time = self.start_time + self.RUN_TIME
        self.clock = self.end_time - self.start_time
        self.observed_plants = [] #TODO can add dummy vals here
        self.collected_plants = {
            'green' : {
                'short' : False,
                'tall' : False                
            },
            'brown' : {
                'short' : False,
                'tall' : False                
            },
            'yellow' : {
                'short' : False,
                'tall' : False
            }
        }
    def decide_action(self, request):
        """
        Below is the Pseudocode for how the decisions are made:

        start()
        for row in range(4):
            align()
            while not at_end:
                (at_end, at_plant, img) = seek
                if at_plant: 
                    (color, height, is_new) = identify_plant()
                    if is_new: 
                        grab()
            turn()
            align()
            while not at_end:
                (at_end, at_plant, img) = seek
                if at_plant: 
                    (color, height, is_new) = identify_plant()
                    if is_new: 
                        grab()
            jump()
        """
        self.pretty_print("DECIDE", "Last Action: %s" % request['last_action'])
        self.pretty_print("DECIDE", "Robot: %s" % request['robot'])
        ## If paused
        if self.running == False:
            if request['last_action'] == 'clear':
                action = 'wait'
            else:
                action = 'clear'
        ## If clock running out
        elif self.clock <= self.GIVE_UP_TIME: # if too little time
            self.pretty_print("DECIDE", "Time almost up! Proceeding to end!")
            if (request['last_action'] == 'finish') or (request['last_action'] == 'wait'):
                action = 'wait'
            else:
                action = 'end'
        return action

    def identify_plant(self, bgr):
        """
        Returns:
            color : green, yellow, brown
            height: short, tall
            is_new : true/false
        """
        if self.VERBOSE: self.pretty_print("CV2", "Identifying plant phenotype ...")
        try:
            # Blur image
            bgr = cv2.medianBlur(bgr, 5)
            thin_kernel = cv2.getStructuringElement(cv2.MORPH_RECT,(8,8))
            fat_kernel = cv2.getStructuringElement(cv2.MORPH_RECT,(10,10))
            brown_kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE,(10,10))
            hsv = cv2.cvtColor(bgr, cv2.COLOR_BGR2HSV)
            detected_areas = [ (0,0,0,0) ] * 3
            
            ## Green
            try:
                green_low = np.array([25, 20, 5]) 
                green_high = np.array([75, 255, 128])
                green_mask = cv2.inRange(hsv, green_low, green_high)
                green_invmask = cv2.bitwise_not(green_mask)
                green_closed = cv2.morphologyEx(green_invmask, cv2.MORPH_CLOSE, thin_kernel)
                green_eroded = cv2.erode(green_closed, fat_kernel)
                green_output = cv2.bitwise_not(green_eroded)
                ret,thresh = cv2.threshold(green_output, 127, 255, 0)
                contours, hierarchy = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
                areas = [cv2.contourArea(c) for c in contours]
                max_index = np.argmax(areas) # Find the index of the largest contour
                cnt = contours[max_index]
                x,y,w,h = cv2.boundingRect(cnt)
                detected_areas[0] = (x, y, w, h)
            except Exception as e:
                self.pretty_print('CV', 'Error: %s' % str(e))
            
            ## Yellow
            try:
                yellow_low = np.array([15, 128, 115])
                yellow_high = np.array([35, 255, 255])
                yellow_mask = cv2.inRange(hsv, yellow_low, yellow_high)
                yellow_invmask = cv2.bitwise_not(yellow_mask)
                yellow_closed = cv2.morphologyEx(yellow_invmask, cv2.MORPH_CLOSE, thin_kernel)
                yellow_eroded = cv2.erode(yellow_closed, fat_kernel)
                yellow_output = cv2.bitwise_not(yellow_eroded)
                ret,thresh = cv2.threshold(yellow_output, 127, 255, 0)
                contours, hierarchy = cv2.findContours(thresh, cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
                areas = [cv2.contourArea(c) for c in contours] 
                max_index = np.argmax(areas) # Find the index of the largest contour
                cnt = contours[max_index]
                x,y,w,h = cv2.boundingRect(cnt)
                detected_areas[1] = (x, y, w, h)
            except Exception as e:
                self.pretty_print('CV', 'Error: %s' % str(e))
                
            ## Brown
            try:
                brown_low_1 = np.array([0,0,0])
                brown_high_1 = np.array([25,255,150])
                brown_low_2 = np.array([160,0,0])
                brown_high_2 = np.array([180,255,150])
                brown_grey = cv2.cvtColor(bgr,cv2.COLOR_BGR2GRAY)
                brown_mask_1 = cv2.inRange(hsv, brown_low_1, brown_high_1)
                brown_mask_2 = cv2.inRange(hsv, brown_low_2, brown_high_2)
                brown_mask = brown_mask_1 + brown_mask_2
                brown_closed = cv2.morphologyEx(brown_mask, cv2.MORPH_CLOSE, brown_kernel)
                brown_cut = cv2.bitwise_and(brown_grey, brown_closed)
                brown_cut[brown_cut == 0] = 255
                brown_thresh = cv2.threshold(brown_cut, 80, 255, cv2.THRESH_BINARY)[1]
                brown_invmask = cv2.bitwise_not(brown_thresh)
                brown_eroded = cv2.erode(brown_invmask, brown_kernel)
                ret, thresh = cv2.threshold(brown_eroded, 127, 255, 0)
                contours, hierarchy = cv2.findContours(thresh, cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
                areas = [cv2.contourArea(c) for c in contours] # Find the index of the largest contour
                max_index = np.argmax(areas)
                cnt = contours[max_index]
                x,y,w,h = cv2.boundingRect(cnt)
                detected_areas[2] = (x, y, w, h)
            except Exception as e:
                self.pretty_print('CV', 'Error: %s' % str(e))
    
            # Find most likely
            areas = [w*h for (x, y, w, h) in detected_areas]
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
            if h > self.CAMERA_TALL_THRESHOLD:
                height = 'tall'
            else:
                height = 'short'
            # bgr = np.dstack((brown_eroded, brown_eroded, brown_eroded)) # remove
            cv2.rectangle(bgr,(x,y),(x+w,y+h), c, 2) # Draw the rectangle
        except Exception as e:
            self.pretty_print("CV", "ERROR: %s" % str(e))
            self.pretty_print("CV", "RANDOMLY ESTIMATING ...")
            colors = ['green', 'yellow', 'brown']
            heights = ['tall', 'short']
            i = randint(0,2)
            j = randint(0,1)
            color = color[i]
            height = heights[j]
        return color, height, bgr
    def add_plant(self, row, plant, color, height):
        for p in self.observed_plants:
            if p == (row, plant, color, height):
                return False
        self.observed_plants.append((row, plant, color, height))
        return True
 
    ## CherryPy Functions
    def __init_tasks__(self):
        if self.VERBOSE: self.pretty_print('CHERRYPY', 'Initializing Monitors')
        try:
            Monitor(cherrypy.engine, self.listen, frequency=self.CHERRYPY_LISTEN_INTERVAL).subscribe()
            Monitor(cherrypy.engine, self.refresh, frequency=self.CHERRYPY_REFRESH_INTERVAL).subscribe()
        except Exception as error:
            self.pretty_print('CHERRYPY', str(error))
    def listen(self):
        """ Listen for Next Sample """
        if self.VERBOSE: self.pretty_print('CHERRYPY', 'Listening for nodes ...')
        req = self.receive_request()
        self.bgr = np.array(req['bgr'], np.uint8)
        action = self.decide_action(req)
        resp = self.send_response(action)
        if self.MONGO_ENABLED: event_id = self.store_event(req, resp)
    def refresh(self):
        """ Update the GUI """
        if self.VERBOSE: self.pretty_print('CHERRYPY', 'Updating GUI ...')
        robot_position = (self.row_num, self.at_plant, self.pass_num, self.at_end)
        self.gui.draw_camera(self.bgr)
        self.gui.draw_board(robot_position)
        if self.running:
            self.clock = self.end_time - time.time()
            if self.clock <= 0:
                self.running = False
        else:
            self.end_time = time.time() + self.clock         
        self.gui.update_gui(self.clock)
    @cherrypy.expose
    def index(self):
        """ Render index page """
        html = open('static/index.html').read()
        return html
    @cherrypy.expose
    def default(self, *args, **kwargs):
        """
        Handle Posts -
        This function is basically the RESTful API
        """
        try:

            pass
        except Exception as err:
            self.pretty_print('ERROR', str(err))
        return None

    ## GUI Functions
    def __init_gui__(self):
        if self.VERBOSE: self.pretty_print('GUI', 'Initializing GUI')
        try:
            self.gui = GUI(self)
            self.running= False
        except Exception as error:
            self.pretty_print('GUI', str(error))
    def run(self, object):
        self.pretty_print("GUI", "Running session ...")
        self.running = True
    def stop(self, object):
        self.pretty_print("GUI", "Halting session ...")
        self.running = False
    def reset(self, object):
        self.pretty_print("GUI", "Resetting to start ...")
        self.__init_statemachine__() # reset values
    def close(self, widget, window):
        try:
            gtk.main_quit()
        except Exception as e:
            self.pretty_print('GUI', 'Console server is still up (CTRL-C to exit)')
# Display
class GUI(object):

    ## Initialize Display
    def __init__(self, object):
        """
        Requires super-object to have several handler functions:
            - shutdown()
            - close()
            - reset()
            - run()
        """
        try:
            # CONSTANTS
            self.GUI_LABEL_PASS = object.GUI_LABEL_PASS
            self.GUI_LABEL_PLANTS = object.GUI_LABEL_PLANTS
            self.GUI_LABEL_ROW = object.GUI_LABEL_ROW
            self.GUI_LABEL_SAMPLES = object.GUI_LABEL_SAMPLES
            self.GUI_LABEL_CLOCK = object.GUI_LABEL_CLOCK
            self.GUI_BOARD_IMAGE = object.GUI_BOARD_IMAGE
            # Window
            self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
            self.window.set_size_request(object.GUI_WINDOW_X, object.GUI_WINDOW_Y)
            self.window.connect("delete_event", object.close)
            self.window.set_border_width(10)
            self.window.show()
            self.hbox = gtk.HBox(False, 0)
            self.window.add(self.hbox)
            # Buttons
            self.hbox2 = gtk.HBox(False, 0)
            self.vbox = gtk.VBox(False, 0)
            self.vbox2 = gtk.VBox(False, 0)
            self.hbox.add(self.vbox)
            self.hbox2.show()
            self.vbox2.show()
            self.hbox3 = gtk.HBox(False, 0)
            self.vbox3 = gtk.VBox(False, 0)
            self.button_run = gtk.Button("Run") # Run Button
            self.button_run.connect("clicked", object.run)
            self.vbox3.pack_start(self.button_run, True, True, 0)
            self.button_run.show()
            self.button_stop = gtk.Button("Stop") # Stop Button
            self.button_stop.connect("clicked", object.stop)
            self.vbox3.pack_start(self.button_stop, True, True, 0)
            self.button_stop.show()
            self.button_reset = gtk.Button("Reset") # Reset Button
            self.button_reset.connect("clicked", object.reset)
            self.hbox3.pack_start(self.button_reset, True, True, 0)
            self.hbox3.add(self.vbox3)
            self.vbox2.add(self.hbox3)
            self.hbox3.show()
            self.vbox3.show()
            self.button_reset.show()
            self.vbox.add(self.vbox2)
            self.label_clock = gtk.Label()
            self.label_clock.set(object.GUI_LABEL_CLOCK)
            self.label_clock.show()
            self.vbox2.add(self.label_clock)
            self.camera_bgr = cv2.imread(object.GUI_CAMERA_IMAGE)
            self.camera_pix = gtk.gdk.pixbuf_new_from_array(self.camera_bgr, gtk.gdk.COLORSPACE_RGB, 8)
            self.camera_img = gtk.Image()
            self.camera_img.set_from_pixbuf(self.camera_pix)
            self.camera_img.show()
            self.vbox2.add(self.camera_img)
            self.vbox.show()
            # Board Image
            self.board_bgr = cv2.imread(object.GUI_BOARD_IMAGE)
            self.board_pix = gtk.gdk.pixbuf_new_from_array(self.board_bgr, gtk.gdk.COLORSPACE_RGB, 8)
            self.board_img = gtk.Image()
            self.board_img.set_from_pixbuf(self.board_pix)
            self.board_img.show()
            self.hbox.add(self.board_img)
            self.hbox.show()
        except Exception as e:
            raise e
    
    ## Update GUI
    def update_gui(self, t):
        self.label_clock.set(self.GUI_LABEL_CLOCK % t)
        while gtk.events_pending():
            gtk.main_iteration_do(False)
    
    ## Draw Board
    def draw_board(self, robot_position, x=75, y=132, x_pad=154, y_pad=40, brown=(116,60,12), yellow=(219,199,6), green=(0,255,0), tall=7, short=2):
        try:
            board_bgr = cv2.imread(self.GUI_BOARD_IMAGE)
            (W,H,D) = board_bgr.shape
            (row_num, at_plant, pass_num, at_end) = robot_position

            # Robot
            if at_end == 0:
                if row_num == 0: # if at beginning
                    (center_x, center_y) = (W - 55, H - 55) ## 55, 55 is best
                elif at_plant != 0:  # if at plant
                    if pass_num == 1:
                        (center_x, center_y) = (W - (at_plant) * x - 77, H - (row_num - 1) * y - 110)
                    elif pass_num == 2:
                        (center_x, center_y) = (W - (6 - at_plant) * x - 77, H - (row_num - 1) * y - 110)
                elif pass_num == 2: # unaligned post-turn
                    (center_x, center_y) = (W - 470, H - (row_num - 1) * y - 110) 
                elif row_num >= 1: # unaligned post-jump
                    (center_x, center_y) = (W - 130, H - (row_num - 1) * y - 110) 
                elif row_num > 4: # to finish
                    (center_x, center_y) = (W - 130, H - (row_num - 1) * y - 110)
                else: # somewhere after plant
                    (center_x, center_y) = (W - 470, H - (row_num - 1) * y - 110)
            elif at_end == 1:
                (center_x, center_y) = (W - 110, H - (row_num-1) * y - 110) # right side at row
            elif at_end == 2:
                (center_x, center_y) = (W - 500, H - (row_num-1) * y - 110) # left side at some row  
            else:
                (center_x, center_y) = (W - 55, H - 55)          
            top_left = ((center_x - 20), (center_y - 20))
            bottom_right = ((center_x + 20), (center_y + 20 ))
            cv2.rectangle(board_bgr, top_left, bottom_right, (255,0,0), thickness=5)

            self.board_pix = gtk.gdk.pixbuf_new_from_array(board_bgr, gtk.gdk.COLORSPACE_RGB, 8)
            self.board_img.set_from_pixbuf(self.board_pix)
        except Exception as e:
            print str(e)

    ## Draw Camera
    def draw_camera(self, bgr):
        try:
            self.camera_bgr = bgr
            rgb = cv2.cvtColor(self.camera_bgr, cv2.COLOR_BGR2RGB)
            self.camera_pix = gtk.gdk.pixbuf_new_from_array(rgb, gtk.gdk.COLORSPACE_RGB, 8)
            self.camera_img.set_from_pixbuf(self.camera_pix)
        except Exception as e:
            print str(e)
# Main
if __name__ == '__main__':
    server = Server(CONFIG_PATH)
    cherrypy.server.socket_host = server.CHERRYPY_ADDR
    cherrypy.server.socket_port = server.CHERRYPY_PORT
    currdir = os.path.dirname(os.path.abspath(__file__))
    conf = {
        '/': {'tools.staticdir.on':True, 'tools.staticdir.dir':os.path.join(currdir,server.CHERRYPY_STATIC_DIR)},
    }
    cherrypy.quickstart(server, '/', config=conf)
