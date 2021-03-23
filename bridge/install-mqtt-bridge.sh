#!/bin/bash

# --- System Updates --- #
apt update
apt full-upgrade

# --- Install packages for build process --- #
apt install -y git bc libncurses5-dev bc build-essential ccache cmake libssl-dev
# --- Install packages for Visual Studio Remote GDB --- #
apt install -y openssh-server g++ gdb make ninja-build rsync zip

# --- Download Paho MQTT C library source files --- #
git clone https://github.com/eclipse/paho.mqtt.c.git
# --- Compile Paho MQTT C library --- #
cd paho.mqtt.c/
make
make install

# --- Install Mosquitto MQTT Broker --- #
#apt-add-repository ppa:mosquitto-dev/mosquitto-ppa
#apt update
#apt install mosquitto mosquitto-clients

# --- Setup CAN interface (PICAN2) --- #
cat >> /boot/config.txt <<EOT
# PICAN2 Setup
dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=25
dtoverlay=spi-bcm2835-overlay
EOT

cat >> /etc/rc.local <<EOT
# Set CAN interface (PICAN2) up
ip link set can0 up type can bitrate 500000
EOT

# --- Install SocketCAN --- #
apt install can-utils libsocketcan-dev libsocketcan2

# --- Export environment variable for use as MQTT client ID --- #
read -p 'Please enter MQTT client ID for this device: ' ID
touch /etc/profile.d/clientid.sh
cat >> /etc/profile.d/clientid.sh <<EOT
# Environment variable for use as MQTT client ID
#!/bin/bash
export MQTT_CLIENTID=$ID
EOT


# --- Compile source files of MQTT Bridge --- #
echo -e "\nCompiling mqtt-bridge firmware...\n"

# --- Setup systemd service --- #


echo -e "\nReboot to finish installation!"