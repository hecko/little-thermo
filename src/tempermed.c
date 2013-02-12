/*
Client for interacting with temperme.com hardware
Copyright (C) 2011  Ihsan Kehribar <ihsan@kehribar.me>
Copyright (C) 2012  Marcel Hecko <maco@blava.net> 
Copyright (C) 2012  Michal Belica 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "littleWire.h"
#include "littleWire_util.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <sys/un.h>
#include "usbenum.h"
#include <usb.h>

#define PROG_NAME "tempermed"
#define SEND_INTERVAL 3000
#define LOCAL_SERVER_PORT "15000" /* port number or service name as string */

//uncomment the following to disable querying a hardware sensor,
//instead return a constant temperature
//#define DUMMY_SENSOR 1

unsigned char version;
struct SensorInfo {
	char *serial;
	littleWire *lw;
	struct SensorInfo *next;
};
struct SensorInfo *sensorInfo = NULL;

void signal_handler(int sig)
{
	switch (sig) {
	case SIGTERM:
		syslog(LOG_NOTICE, "received a SIGTERM signal");
		exit(0);
		break;
	case SIGINT:
		syslog(LOG_NOTICE, "received a SIGINT signal");
		exit(0);
		break;
	}
}

void InitTemp(char *serial, littleWire **myLittleWire)
{
	int ret;
#ifdef DUMMY_SENSOR
	return;
#endif
	syslog(LOG_DEBUG, "InitTemp serial %s called\n", serial);
	//myLittleWire = littleWire_connect();
	ret = usbOpenDevice(myLittleWire, VENDOR_ID, "*", PRODUCT_ID, "*", serial, NULL, NULL );

	if (*myLittleWire == NULL || ret != 0) {
		syslog(LOG_ERR, "Little Wire could not be found! Is it plugged in and are you running this as root?\n");
		//exit(EXIT_FAILURE);
	}

	version = readFirmwareVersion(*myLittleWire);
	syslog(LOG_ERR, "Little Wire firmware version: %d.%d\n",((version & 0xF0) >> 4), (version & 0x0F));

	pinMode(*myLittleWire, PIN2, INPUT);
}

void SIDel() {
    struct SensorInfo *si = NULL, *psi;

    while (si = sensorInfo) {
        if (si != NULL) {
            si = si->next;
            syslog(LOG_INFO,"Deleting sensor %s\n",si->serial);
            psi = si;
            free(psi);
        }
    }
    sensorInfo = NULL;
}

void EnumAllTemp()
{
	char **serials;
	struct SensorInfo *newSI = NULL;
	struct SensorInfo *prevSI = sensorInfo;
	int i;

    SIDel();

	serials = list_dev_serial(VENDOR_ID, PRODUCT_ID);
	for (i = 0; serials[i] != NULL; ++i) {
		syslog(LOG_INFO,"Found %s \n",serials[i]);
	}
	syslog(LOG_INFO,"Found %d devices.", i);
	if (i == 0) {
		syslog(LOG_ERR, "Found no devices.\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; serials[i] != NULL; ++i) {
        syslog(LOG_ERR, "Malloc for %s\n",serials[i]);
		newSI = malloc(sizeof(struct SensorInfo));
        if (newSI == NULL) {
            syslog(LOG_ERR, "newSI is NULL! Exitting.");
            exit;
        } else {
            syslog(LOG_ERR, "newSI is allocated!");
        }
		newSI->next = NULL;
		newSI->serial = malloc(strlen(serials[i])+1);
		strcpy(newSI->serial, serials[i]);
		if (prevSI == NULL) {
	        sensorInfo = newSI;
			prevSI = newSI;
		} else {
			prevSI->next = newSI;
		    prevSI = newSI;
		}
	}
	free_dev_serial(serials);
    syslog(LOG_INFO,"EnumAllTemp funcion ended\n");
}

void CloseTemp(littleWire *lw)
{
	usb_close(lw);
	lw = NULL;
}

float ReadTemp(char *serial)
{
	littleWire *lw;
	unsigned int adcValue;
#ifdef DUMMY_SENSOR
	return 3.1415926535897932384626433832;
#endif
	InitTemp(serial, &lw);
	adcValue = analogRead(lw, ADC_TEMP_SENS);
	CloseTemp(lw);
	return (float)((adcValue * 0.1818) - 25.0364);
}

size_t curl_discard_write( char *ptr, size_t size, size_t nmemb, void *userdata)
{
	return size*nmemb;
}

int http_send_temp(CURL *curl_handle, char *serial, float temp_c)
{
	CURLcode res;
	char *curl_data;

	if (curl_handle) {
		//prepare post data
		asprintf(&curl_data, "sn=%s&temp_c=%g", serial, temp_c);
		//set URL and POST data
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, curl_data);
		curl_easy_setopt(curl_handle, CURLOPT_URL, "http://www.temperme.com/call/index.php");
		//set no progress meter
		curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &curl_discard_write);
		res = curl_easy_perform(curl_handle);
		if (res != CURLE_OK) {
			syslog(LOG_NOTICE, "curl_easy_perform() failed: %s\n",
				   curl_easy_strerror(res));
		}
		syslog(LOG_INFO, "temperme: %s\n", curl_data);
		free(curl_data);
	}
}

