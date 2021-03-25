/*
* Company: University of Applied Science Karlsruhe
* Author: Bruno Kempf
*
* Project Name: mqtteebridge
* File Name: pahomqtt.h
* Create Date: 26.02.2021 13:12:26
*
* Description: Global function prototypes and variables for MQTT functionality
*
* Revision:
*		Revision 0.01 - File Created
*
* Additional Comments:
*/

#ifndef PAHOMQTT_H
#define PAHOMQTT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "MQTTAsync.h"
#include "xmlconfig.h"

#define QOS	0

bool disconnected;
bool connected;


// --- Typedefs --- //
typedef struct
{
	char device[XML_LEN_VALUES];
	char module[XML_LEN_VALUES];
	char location[XML_LEN_VALUES];
	char canid[XML_LEN_VALUES];
} mqttrcvtopic_t;


// --- Connection Callbacks --- //
void connlost(void* context, char* cause);
void onDisconnect(void* context, MQTTAsync_successData* response);
void onConnectFailure(void* context, MQTTAsync_failureData* response);
void onConnect(void* context, MQTTAsync_successData* response);

// --- Publish/Subscribe Callbacks --- //
void onPublish(void* context, MQTTAsync_successData* response);
void onPublishFailure(void* context, MQTTAsync_failureData* response);
int msgrcvdCallback(void* context, char* topicName, int topicLen, MQTTAsync_message* message);
void onSubscribe(void* context, MQTTAsync_successData* response);
void onSubscribeFailure(void* context, MQTTAsync_failureData* response);

#endif