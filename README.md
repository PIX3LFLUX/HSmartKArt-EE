**[Introduction](#introduction)** |
**[MQTT-based E/E architecture](#mqtt-based-ee-architecture)** |
**[Installation](#installation)** |
**[Usage](#usage)** |

<FRAME>

# HSmartKArt-EE

**HSmartKArt-EE** provides an approach for mastering the challenges of future vehicle systems.
Connected Cars, autonomous driving and other functions that will shape the future of cars all
rely on cloud-based services. Accordingly cross-domain communication is a demanding topic for
future vehicle systems. Additionally a centralized, zone-oriented topology is key to modern
E/E architectures. Core piece of this E/E architecture is the MQTT protocol.

## Introduction

The HSmartKArt is an electrical powered recumbent bike based on the commercially available
e.Go Kart. With HSmartKArt-EE an E/E architecture is provided for this vehicle as the
foundation for modern driver assitance systems and cloud-based services. It is a reference
deployment of a more generic design for an modern E/E architecture developed in a student
research project at the Karlsruhe University of Applied Sciences. Section 'MQTT-based E/E
architecture reference deployment' describes the HSmartKArt-EE deployment.

This repository contains two installation scripts. One installs a web server which hosts a
JupyterLab environment for public access on an Ubunut Server OS. The other is for an
Debian-based Embbeded Linux device like the Raspberry Pi and installs all components to
program the Raspberry Pi with the firmware.

**NOTE**: Currently the firmware needs to be compiled manually using the gnu11 C compiler.

## MQTT-based E/E architecture

Considering future vehicle systems following requirements apply for E/E architectures:
1. Modulare, centralized topology
2. Efficient hardware and protocols
3. Cross-domain communication
4. Scalability

To meet this demands the following design has been elaborated:
![MQTT-based E/E architecture](docs/mqttee.png)

This is the generic design of an E/E architecture for future vehicle systems. Its modulare and
centralized structure paired with the Bridges and the characteristics of the MQTT protocol
compose an architecture suitable as a platform for sophisticated vehicle netzworks.

For the purpose of prototyping a Raspberry Pi is used as an embedded platform. Also a public
MQTT Broker provided by HiveMQ is used. The main components of the design are:
* **CAN-to-MQTT Bridge**: This device is a Raspberry Pi with a CAN HAT. It listens directly on
	the	CAN bus and grabs CAN frames. The frames can be processed and/or published. Which CAN
	frames the device publishes is configured by an XML configuration file. So this device is
	essentially a CAN-to-MQTT Bridge.

* **MQTT Broker**: An implementation of a local but publicly accessable Broker as proposed in
	the generic design is currently not included in this repository. Eclipse Mosquitto could be
	used as a local Broker.	Unfortunately the configuration of all the network components to
	make the Broker accessible from the outside of the local area network is currently not
	finished and therefore not included in the repository. For the purpose of prototyping a
	public MQTT Broker running on the HiveMQ platform is used for handling the distribution of
	messages throughout the	connected clients. The public Broker is used to evaluate the global
	access of the CAN data.

![Reference deployment](docs/hsmartkartee.png)


## Installation
To install the MQTT Bridge on the Raspberry Pi an installation script is provided. Also an
installation script for hosting the Jupyter Noteboooks from a Server is provided. The Server
may be used to develop Python applications for analyzing, visualizing and recording the CAN
data received using MQTT.

### Prerequisites

For the **MQTT/CAN Bridge** component a Raspberry Pi with a CAN HAT is needed. Alternatively
using a similar device running a Debian-based Linux distribution with a CAN interface is
possible. The CAN interface needs to be compatible with the SocketCAN Linux library.

When using HiveMQ Cloud as public **MQTT Broker** an account is needed to create a HiveMQ Cloud
cluster. After creating an account

For the **Ubuntu Server** a Host PC (or Laptop) running the Ubuntu Server 20.04 OS is needed.
Also a public IPv4 address and/or a DNS with an A record for the public IP address needs to be
available.
**NOTE**:	When using Unitymedia Connect Box consider the setup of an Portmapper to avoid
			errors due to the IP policy of Unitymedia (DS Lite).

### Installation script for MQTT Bridge
For installation transfer the `install-mqtt-bridge.sh` file to the Raspberry Pi. To install
the software for the MQTT Bridge on a Raspberry Pi simply enter the following commands in the
command line:
```bash
sudo bash ./install-mqtt-bridge.sh
```
The script will install following comoponents:
1. Basic libraries and dependencies
2. Paho MQTT C Client library (https://www.eclipse.org/paho/index.php?page=clients/c/index.php)
3. The mqttee-bridge software

**NOTE**:	SocketCAN library for CAN communication is part of the Linux Kernel and needs no
			manual installation. See also https://en.wikipedia.org/wiki/SocketCAN for more
			information.

### Installation script for Jupyter Notebook Server
For installation transfer the `install-jupyter.sh` file (VERZEICHNIS MIT ABHÄNGIGKEITEN?????????????????????????????????????????????????) to the Host PC running Ubuntu Server
OS. Enter following command in the command line to install the Jupyter Notebook Server:
```bash
sudo bash ./install-jupyter.sh
```
The script will install following comoponents:
1. Basic libraries and dependencies
2. Certbot for SSL certificate
3. Nginx Web Server
4. Linux firewall setup (ufw)
5. Miniconda 3
6. JupyterLab (including Jupyter Server and Jupyter Notebook components)
7. Paho MQTT Python Client

The Jupyter Notebook Server is ready to use with a browser for developing applications
subscribing or publishing to CAN data topics or configuration topics and process the received
sensor data.

## Usage

### XML configuration file
The configuration of the MQTT Bridges is described using the
[Extensible Markup Language (XML)](https://www.w3.org/XML/).

The provided XML configuration file applies for a CAN-to-MQTT Bridge.

ABBILDUNG CAN-to-MQTT Bridge !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

It describes the CAN frames available on the bus which have to be transmitted on a MQTT topic.
The topic on which the CAN frame will be published is automatically created using the
following pattern:
```bash
DEVICE/MODULE/LOCATION/dataCAN
```
The part in `UPPER CASE` is specified in the XML configuration file. See provided configuration
file for reference. These topics are customizeable by editing the configuration file and
updating the MQTT Bridge with the new file. The subtopic `dataCAN` is fixed and serves as
channel for CAN data traffic.

### MQTT Bridge
To start the MQTT Bridge software run the following command in the directory XYZDIRECTORY?????????????????????????????????????????????????????????????????????????????????????????????????????

#### Configuration update
To update the configuration of an MQTT Bridge simply publish the new XML configuration file on
the configuration topic of the corresponding device. The device will automatically apply the
received configuration file and resume to normal operation, transmitting the CAN frames
specified in the new configuration file on the topic `DEVICE/MODULE/LOCATION/dataCAN`. The
device configuration topics have the following pattern:
```bash
DEVICE/config
```
Like the pattern for the CAN data topics the `UPPER CASE` part is specified in the configuration
file and can be changed by editing the configuration file. The subtopic `config` is fixed and
serves as configuration channel.

**NOTE**:	The topic on which the new configuration file needs to be published is determined by
			the value of the <DEVICE> tag in the old configuration file.

### Jupyter Notebook Server
A Nginx Web Server is installed on the Host PC for SSL implementation. Nginx redirects the
incoming HTTPS traffic to the Jupyter Notebook Server.

The Jupyter Notebook Server is setup as a systemd service and will start automatically on
system startup. To use the Jupyter Notebook Server open a browser and enter the domain
associated with the IP adress of Ubuntu Server.
In a local environment the local IP address of the Ubuntu Server can be used.

ABBILDUNG: JupyterLab im Browser geöffnet, Fokus auf die Adresszeile. Text zur Abbildung in der erklärt wird, dass die Domain für den Ubuntu Server eingerichtet wurde.
