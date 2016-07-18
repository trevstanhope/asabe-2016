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
    print "Using default config file"
    CONFIG_PATH = 'settings.json'

# CherryPy3 server
class Server:
    
    ### Initialize ###
    def __init__(self, config_path):
        
        # Configuration
        self.load_config(config_path)
        
        # Initializers
        self.__init_zmq__()
        self.__init_tasks__()
        self.__init_statemachine__()
        self.__init_gui__()

    ### Useful Functions ###
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
       
    ### ZMQ Functions ###
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
    
    ### State-Machine Functions ###
    def __init_statemachine__(self):
        self.bgr = cv2.imread(self.GUI_CAMERA_IMAGE)
        self.mask = cv2.imread(self.GUI_MASK_IMAGE)
        self.running = False
        self.start_time = time.time()
        self.end_time = self.start_time + self.RUN_TIME
        self.clock = self.end_time - self.start_time
        self.orange_balls_collected = 0
        self.green_balls_collected = 0
        self.blank = np.zeros((self.CAMERA_HEIGHT, self.CAMERA_WIDTH), np.uint8)
        self.last_value = 0
        self.last_color = None
        self.transfer_complete = False
    def decide_action(self, request):
        """
        Below is the Pseudocode for how the decisions are made:
        start()
        """
        self.pretty_print("DECIDE", "Last Action: %s" % request['last_action'])
        self.pretty_print("DECIDE", "Robot: %s" % request['robot'])
        
        if request['robot'] == 'picker':
            heading, distance, color = self.find_ball(np.array(request['bgr'], np.uint8))

        ## If paused
        if self.running == False:
            self.pretty_print("DECIDE", "Trial not started! Robots will zero themselves and wait!")
            action = 'Z'

        ## If running
        elif self.clock > 0: 
            
            ## Picker 
            if request['robot'] == 'picker':
                if request['last_action'] == 'Z':
                    action = 'F5000' 
                elif (request['last_action'] == 'F') or (request['last_action'] == 'L') or (request['last_action'] == 'R') or (request['last_action'] == 'C') or (request['last_action'] == 'E') or (request['last_action'] == 'B'):
                    if ((heading is not None) and (distance is not None) and (color is not None)):
                        self.last_color = color
                        self.pretty_print("DECIDE", "Heading: %d, Distance: %d, Color: %s" % (heading, distance, color))
                        if distance <= self.TARGET_DISTANCE:
                            if color == 'green':
                                action = 'G'
                                self.green_balls_collected += 1
                            elif color == 'orange':
                                action = 'O'
                                self.orange_balls_collected += 1
                        elif heading < -self.ALIGNMENT_TOLERANCE:
                            action = 'L' + str(abs(heading))
                        elif heading > self.ALIGNMENT_TOLERANCE:
                            action = 'R' + str(abs(heading))
                            self.last_value = abs(heading)
                        else:
                            action = 'F' + str(abs(distance))
                            self.last_value = abs(heading)
                    else:
                        if (request['last_action'] == 'F') and (self.last_value > 0 and self.last_value <= 2000):
                            if self.last_color == 'green':
                                action = 'G'
                                self.green_balls_collected += 1
                            else:
                                action = 'O'
                                self.orange_balls_collected += 1
                        else:
                            self.pretty_print("DECIDE", "No ball detected! Backing up for safety!")
                            action = 'B500'
                            self.last_value = 500
                elif (request['last_action'] == 'G') or (request['last_action'] == 'O'):
                    if (self.orange_balls_collected + self.green_balls_collected == 1):
                        action = "E"
                    elif (self.orange_balls_collected + self.green_balls_collected == 7):
                        action = "E"
                    else:
                        action = "C"
                elif (self.orange_balls_collected + self.green_balls_collected == 8):
                    action = 'S'
                elif request['last_action'] == 'S':
                    action = 'A'
                elif request['last_action'] == 'A':
                    action = 'J'
                elif request['last_action'] == 'J':
                    action = 'T'
                elif request['last_action'] == 'T':
                    self.transfer_complete = True
                    action = 'W'
                elif request['last_action'] == '?':
                    action = '?'
                    self.pretty_print("WARNING", "Last action unknown! Check logs!")
                else:
                    action = '?'
                    self.pretty_print("WARNING", "Unsupported PICKER action!")
            ## Delivery
            elif request['robot'] == 'delivery':
                if request['last_action'] == 'Z':
                    action = 'J' 
                elif request['last_action'] == 'J':
                    action = 'A' 
                elif request['last_action'] == 'A':
                    action = 'F' 
                elif request['last_action'] == 'F':  
                    action = 'T' 
                elif request['last_action'] == 'T':
                    action = 'W'
                elif request['last_action'] == 'W':
                    if self.transfer_complete:
                        action = 'R'
                    else:
                        action = 'W'
                elif request['last_action'] == 'R':
                    action = 'G' + str(self.green_balls_collected)
                elif request['last_action'] == 'G':
                    action = 'O' + str(self.orange_balls_collected)
                elif request['last_action'] == 'O':
                    action = 'D'
                elif request['last_action'] == 'D':
                    action = '?'
                elif request['last_action'] == '?':
                    action = '?'
                    self.pretty_print("WARNING", "Last action unknown! Check logs!")
                else:
                    action = '?'
                    self.pretty_print("WARNING", "Unsupported DELIVERY action!")
            else:
                raise Exception("Unrecognized robot identifier!")
        ## If times is up
        else:
            self.pretty_print("DECIDE", "Time is up! Robots will wait!")
            action = 'W' # halt and wait at end
        self.pretty_print("DECIDE", "Next Action: %s" % action)
        return action

    ### Computer Vision ###
    def find_ball(self, bgr, RADIUS_MIN=4, RADIUS_MAX=40):
        """
        Find the contours for both masks, then use these
        to compute the minimum enclosing circle and centroid
        Returns:
            color : green, yellow
            pos: heading, distance, color
        """
        GREEN_LOWER = (self.GREEN_HUE_MIN, self.GREEN_SAT_MIN, self.GREEN_VAL_MIN)
        GREEN_UPPER = (self.GREEN_HUE_MAX, self.GREEN_SAT_MAX, self.GREEN_VAL_MAX) 
        ORANGE_LOWER = (self.ORANGE_HUE_MIN, self.ORANGE_SAT_MIN, self.ORANGE_VAL_MIN) 
        ORANGE_UPPER = (self.ORANGE_HUE_MAX, self.ORANGE_SAT_MAX, self.ORANGE_VAL_MAX)

        if self.VERBOSE: self.pretty_print("CV2", "Searching for ball ...")
        blurred = cv2.GaussianBlur(bgr, (25, 25), 0)
        hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)
        green_mask = cv2.inRange(hsv, GREEN_LOWER, GREEN_UPPER)
        green_mask = cv2.erode(green_mask, None, iterations=2)
        green_mask = cv2.dilate(green_mask, None, iterations=2)
        orange_mask = cv2.inRange(hsv, ORANGE_LOWER, ORANGE_UPPER)
        orange_mask = cv2.erode(orange_mask, None, iterations=4)
        orange_mask = cv2.dilate(orange_mask, None, iterations=2)
        orange_bgr = np.dstack((self.blank, orange_mask, orange_mask)) # set self.mask to be accessed by the GUI
        green_bgr = np.dstack((self.blank, green_mask, self.blank)) # set self.mask to be accessed by the GUI
        detected_balls = []
        orange_circles = cv2.HoughCircles(orange_mask, cv.CV_HOUGH_GRADIENT, 4.0, 10)
        green_circles = cv2.HoughCircles(green_mask, cv.CV_HOUGH_GRADIENT, 4.0, 10)
        if orange_circles is not None:
            orange_circles = np.round(orange_circles[0, :]).astype("int")
        if green_circles is not None:
            green_circles = np.round(green_circles[0, :]).astype("int")
 
        # Green Contours
        green_contours = cv2.findContours(green_mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)[-2]
        center = None # initialize the current (x, y) center of the ball
        if len(green_contours) > 0: # only proceed if at least one contour was found
            for c in green_contours:
                ((x, y), radius) = cv2.minEnclosingCircle(c)
                if (radius > RADIUS_MIN) and (radius < RADIUS_MAX): # only proceed if the radius meets a minimum size
                    if green_circles is not None:
                        for (x2,y2,r2) in green_circles:
                            d = np.sqrt((x - x2)**2 + (y - y2)**2)
                            if (d < 20) and (r2 > RADIUS_MIN) and (r2 < RADIUS_MAX):
                                detected_balls.append((x,y,radius,'green'))
                                cv2.circle(bgr, (int(x2), int(y2)), int(r2), (0, 255, 0), 2) 
                    else:
                        cv2.putText(green_bgr, 'X', (int(x)-10,int(y)+10), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,0,255), 2)

        # Orange Contours
        orange_contours = cv2.findContours(orange_mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)[-2]
        center = None # initialize the current (x, y) center of the ball
        if len(orange_contours) > 0: # only proceed if at least one contour was found
            for c in orange_contours:
                ((x, y), radius) = cv2.minEnclosingCircle(c)
                if (radius >= RADIUS_MIN) and (radius <= RADIUS_MAX): # only proceed if the radius meets a minimum size
                    if orange_circles is not None:
                        for (x2,y2,r2) in orange_circles:
                            d = np.sqrt((x - x2)**2 + (y - y2)**2)
                            if d < 20 and (r2 > RADIUS_MIN) and (r2 < RADIUS_MAX):
                                detected_balls.append((x,y,r2,'orange'))
                                cv2.circle(bgr, (int(x2), int(y2)), int(r2), (0, 255, 255), 2)
                    else:
                        cv2.putText(orange_bgr, 'X', (int(x)-10,int(y)+10), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,0,255), 2)

        # Draw
        if len(detected_balls) > 0:
            for x,y,r,color in detected_balls:
                d = self.estimate_distance(y,r)
                if color == 'green':
                    cv2.putText(bgr, str(d), (int(x)+10, int(y)+10), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,0,255), 2)
                if color == 'orange':
                    cv2.putText(bgr, str(d), (int(x)+10, int(y)+10), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,0,255), 2)
            x,y,r,c = max(detected_balls) # return the farthest to the right, use min() for left
            cv2.circle(bgr, (int(x),int(y)), 3, (0, 0, 255), -1)
            self.bgr = bgr # set BGR of GUI with the updated, drawn-on version
            self.mask = orange_bgr + green_bgr
            heading = self.estimate_heading(x)
            distance = self.estimate_distance(y,r)
            return heading, distance, color
        else:
            self.bgr = bgr
            self.mask = orange_bgr + green_bgr
            return None, None, None
    def estimate_distance(self, y, r):
        return self.DISTANCE_GAIN * int(2200 * r ** -1.7)
    def estimate_heading(self, x): 
        return self.HEADING_GAIN * int(x - self.CAMERA_WIDTH / 2)
        
    ### CherryPy Server Functions ###
    def __init_tasks__(self):
        if self.VERBOSE: self.pretty_print('CHERRYPY', 'Initializing Monitors ...')
        try:
            Monitor(cherrypy.engine, self.listen, frequency=self.CHERRYPY_LISTEN_INTERVAL).subscribe()
            Monitor(cherrypy.engine, self.refresh, frequency=self.CHERRYPY_REFRESH_INTERVAL).subscribe()
        except Exception as error:
            self.pretty_print('CHERRYPY', str(error))
    def listen(self):
        """ Listen for Next Sample """
        if self.VERBOSE: self.pretty_print('CHERRYPY', 'Listening for nodes ...')
        req = self.receive_request()
        action = self.decide_action(req)
        resp = self.send_response(action)
    def refresh(self):
        """ Update the GUI """
        self.gui.draw_camera(self.bgr, self.mask)
        picker_position = (0,0) #TODO
        delivery_position = (0,0) #TODO
        self.gui.draw_board(picker_position, delivery_position)
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

    ### GUI Functions ###
    def __init_gui__(self):
        if self.VERBOSE: self.pretty_print('GUI', 'Initializing GUI ...')
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
            
            # Constants
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
            self.camera_mask = cv2.imread(object.GUI_MASK_IMAGE)
            self.camera_pix = gtk.gdk.pixbuf_new_from_array(self.camera_bgr, gtk.gdk.COLORSPACE_RGB, 8)
            self.camera_pix_mask = gtk.gdk.pixbuf_new_from_array(self.camera_mask, gtk.gdk.COLORSPACE_RGB, 8)
            self.camera_img = gtk.Image()
            self.camera_img_mask = gtk.Image()
            self.camera_img.set_from_pixbuf(self.camera_pix)
            self.camera_img_mask.set_from_pixbuf(self.camera_pix_mask)
            self.camera_img.show()
            self.camera_img_mask.show()
            self.vbox2.add(self.camera_img)
            self.vbox2.add(self.camera_img_mask)
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
    def draw_board(self, picker_position, delivery_position, x=75, y=132, x_pad=154, y_pad=40, brown=(116,60,12), yellow=(219,199,6), green=(0,255,0), tall=7, short=2):
        try:
            board_bgr = cv2.cvtColor(cv2.imread(self.GUI_BOARD_IMAGE), cv2.COLOR_BGR2RGB)
            (W,H,D) = board_bgr.shape

            # Picker Robot
            #!TODO
            
            # Delivery Robot
            #!TODO

            self.board_pix = gtk.gdk.pixbuf_new_from_array(board_bgr, gtk.gdk.COLORSPACE_RGB, 8)
            self.board_img.set_from_pixbuf(self.board_pix)
        except Exception as e:
            print str(e)

    ## Draw Camera
    def draw_camera(self, bgr, mask):
        try:
            self.camera_bgr = cv2.resize(bgr, (320,240))
            self.camera_mask = cv2.resize(mask, (320,240))
            rgb = cv2.cvtColor(self.camera_bgr, cv2.COLOR_BGR2RGB)
            rgb_mask = cv2.cvtColor(self.camera_mask, cv2.COLOR_BGR2RGB)
            self.camera_pix = gtk.gdk.pixbuf_new_from_array(rgb, gtk.gdk.COLORSPACE_RGB, 8)
            self.camera_pix_mask = gtk.gdk.pixbuf_new_from_array(rgb_mask, gtk.gdk.COLORSPACE_RGB, 8)
            self.camera_img.set_from_pixbuf(self.camera_pix)
            self.camera_img_mask.set_from_pixbuf(self.camera_pix_mask)
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
