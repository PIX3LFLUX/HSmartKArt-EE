/*
* Company: University of Applied Science Karlsruhe
* Author: Bruno Kempf
*
* Project Name: mqtteebridge
* File Name: xmlconfig.c
* Create Date: 26.02.2021 13:12:26
*
* Description: Methods for reading XML file and device configuration
*
* Revision:
*		Revision 0.01 - File Created
*
* Additional Comments:
*/

#include "xmlconfig.h"
#include "socketcan.h"
#include "MQTTAsync.h"


// --- External variables --- //
extern char* config_rcvd;
extern xmldevice_t* device;

// --- Global variables --- //
char value[XML_LEN_VALUES];


/*
* Check configuration file
*/
uint xml_check()
{
	char* pos = config_rcvd;

	if ((pos = strstr(pos, XML_TAG_CHECK)) != NULL)
	{
		pos += strlen(XML_TAG_CHECK);
		xml_value_read(pos);
		if (strcmp(value, XML_CHECK_STRING))
		{
			printf("Check-string of configuration file is not valid\nAborting configuration process\n");
			return EXIT_FAILURE;
		}
		else
		{
			printf("Check-string of configuration file is valid\nProceeding with configuration process\n");
			return EXIT_SUCCESS;
		}
	}
	else
	{
		printf("No check-string found in configuration file\nAborting configuration process\n");
		return EXIT_FAILURE;
	}
}


/*
* Read the value of the parameter
*/
void xml_value_read(char* pos)
{
	size_t i = 0;
	while (*(pos + i) != '<')
	{
		value[i] = *(pos + i);
		i++;
	}
	value[i] = '\0';
	//printf("Value: %s\n", value);
	/* Alternative:
	char c;
	size_t i = 0;

	char buf[16];
	memset(buf, 0, sizeof(buf));

	while ((c = (char)fgetc(fp)) != '<')
	{
		buf[i] = c;
		i++;
	}
	buf[i] = '\0';
	printf("Value: %s\n", buf);

	return (char*)malloc(strlen(buf) * sizeof(char));
	*/
}


/*
* Setup for config (allocate memory for configuration structure)
*/
void xml_setup()
{
	char* pos = config_rcvd;
	
	// --- Allocate memory for DEVICE configuration structure --- //
	device = (xmldevice_t*)malloc(sizeof(xmldevice_t));
	
	// --- Allocate memory for MODULE configuration structures --- //
	pos = strstr(pos, XML_TAG_CNTMOD);
	pos += strlen(XML_TAG_CNTMOD);
	xml_value_read(pos);
	device->cntmod = (size_t)atoi(value);
	device->modules = (xmlmodule_t*)malloc(device->cntmod * sizeof(xmlmodule_t));

	// --- Allocate memory for FRAME configuration structures --- //
	//pos = config_rcvd; // Reset positional pointer
	for (size_t i_mod = 0; i_mod < device->cntmod; i_mod++)
	{
		pos = strstr(pos, XML_TAG_CNTFRM);
		pos += strlen(XML_TAG_CNTFRM);
		xml_value_read(pos);
		device->modules[i_mod].cntfrm = (size_t)atoi(value);
		device->modules[i_mod].frames = (xmlframe_t*)malloc(device->modules[i_mod].cntfrm * sizeof(xmlframe_t));
		
		// --- Allocate memory for SIGNAL configuration structures --- //
		for (size_t i_frm = 0; i_frm < device->modules[i_mod].cntfrm; i_frm++)
		{
			pos = strstr(pos, XML_TAG_CNTSIG);
			pos += strlen(XML_TAG_CNTSIG);
			xml_value_read(pos);
			device->modules[i_mod].frames[i_frm].cntsig = (uint8_t)atoi(value);
			device->modules[i_mod].frames[i_frm].signals = (xmlsignal_t*)malloc(device->modules[i_mod].frames[i_frm].cntsig * sizeof(xmlsignal_t));
		}
	}
}


/*
* Read config
*/
void xml_config_read()
{
	char* pos = config_rcvd;
	
	// --- Read configuration parameters into structure --- //
	pos = strstr(pos, XML_TAG_DEVNAME);
	pos += strlen(XML_TAG_DEVNAME);
	xml_value_read(pos);
	strcpy(device->name, value);
	
	//pos = config_rcvd; // Reset positional pointer
	for (size_t i_mod = 0; i_mod < device->cntmod; i_mod++)
	{
		pos = strstr(pos, XML_TAG_MODNAME);
		pos += strlen(XML_TAG_MODNAME);
		xml_value_read(pos);
		strcpy(device->modules[i_mod].name, value);

		pos = strstr(pos, XML_TAG_LOC);
		pos += strlen(XML_TAG_LOC);
		xml_value_read(pos);
		strcpy(device->modules[i_mod].location, value);

		for (size_t i_frm = 0; i_frm < device->modules[i_mod].cntfrm; i_frm++)
		{
			pos = strstr(pos, XML_TAG_FRMID);
			pos += strlen(XML_TAG_FRMID);
			xml_value_read(pos);
			device->modules[i_mod].frames[i_frm].canid = (unsigned int)atoi(value);
			for (size_t i_sig = 0; i_sig < device->modules[i_mod].frames[i_frm].cntsig; i_sig++)
			{
				pos = strstr(pos, XML_TAG_SIGNAME);
				pos += strlen(XML_TAG_SIGNAME);
				xml_value_read(pos);
				strcpy(device->modules[i_mod].frames[i_frm].signals[i_sig].name, value);

				pos = strstr(pos, XML_TAG_SIGPOS);
				pos += strlen(XML_TAG_SIGPOS);
				xml_value_read(pos);
				device->modules[i_mod].frames[i_frm].signals[i_sig].pos = (uint8_t)atoi(value);

				pos = strstr(pos, XML_TAG_SIGLEN);
				pos += strlen(XML_TAG_SIGLEN);
				xml_value_read(pos);
				device->modules[i_mod].frames[i_frm].signals[i_sig].len = (uint8_t)atoi(value);
			}
		}
	}
}


/*
* Create MQTT topic for frames
*/
void xml_create_topics()
{
	char tmp[XML_LEN_TOPICS] = {};
	strcat(tmp, device->name);
	strcat(tmp, "/config");
	strcpy(device->topic_config, tmp);
	for (size_t i_mod = 0; i_mod < device->cntmod; i_mod++)
	{
		memset(tmp, 0, sizeof(tmp));
		strcat(tmp, device->name);
		strcat(tmp, "/");
		strcat(tmp, device->modules[i_mod].name);
		strcat(tmp, "/");
		strcat(tmp, device->modules[i_mod].location);
		strcat(tmp, "/dataCAN");
		strcpy(device->modules[i_mod].topic_can, tmp);
	}
}
