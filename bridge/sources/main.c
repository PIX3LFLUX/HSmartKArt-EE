/*
* Company: University of Applied Science Karlsruhe
* Author: Bruno Kempf
*
* Project Name: mqttee-xml-bridge
* File Name: main.c
* Create Date: 26.02.2021 13:12:26
*
* Description: MQTT/CAN Bridge
*
* Revision:
*		Revision 0.01 - File Created
*
* Additional Comments:
*/


#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include "pahomqtt.h"
#include "socketcan.h"
#include "xmlconfig.h"




// --- External variables --- //
extern char* config_rcvd;
extern xmldevice_t* device;
extern bool config_update;

extern bool disconnected;
extern bool connected;

// --- Global variables --- //
char default_conf[] = "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?><xml><CHECK-STRING>MQTTEE-v0.1</CHECK-STRING><DEVICE-NAME>RPI3BP0</DEVICE-NAME><COUNTED-MODULES>2</COUNTED-MODULES><MODULE><MODULE-NAME>ULTRASONIC</MODULE-NAME><LOCATION>FRONT</LOCATION><COUNTED-FRAMES>1</COUNTED-FRAMES><FRAME><IDENTIFIER>64</IDENTIFIER><COUNTED-SIGNALS>5</COUNTED-SIGNALS><SIGNAL><SIGNAL-NAME>BYTE_1</SIGNAL-NAME><SIG-POS>0</SIG-POS><LENGTH>8</LENGTH></SIGNAL><SIGNAL><SIGNAL-NAME>BYTE_2</SIGNAL-NAME><SIG-POS>8</SIG-POS><LENGTH>8</LENGTH></SIGNAL><SIGNAL><SIGNAL-NAME>BYTE_3_4</SIGNAL-NAME><SIG-POS>16</SIG-POS><LENGTH>16</LENGTH></SIGNAL><SIGNAL><SIGNAL-NAME>BYTE_5_6</SIGNAL-NAME><SIG-POS>32</SIG-POS><LENGTH>16</LENGTH></SIGNAL><SIGNAL><SIGNAL-NAME>BYTE_7_8</SIGNAL-NAME><SIG-POS>48</SIG-POS><LENGTH>16</LENGTH></SIGNAL></FRAME></MODULE><MODULE><MODULE-NAME>ULTRASONIC</MODULE-NAME><LOCATION>REAR</LOCATION><COUNTED-FRAMES>1</COUNTED-FRAMES><FRAME><IDENTIFIER>128</IDENTIFIER><COUNTED-SIGNALS>4</COUNTED-SIGNALS><SIGNAL><SIGNAL-NAME>BYTE_1_2</SIGNAL-NAME><SIG-POS>0</SIG-POS><LENGTH>16</LENGTH></SIGNAL><SIGNAL><SIGNAL-NAME>BYTE_3_4</SIGNAL-NAME><SIG-POS>16</SIG-POS><LENGTH>16</LENGTH></SIGNAL><SIGNAL><SIGNAL-NAME>BYTE_5_6</SIGNAL-NAME><SIG-POS>32</SIG-POS><LENGTH>16</LENGTH></SIGNAL><SIGNAL><SIGNAL-NAME>BYTE_7_8</SIGNAL-NAME><SIG-POS>48</SIG-POS><LENGTH>16</LENGTH></SIGNAL></FRAME></MODULE></xml>";

int rc_mqtt = 0;
MQTTAsync client_handle;

int sktcan = 0;
struct can_frame canfrmrx;
//canfrmtx.can_id = 0x8C000000; // 8 = Extended Frame Format (bit 31), C000000 = Frame ID (bit 0-28)
ssize_t nbytesread = 0;

struct timespec delay = { .tv_sec = 0, .tv_nsec = 10000000 }; // 10ms


