/*
* Company: University of Applied Science Karlsruhe
* Author: Bruno Kempf
*
* Project Name: mqttee-xml-bridge
* File Name: socketcan.h
* Create Date: 26.02.2021 13:12:26
*
* Description: Global function prototypes and variables for SocketCAN functionality
*
* Revision:
*		Revision 0.01 - File Created
*
* Additional Comments:
*/

#ifndef SOCKETCAN_H
#define SOCKETCAN_H

#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

 // Header Files in /usr/include/ may be included using <>. Subdirectories are relative to /usr/include/!
#include <linux/can.h>
#include <linux/can/raw.h>

#endif