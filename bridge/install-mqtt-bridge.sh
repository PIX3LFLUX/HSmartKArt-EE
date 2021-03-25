#!/bin/bash

# style reset
STYLE_RESET='\e[0m'
# red foreground color
STYLE_RED_FORE='\e[31m'
# blue foreground color
STYLE_BLUE_FORE='\e[34m'

#usage()
##{
#	echo -e "Usage: $0 [-d|--domain mydomain.com|ip-address]"
#	echo -e "\tA domain or an IP address on which the server is accessible needs to be provided as argument"
#	exit 1
##}

# Check command line arguments
#while [ "${1:-}" != "" ]
#do
#	case "$1" in
#		-d|--domain)	shift # shift moves arguments position down by one
#						SERVER_DOMAIN="$1"
#						;;
#		* )				usage
#						;;
#	esac
#	shift
#done

# --- Get directory of execution --- #
INSTALLDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# --- System Updates --- #
echo -e "${STYLE_BLUE_FORE}\n\nUpgrade system${STYLE_RESET}\n"
apt update
apt full-upgrade -y

# --- Install packages for build process --- #
echo -e "${STYLE_BLUE_FORE}\n\nInstall development tools${STYLE_RESET}\n"
apt install -y git bc libncurses5-dev bc build-essential ccache cmake libssl-dev
# --- Install packages for Visual Studio Remote GDB --- #
apt install -y openssh-server g++ gdb make ninja-build rsync zip

# --- Download Paho MQTT C library source files --- #
if [ ! -v $MQTTEE_PAHO_SETUP ]
then
	echo -e "${STYLE_BLUE_FORE}\n\nInstall Paho MQTT C library${STYLE_RESET}\n"
	git clone https://github.com/eclipse/paho.mqtt.c.git
	# --- Compile Paho MQTT C library --- #
	cd paho.mqtt.c
	make
	make install
	cd ..

	cat >> /etc/profile.d/mqttee-installation.sh <<EOT
#!/bin/bash
export MQTTEE_PAHO_SETUP=done
EOT

else
	echo -e "Paho MQTT C library already installed. Skipping to next Installation step\n"
fi

# --- Install Mosquitto MQTT Broker --- #
#echo -e "${STYLE_BLUE_FORE}\n\nInstall Mosquitto MQTT Broker${STYLE_RESET}\n"
#apt-add-repository ppa:mosquitto-dev/mosquitto-ppa
#apt update
#apt install -y mosquitto mosquitto-clients

# --- Setup CAN interface (PICAN2) --- #
echo -e "${STYLE_BLUE_FORE}\n\nSetup CAN interface (PICAN2 HAT)${STYLE_RESET}"
if [ ! -v $MQTTEE_PICAN2_SETUP ]
then
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

    cat >> /etc/profile.d/mqttee-installation.sh <<EOT
#!/bin/bash
export MQTTEE_PICAN2_SETUP=done
EOT

else
	echo -e "CAN interface already set up. Skipping to next Installation step\n"
fi

# --- Install SocketCAN --- #
echo -e "${STYLE_BLUE_FORE}\n\nInstall SocketCAN${STYLE_RESET}\n"
apt install can-utils libsocketcan-dev libsocketcan2

# --- Export environment variable for use as MQTT client ID --- #
echo -e "${STYLE_BLUE_FORE}\n\nSetup MQTT Client ID${STYLE_RESET}\n"
read -p 'Please enter MQTT client ID for this device: ' ID
if [ ! -f /etc/profile.d/clientid.sh ]
then
	cat >> /etc/profile.d/clientid.sh <<EOT
# Environment variable for use as MQTT client ID
#!/bin/bash
export MQTT_CLIENTID=$ID
EOT

else
	echo -e "MQTT_CLIENTID environment variable setup already performed. Skipping to next Installation step\n"
fi


# --- Compile source files of MQTT Bridge --- #
echo -e "${STYLE_BLUE_FORE}\n\nCompiling mqttbridge${STYLE_RESET}\n"
export CPATH=/usr/local/include
export LIBRARY_PATH=/usr/local/lib
cd sources
gcc -v -Wall -o $INSTALLDIR/mqttbridge main.c pahomqtt.c pahomqtt.h socketcan.c socketcan.h xmlconfig.c xmlconfig.h -l"rt" -l"paho-mqtt3a" -l"paho-mqtt3as" -l"paho-mqtt3c" -l"paho-mqtt3cs"
cd ..

echo -e "${STYLE_BLUE_FORE}\n\nUsage: $INSTALLDIR/mqttbridge [-c true|false] [-s SUBSCRIBE/TOPIC] [-b my-broker-domain.com|IP] [-h help]${STYLE_RESET}\n\n"
echo -e "${STYLE_RED_FORE}Reboot system to finish installation.${STYLE_RESET}\n"

exit 0