/* Signal Handler for SIGINT */
void sigint_handler(int sig_num)
{
	/* Reset handler to catch SIGINT next time.
	   Refer http://en.cppreference.com/w/c/program/signal */
	printf("\nSignal handler for Ctrl+C\n");

	MQTTAsync_disconnectOptions disconn_opts = MQTTAsync_disconnectOptions_initializer;
	disconn_opts.onSuccess = onDisconnect;
	disconn_opts.context = client_handle;
	if ((rc_mqtt = MQTTAsync_disconnect(client_handle, &disconn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Disconnection failed, return code %d\n", rc_mqtt);
		exit(EXIT_FAILURE);
	}
	while (!disconnected)
	{
		if (clock_nanosleep(CLOCK_MONOTONIC, 0, &delay, NULL)) // delay = 10ms
		{
			perror("clock_nanosleep");
			exit(EXIT_FAILURE);
		}
	}
	MQTTAsync_destroy(&client_handle);

	/* Do a graceful cleanup of the program like:
	free memory/resources/etc and exit */
	exit(0);
}


int main(int argc, char* argv[])
{
	// -------------------------------------- //
	// --- Process command line arguments --- //
	// -------------------------------------- //
	char* broker = "broker.hivemq.com";
	char* subscribetopic = "MQTTBridges";
	bool usecan = false;
	int c;
	/*
	* If the value of opterr variable is nonzero, then getopt prints an error
	* message to the standard error stream if it encounters an unknown option
	* character or an option with a missing required argument. This is the
	* default behavior. If you set this variable to zero, getopt does not print
	* any messages, but it still returns the character ? to indicate an error.
	*/
	//opterr = 0;
	/*
	* An option character in this string can be followed by a colon (:) to
	* indicate that it takes a argument.
	*/
	while ((c = getopt(argc, argv, "c:b:s:h")) != -1)
	{
		switch (c)
		{
		case 'c':
			if (strcmp(optarg, "true"))
				usecan = false;
			else
				usecan = true;
			break;
		case 'b':
			broker = optarg;
			break;
		case 's':
			subscribetopic = optarg;
			break;
		case 'h':
			printf("Following command options are supported: (Default values are in brackets [<default_value>])\n");
			printf("-c CAN interface support:\n\tTakes true or false as argument. When a CAN interface is supported set to true.\n\t[false]\n");
			printf("-b Domain or IP of the broker:\n\tTakes a domain or an IP to connect to a broker.\n\t[broker.hivemq.com]\n");
			printf("-s Subscribe topic:\n\tAdditionally to the config topic the MQTT Bridge will subscribe on this topic.\n\t[MQTTBridges]\n");
			printf("-h Show this help\n");
			exit(EXIT_SUCCESS);
		case '?':
			/*if (optopt == 'c')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else */if (isprint(optopt))
				fprintf(stderr, "Unknown option '-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
			return 1;
		default:
			exit(EXIT_FAILURE); // See https://stackoverflow.com/questions/397075/what-is-the-difference-between-exit-and-abort
		}
	}

	for (ssize_t i = optind; i < argc; i++)
	{
		printf("Non-option argument %s\n", argv[i]);
	}

	

	// ---------------------- //
	// --- Initialization --- //
	// ---------------------- //
	config_rcvd = NULL;
	device = NULL;
	config_update = false;

	disconnected = false;
	connected = false;
	
	char* clientid = getenv("MQTT_CLIENTID");

	uint8_t* dataptr = NULL;
	uint8_t* dataptr_u8 = NULL;
	uint16_t* dataptr_u16 = NULL;
	uint32_t* dataptr_u32 = NULL;
	uint64_t* dataptr_u64 = NULL;

	long sktflags = 0;

	// --- Setup Signal Handler for CTRL+C --- //
	signal(SIGINT, sigint_handler);

	// --- Apply default Configuration --- //
	printf("Applying default configuration...\n");
	config_rcvd = (char*)malloc(sizeof(default_conf));
	strcpy(config_rcvd, default_conf);
	if (xml_check() == EXIT_SUCCESS)
	{
		xml_setup();
		xml_config_read();
		xml_create_topics();
	}
	free(config_rcvd);
	printf("Default configuration applied\nProceeding with Setup for SocketCAN and MQTT\n");



	// ----------------------- //
	// --- SocketCAN Setup --- //
	// ----------------------- //
	int rc_skt;
	struct sockaddr_can addr;
	struct ifreq ifr;

	const char* ifname = "can0"; // Bus speed is defined in /etc/rc.local

	if (usecan)
	{
		if ((sktcan = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
			perror("Error while opening socket!");
			return sktcan;
		}

		strcpy(ifr.ifr_ifrn.ifrn_name, ifname);
		ioctl(sktcan, SIOCGIFINDEX, &ifr);

		addr.can_family = AF_CAN;
		addr.can_ifindex = ifr.ifr_ifru.ifru_ivalue;

		printf("%s at index %d\n", ifr.ifr_name, ifr.ifr_ifindex); // ifr.ifr_name und ifr.ifr_ifindex are defines, see net/if.h

		if ((rc_skt = bind(sktcan, (struct sockaddr*)&addr, sizeof(addr))) < 0) {
			perror("Error in socket bind!");
			return rc_skt;
		}

		// --- Make SocketCAN read non-blocking --- //
		sktflags = fcntl(sktcan, F_GETFL);
		sktflags |= O_NONBLOCK;
		fcntl(sktcan, F_SETFL, sktflags);
	}



	// ------------------ //
	// --- MQTT Setup --- //
	// ------------------ //
	disconnected = false;
	connected = false;

	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_disconnectOptions disconn_opts = MQTTAsync_disconnectOptions_initializer;
	MQTTAsync_responseOptions sub_opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_responseOptions pub_opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

	MQTTAsync_create(&client_handle, broker, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	MQTTAsync_setCallbacks(client_handle, NULL, connlost, msgrcvdCallback, NULL);

	conn_opts.keepAliveInterval = 120;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = clientid;
	if ((rc_mqtt = MQTTAsync_connect(client_handle, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc_mqtt);
		exit(EXIT_FAILURE);
	}
	while (!connected)
	{
		if (clock_nanosleep(CLOCK_MONOTONIC, 0, &delay, NULL))
		{
			perror("clock_nanosleep");
			exit(EXIT_FAILURE);
		}
	}

	// --- MQTT Subscribe (MQTT data) --- //
	sub_opts.onSuccess = onSubscribe;
	sub_opts.onFailure = onSubscribeFailure;
	sub_opts.context = subscribetopic;
	if ((rc_mqtt = MQTTAsync_subscribe(client_handle, subscribetopic, QOS, &sub_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc_mqtt);
		exit(EXIT_FAILURE);
	}
	// --- MQTT Subscribe (configuration) --- //
	sub_opts.context = device->topic_config;
	if ((rc_mqtt = MQTTAsync_subscribe(client_handle, device->topic_config, QOS, &sub_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc_mqtt);
		exit(EXIT_FAILURE);
	}
	// --- MQTT Publish (CAN data) --- //
	pub_opts.onSuccess = onPublish;
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	pubmsg.payloadlen = 8;





	// ------------------ //
	// --- Super Loop --- //
	// ------------------ //
	while (true)
	{
		// --- Check for configuration updates --- //
		if (config_update)
		{
			if (xml_check())
			{
				if ((rc_mqtt = MQTTAsync_unsubscribe(client_handle, device->topic_config, &sub_opts)) != MQTTASYNC_SUCCESS)
				{
					printf("Failed to start unsubscribe, return code %d\n", rc_mqtt);
					exit(EXIT_FAILURE);
				}
				for (size_t i_mod = 0; i_mod < device->cntmod; i_mod++)
				{
					if ((rc_mqtt = MQTTAsync_unsubscribe(client_handle, device->modules[i_mod].topic_can, &sub_opts)) != MQTTASYNC_SUCCESS)
					{
						printf("Failed to start unsubscribe, return code %d\n", rc_mqtt);
						exit(EXIT_FAILURE);
					}
				}
				free(device);
				xml_setup();
				xml_config_read();
				xml_create_topics();
				if ((rc_mqtt = MQTTAsync_subscribe(client_handle, device->topic_config, QOS, &sub_opts)) != MQTTASYNC_SUCCESS)
				{
					printf("Failed to start subscribe, return code %d\n", rc_mqtt);
					exit(EXIT_FAILURE);
				}
				for (size_t i_mod = 0; i_mod < device->cntmod; i_mod++)
				{
					if ((rc_mqtt = MQTTAsync_subscribe(client_handle, device->modules[i_mod].topic_can, QOS, &sub_opts)) != MQTTASYNC_SUCCESS)
					{
						printf("Failed to start unsubscribe, return code %d\n", rc_mqtt);
						exit(EXIT_FAILURE);
					}
				}
			}
			free(config_rcvd); // memory for config_rcvd is allocated in the msgrcvdCallback() function, see pahomqtt.c
			config_update = false;
			printf("Configuration applied\nProceeding with normal operation...\n");
		}

		if (usecan)
		{
			// --- Read CAN frame --- //
			/*
			* 1. BLOCKING MODE: read() function blocks until CAN frame is available.
			* 2. NON-BLOCKING MODE: if no CAN frame is available check for configuration
			*					updates. After that check again for available CAN
			*					frames.
			* No CAN ID filter: all CAN frames are received.
			* NOTE: Receiving MQTT messages is handled asynchronously (callback)
			*	--> Processing MQTT messages is not restricted in blocking mode
			*/
			if ((nbytesread = read(sktcan, &canfrmrx, sizeof(struct can_frame))) < 0)
			{
				if (errno == EAGAIN)
					continue;
				else
				{
					perror("CAN RAW socket read");
					exit(EXIT_FAILURE);
				}
			}

			// --- Transmit CAN frame using MQTT --- //
			pubmsg.payload = canfrmrx.data;
			printf("\n\nPublishing CAN frame with ID %u", canfrmrx.can_id);
			for (size_t i_mod = 0; i_mod < device->cntmod; i_mod++)
			{
				for (size_t i_frm = 0; i_frm < device->modules[i_mod].cntfrm; i_frm++)
				{
					if (canfrmrx.can_id == device->modules[i_mod].frames[i_frm].canid)
					{
						printf(" on topic\n\t%s\n", device->modules[i_mod].topic_can);
						if ((rc_mqtt = MQTTAsync_sendMessage(client_handle, device->modules[i_mod].topic_can, &pubmsg, &pub_opts)) != MQTTASYNC_SUCCESS)
						{
							printf("Failed to start sendMessage, return code %d\n", rc_mqtt);
							exit(EXIT_FAILURE);
						}

						// --- Unpack CAN frame --- //
						printf("Data representation as specified in XML configuration file:\n");
						dataptr = canfrmrx.data;
						for (size_t i_sig = 0; i_sig < device->modules[i_mod].frames[i_frm].cntsig; i_sig++)
						{
							switch (device->modules[i_mod].frames[i_frm].signals[i_sig].len)
							{
							case 8:
								dataptr_u8 = dataptr;
								// Print data and increment pointer
								printf("\tSignal %u data: %u\n", i_sig + 1, *dataptr_u8++);
								dataptr = dataptr_u8;
								break;
							case 16:
								dataptr_u16 = (uint16_t*)dataptr;
								// Print data and increment pointer
								printf("\tSignal %u data: %u\n", i_sig + 1, *dataptr_u16++);
								dataptr = (uint8_t*)dataptr_u16;
								break;
							case 32:
								dataptr_u32 = (uint32_t*)dataptr;
								// Print data and increment pointer
								printf("\tSignal %u data: %u\n", i_sig + 1, *dataptr_u32++);
								dataptr = (uint8_t*)dataptr_u32;
								break;
							case 64:
								dataptr_u64 = (uint64_t*)dataptr;
								// Print data (no increment because CAN data is 64bit at max)
								printf("\tSignal %u data: %llu\n", i_sig + 1, *dataptr_u64);
								break;
							default:
								printf("main(): Unsupported signal length");
								break;
							}
						} // for i_sig
					}
				} // for i_frm
			} // for i_mod
		}
	}





	// --- MQTT Clean Up --- //
	disconn_opts.onSuccess = onDisconnect;
	disconn_opts.context = client_handle;
	if ((rc_mqtt = MQTTAsync_disconnect(client_handle, &disconn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Disconnection failed, return code %d\n", rc_mqtt);
		exit(EXIT_FAILURE);
	}
	while (!disconnected)
	{
		if (clock_nanosleep(CLOCK_MONOTONIC, 0, &delay, NULL)) // delay = 10ms
		{
			perror("clock_nanosleep");
			exit(EXIT_FAILURE);
		}
	}
	MQTTAsync_destroy(&client_handle);


	// --- SocketCAN Clean Up --- //
	close(sktcan);

	return 0;
}

