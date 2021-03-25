/*
* Company: University of Applied Science Karlsruhe
* Author: Bruno Kempf
*
* Project Name: mqtteebridge
* File Name: xmlconfig.h
* Create Date: 26.02.2021 13:12:26
*
* Description: Global function prototypes and variables for device configuration
* using XML files.
*
* Revision:
*		Revision 0.01 - File Created
*
* Additional Comments:
*/

#ifndef XMLCONFIG_H
#define XMLCONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>


// --- Defines --- //
#define XML_CHECK_STRING	"MQTTEE-v0.1"

#define XML_TAG_CHECK	"<CHECK-STRING>"
#define XML_TAG_DEVNAME	"<DEVICE-NAME>" // Used to create topic
#define XML_TAG_CNTMOD	"<COUNTED-MODULES>"
#define XML_TAG_MODULE	"<MODULE>"
#define XML_TAG_MODNAME	"<MODULE-NAME>" // Used to create topic
#define XML_TAG_LOC		"<LOCATION>" // Used to create topic
#define XML_TAG_CNTFRM	"<COUNTED-FRAMES>"
#define XML_TAG_FRAME	"<FRAME>"
#define XML_TAG_FRMID	"<IDENTIFIER>"
#define XML_TAG_CNTSIG	"<COUNTED-SIGNALS>"
#define XML_TAG_SIGNAL	"<SIGNAL>"
#define XML_TAG_SIGNAME	"<SIGNAL-NAME>"
#define XML_TAG_SIGPOS	"<SIG-POS>"
#define XML_TAG_SIGLEN	"<LENGTH>"
#define XML_TAG_XMLEND	"</xml>"

#define XML_LEN_TAGS	64
#define XML_LEN_VALUES	20 // 64 bit --> 2^64 = 18446744073709551616 = 20 digits
#define XML_LEN_TOPICS	256

// --- Typedefs --- //
typedef struct
{
	char name[XML_LEN_VALUES];
	uint8_t pos;
	uint8_t len;
} xmlsignal_t;

typedef struct
{
	char topic[XML_LEN_TOPICS];
	unsigned int canid;
	uint8_t cntsig;
	xmlsignal_t* signals;
} xmlframe_t;

typedef struct
{
	char type[XML_LEN_VALUES];
	char name[XML_LEN_VALUES];
	char location[XML_LEN_VALUES];
	size_t cntfrm;
	xmlframe_t* frames;
} xmlmodule_t;

typedef struct
{
	char name[XML_LEN_VALUES];
	char topic_config[XML_LEN_TOPICS];
	size_t cntmod;
	xmlmodule_t* modules;
} xmldevice_t;

// --- Global variables --- //
char* config_rcvd;
xmldevice_t* device;
bool config_update;

// --- Function prototypes --- //
uint xml_check();
void xml_value_read(char* pos);
void xml_setup();
void xml_config_read();
void xml_create_topics();


#endif