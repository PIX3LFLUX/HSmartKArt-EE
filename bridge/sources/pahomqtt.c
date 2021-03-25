/*
* Company: University of Applied Science Karlsruhe
* Author: Bruno Kempf
*
* Project Name: mqtteebridge
* File Name: pahomqtt.c
* Create Date: 26.02.2021 13:12:26
*
* Description: Methods for MQTT functionality using Paho C library
*
* Revision:
*		Revision 0.01 - File Created
*
* Additional Comments:
*/

#include "pahomqtt.h"

// --- External variables --- //
extern bool disconnected;
extern bool connected;

extern bool config_update;

extern char* config_rcvd;


/*
* Connection Callbacks
*/
void connlost(void* context, char* cause)
{
	MQTTAsync client_handle = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;
	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);
	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTAsync_connect(client_handle, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start reconnection, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}
void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Disconnection successful\n");
	disconnected = true;
}
void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connection failed, rc %d\n", response ? response->code : 0);
	exit(EXIT_FAILURE);
}
void onConnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful connection of Client %s\n", (char*)context);
	connected = true;
}



/*
* Publish/Subscribe Callbacks
*/
void onPublish(void* context, MQTTAsync_successData* response)
{
	//printf("Message with token value %d delivery confirmed\n", response->token);
}

void onPublishFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Publish failed, rc %d\n", response ? response->code : 0);
	exit(EXIT_FAILURE);
}

/*
* This is a callback function. The client application must provide an implementation of this function
* to enable asynchronous notification of delivery of messages to the server. The function is registered
* with the client library by passing it as an argument to MQTTAsync_setCallbacks(). It is called by the
* client library after the client application has published a message to the server. It indicates that the
* necessary handshaking and acknowledgements for the requested quality of service (see
* MQTTAsync_message.qos) have been completed. This function is executed on a separate thread to the one on
* which the client application is running.
*/
/*
void delivrycmplt(void* context, MQTTAsync_token token)
{

}
*/


/*
* This is a callback function. The client application must provide an implementation of
* this function to enable asynchronous receipt of messages. The function is registered
* with the client library by passing it as an argument to MQTTAsync_setCallbacks(). It
* is called by the client library when a new message that matches a client subscription
* has been received from the server. This function is executed on a separate thread to
* the one on which the client application is running.
*
* This function must return a boolean value indicating whether or not the message
* has been safely received by the client application. Returning true indicates that
* the message has been successfully handled. Returning false indicates that there
* was a problem. In this case, the client library will reinvoke
* MQTTAsync_messageArrived() to attempt to deliver the message to the application
* again.
*/
int msgrcvdCallback(void* context, char* topicName, int topicLen, MQTTAsync_message* message)
{
	uint16_t msgrcvdlen = (uint16_t)message->payloadlen;
	uint8_t* msgrcvdptr = (uint8_t*)message->payload;
	uint8_t* dataptr_u8 = NULL;
	uint16_t* dataptr_u16 = NULL;
	uint32_t* dataptr_u32 = NULL;
	uint64_t* dataptr_u64 = NULL;
	size_t i_tm = 0; // index topic message
	size_t i_ts = 0; // index topic structure
	size_t i_dev = 0;
	size_t i_mod = 0;
	size_t i_frm = 0;
	mqttrcvtopic_t rcvTopic;
	memset(rcvTopic.canid, 0, sizeof(rcvTopic.canid));
	memset(rcvTopic.device, 0, sizeof(rcvTopic.device));
	memset(rcvTopic.module, 0, sizeof(rcvTopic.module));

	if (strstr(topicName, "/config/update") == NULL)
	{
		printf("\n\nMessage Content (byte stream): ");
		for (i_tm = 0; i_tm < msgrcvdlen; i_tm++)
			printf("%u ", *msgrcvdptr++);
		
		// --- Dissect MQTT topic --- //
		i_tm = 0;
		while (*(topicName + i_tm) != '/')
		{
			rcvTopic.device[i_ts] = *(topicName + i_tm);
			i_tm++;
			i_ts++;
		}
		i_tm++;
		i_ts = 0;
		while (*(topicName + i_tm) != '/')
		{
			rcvTopic.module[i_ts] = *(topicName + i_tm);
			i_tm++;
			i_ts++;
		}
		i_tm++;
		i_ts = 0;
		while (*(topicName + i_tm) != '/')
		{
			rcvTopic.location[i_ts] = *(topicName + i_tm);
			i_tm++;
			i_ts = 0;
		}
		i_tm++;
		i_ts = 0;
		while (*(topicName + i_tm) != '\0')
		{
			rcvTopic.canid[i_ts] = *(topicName + i_tm);
			i_tm++;
			i_ts++;
		}

		// --- Use information of dissected topic to apply correct signal mapping --- //
		printf("\n\nData representation as specified in XML configuration file:\n");
		printf("Looking up signal mapping...\n");
		for (i_dev = 0; i_dev < 1; i_dev++) // Currently only 1 device per xml file is supported
		{
			for (i_mod = 0; i_mod < device->cntmod; i_mod++)
			{
				for (i_frm = 0; i_frm < device->modules[i_mod].cntfrm; i_frm++)
				{
					if (device->modules[i_mod].frames[i_frm].canid == atoi(rcvTopic.canid))
					{
						goto stop;
					}
				}
			}
		}
	stop: printf("Applying mapping of frame with ID %s: %u signals\n", rcvTopic.canid, device->modules[i_mod].frames[i_frm].cntsig);
		msgrcvdptr = (uint8_t*)message->payload;
		for (size_t i_sig = 0; i_sig < device->modules[i_mod].frames[i_frm].cntsig; i_sig++)
		{
			switch (device->modules[i_mod].frames[i_frm].signals[i_sig].len)
			{
			case 8:
				dataptr_u8 = msgrcvdptr;
				// Print data and increment pointer
				printf("\tSignal %u data: %u\n", i_sig + 1, *dataptr_u8++);
				msgrcvdptr = dataptr_u8;
				break;
			case 16:
				dataptr_u16 = (uint16_t*)msgrcvdptr;
				// Print data and increment pointer
				printf("\tSignal %u data: %u\n", i_sig + 1, *dataptr_u16++);
				msgrcvdptr = (uint8_t*)dataptr_u16;
				break;
			case 32:
				dataptr_u32 = (uint32_t*)msgrcvdptr;
				// Print data and increment pointer
				printf("\tSignal %u data: %u\n", i_sig + 1, *dataptr_u32++);
				msgrcvdptr = (uint8_t*)dataptr_u32;
				break;
			case 64:
				if (i_sig < 1) // uint64_t = 8 byte -> frame has only 1 signal (device->modules[i_mod].frames[i_frm].signals[0])
				{

				}
				dataptr_u64 = (uint64_t*)msgrcvdptr;
				// Print data (no increment because CAN data is 64bit at max)
				printf("\tSignal %u data: %llu\n", i_sig + 1, *dataptr_u64);
				break;
			default:
				printf("main(): Unsupported signal length");
				break;
			}
		}
	}
	else
	{
		config_rcvd = (char*)malloc(msgrcvdlen);
		strcpy(config_rcvd, msgrcvdptr);
		config_update = true;
	}
	
	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);

	return msgrcvdlen;
}
void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded, topic: %s\n", (char*)context);
}
void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response ? response->code : 0);
}
