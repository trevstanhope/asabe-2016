#!/bin/sh
git config --global user.name "Trevor Stanhope"
git config --global user.email "tpstanhope@gmail.com"
apt-get update
apt-get upgrade
apt-get install build-essential cmake -y -qq
apt-get install python-dev python-setuptools python-pip -y -qq
apt-get install python-zmq python-serial python-cherrypy3 -y -qq
apt-get install arduino arduino-mk -y -qq
apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev -y -qq
apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev -y -qq
apt-get install ipython -y -qq
apt-get install python-opencv -y -qq

# Arduino Libraries
cp -r libraries/* /usr/share/arduino/libraries
