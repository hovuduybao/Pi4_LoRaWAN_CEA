#!/bin/bash

# Add neccessary packages for using LMIC with Raspberry Pi 4

sudo apt-get update && sudo apt-get upgrade -y

# WiringPi
cd /tmp
wget https://project-downloads.drogon.net/wiringpi-latest.deb
sudo dpkg -i wiringpi-latest.deb

gpio -v
cd ~ 

# bcm2835
sudo apt-get install libcap2 libcap-dev -y
sudo adduser $USER kmem
echo 'SUBSYSTEM=="mem", KERNEL=="mem", GROUP="kmem", MODE="0660"' | sudo tee /etc/udev/rules.d/98-mem.rules

cd /tmp
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.71.tar.gz
tar zxvf bcm2835-1.71.tar.gz
cd bcm2835-1.71
./configure
make
sudo make check
sudo make install

# Github for project 
git clone https://github.com/hovuduybao/Pi4_LoRaWAN_CEA/
cd Pi4_LoRaWAN_CEA/csv_error_state_send
make clean && make && sudo ./csv_error_state_send
