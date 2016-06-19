#!/bin/sh
git config --global user.name "Trevor Stanhope"
git config --global user.email "tpstanhope@gmail.com"
apt-get update
apt-get upgrade
apt-get install build-essential cmake -y -qq
apt-get install python-dev python-setuptools python-pip -y -qq
apt-get install python-opencv python-zmq python-serial python-cherrypy3 -y -qq
apt-get install arduino arduino-mk -y -qq
apt-get install python-numpy -y -qq

# Arduino Libraries
cp -r libraries/* /usr/share/arduino/libraries