//sender part which regularly sends data to the cloud 
void *Sender(void *arg)
{
	char **serials;
	CURL *curl_handle;
	int i;

	if(!(curl_handle = curl_easy_init())) {
		syslog(LOG_ERR, "curl_easy_init failed\n");
		return;
	}
	syslog(LOG_INFO,"temperme: entering Sender loop\n");
	while (1) {
		float temp_c;
		struct SensorInfo *pSI = sensorInfo;

		usb_init();
		//EnumAllTemp();
		serials = list_dev_serial(VENDOR_ID, PRODUCT_ID);
		for (i = 0; serials[i] != NULL; ++i) {
			while(pSI != NULL) {
				syslog(LOG_INFO,"temperme: Reading temp from serial %s \n",serials[i]);
				temp_c = ReadTemp(serials[i]);
				http_send_temp(curl_handle, serials[i], temp_c);
				pSI = (*pSI).next;
			}
		}
		free(pSI);

		delay(SEND_INTERVAL);
	}
	free_dev_serial(serials);
	curl_easy_cleanup(curl_handle);
}

//code witch waits for client connection to return the temperature value
void *servlet(void *arg)
{								/* servlet thread */
	FILE *fp = (FILE *) arg;	/* get & convert the data */
	float temp_c;
	struct SensorInfo *pSI = sensorInfo;

	while(pSI != NULL) {
		temp_c = ReadTemp((*pSI).serial);
		fprintf(fp, "%s:%g\n", (*pSI).serial, temp_c);	/* echo it back */
		pSI = (*pSI).next;
	}
	fclose(fp);					/* close the client's channel */
	return 0;					/* terminate the thread */
}

void *Server(void *arg)
{
	int sd, ret;
	struct addrinfo req, *ans;
	int optval;

	syslog(LOG_NOTICE, "Starting server code...");

	bzero(&req, sizeof(req));
	req.ai_family = AF_UNSPEC;
	req.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	req.ai_socktype = SOCK_STREAM;
	req.ai_protocol = 0;

	if ((ret = getaddrinfo(NULL, LOCAL_SERVER_PORT, &req, &ans)) != 0) {
		syslog(LOG_ERR, "getaddrinfo failed code %d\n", ret);
		exit(1);
	}

	if ((sd = socket((*ans).ai_family, (*ans).ai_socktype, (*ans).ai_protocol)) < 0) {
		syslog(LOG_ERR, "cannot create socket \n");
		exit(1);
	}
	optval = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	if (bind(sd, (*ans).ai_addr, (*ans).ai_addrlen) != 0) {
		syslog(LOG_ERR, "problem with server bind");
		exit(1);
	}

	if (listen(sd, 10) != 0) {
		perror("listen");
		syslog(LOG_ERR, "problem with server listener");
		exit(1);
	} else {
		int as;
		pthread_t child;
		FILE *fp;

		while (1) {
			/* process all incoming clients */
			/* accept connection */
			if ((as = accept(sd, 0, 0)) == -1) {
				perror("accept");
				syslog(LOG_ERR, "problem with socket accept");
				exit(1);
			}
			fp = fdopen(as, "r+");	/* convert into FILE* */
			pthread_create(&child, 0, servlet, fp);	/* start thread */
			pthread_detach(child);	/* don't track it */
		}
	}
	syslog(LOG_NOTICE, "Server code ending...");
}

int main(int argc, char **argv)
{
	int i;

	openlog(PROG_NAME, LOG_NOWAIT | LOG_PID, LOG_USER);
	//disable stdout buffering
	setbuf(stdout, NULL);

	//if (argc != 2) {
	//	syslog(LOG_ERR, "Please run the program with serial number: %s <serial>\n",
	//		   argv[0]);
	//	exit(1);
	//}
	//conf_serial = argv[1];

	usb_init();
	EnumAllTemp();

	syslog(LOG_NOTICE, "Starting temperature monitoring server");

	signal(SIGTERM, signal_handler);	/* catch kill signal */
	signal(SIGINT, signal_handler);	/* catch kill signal */
	//close(STDIN_FILENO);
	//close(STDOUT_FILENO);
	//close(STDERR_FILENO);

	pid_t pid, sid;
	syslog(LOG_ERR, "Forking and putting into background...");
	pid = fork();
	if (pid == -1) {
		syslog(LOG_ERR, "Couldn't fork, please contact support@foo.foo");
		exit(EXIT_FAILURE);
	}
	if (pid == 0) {
		syslog(LOG_NOTICE, "Fork success...we are now in child...");
		umask(0);
		sid = setsid();
		if (sid < 0) {
			syslog(LOG_ERR, "Could not create process group for temper\n");
			exit(EXIT_FAILURE);
		}
		if ((chdir("/tmp/")) < 0) {
			syslog(LOG_ERR, "Could not change working directory to /tmp/\n");
			exit(EXIT_FAILURE);
		}
		syslog(LOG_NOTICE, "...now a full-featured child :)");
		pthread_t sender_child, server_child;
		pthread_create(&sender_child, NULL, Sender, NULL);
		pthread_create(&server_child, NULL, Server, NULL);
		pthread_join(sender_child, NULL);
		closelog();
		pthread_exit(NULL);
		// here the program ends
	} else {
		syslog(LOG_NOTICE, "Parent: Fork success.");
	}

	syslog(LOG_NOTICE, "Parent will now exit.");
}
// vim: noet:sw=4:ts=4:sts=0
