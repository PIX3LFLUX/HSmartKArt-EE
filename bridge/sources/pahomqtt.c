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

	if (strstr(topicName, "/config") == NULL)
	{
		printf("Message Content: ");
		for (size_t i = 0; i < msgrcvdlen; i++)
		{
			printf("%u ", *msgrcvdptr++);
		}
		printf("\n");
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
