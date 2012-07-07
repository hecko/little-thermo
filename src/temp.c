/*
	Created: December 2011
	by ihsan Kehribar <ihsan@kehribar.me>
	Changed: July 2012
	by Marcel Hecko <maco@blava.net>
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

unsigned char version;

void signal_handler(int sig) {
	switch(sig) {

		case SIGTERM:
			syslog(LOG_NOTICE,"received a SIGTERM signal");
			exit(0);
			break;
		case SIGINT:
			syslog(LOG_NOTICE, "received a SIGINT signal");
			exit(0);
			break;
	}
}

//sender part which regularly sends data to the cloud 
void* Sender(void *arg) {

	littleWire *myLittleWire = NULL;
        unsigned int adcValue;

        myLittleWire = littleWire_connect();

        if(myLittleWire == NULL){
                syslog(LOG_ERR, "Little Wire could not be found! Make sure you are running this as root!\n");
                exit(EXIT_FAILURE);
        }

        version = readFirmwareVersion(myLittleWire);
        syslog(LOG_ERR, "Little Wire firmware version: %d.%d\n",((version & 0xF0)>>4),(version&0x0F));

        pinMode(myLittleWire,PIN2,INPUT);

        CURL *curl_handle;
        CURLcode res;
        curl_handle = curl_easy_init();
        char *curl_data;

        while(1){
                adcValue=analogRead(myLittleWire, ADC_TEMP_SENS);
                if (curl_handle) {
                        //prepare post data
                        asprintf(&curl_data, "%s%f", "user=maco&key=temp&val=", (float)((0.888*adcValue)-235.8));
                        //set URL and POST data
                        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, curl_data);
                        curl_easy_setopt(curl_handle, CURLOPT_URL, "http://linode.blava.net/meter/");
                        //set no progress meter
                        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
                        res = curl_easy_perform(curl_handle);
                        if(res != CURLE_OK) {
                                syslog(LOG_NOTICE, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
                        }
                }
                syslog(LOG_INFO, "%s\n", curl_data);
                free(curl_data);

                delay(60000);
        }
	curl_easy_cleanup(curl_handle);
}

void* Server(void *arg) {
	#define LOCAL_SERVER_PORT 1500
	#define MAX_MSG 1 

	syslog(LOG_NOTICE, "Starting server code...");

	int sd, rc, n, cliLen;
	struct sockaddr_in cliAddr, servAddr;
	char msg[MAX_MSG];
	sd=socket(AF_INET, SOCK_DGRAM, 0);
	if(sd<0) {
		syslog(LOG_NOTICE, "cannot open socket \n");
		exit(1);
	}
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(LOCAL_SERVER_PORT);
	rc = bind (sd, (struct sockaddr *) &servAddr,sizeof(servAddr));

	if(rc<0) {
		syslog(LOG_NOTICE, "cannot bind port number %d \n", LOCAL_SERVER_PORT);
		exit(1);
	}
	syslog(LOG_NOTICE, "waiting for data on port UDP %u\n", LOCAL_SERVER_PORT);

	/* server infinite loop */
	while(1) {
		/* init buffer */
		memset(msg,0x0,MAX_MSG);
		/* receive message */
		cliLen = sizeof(cliAddr);
		n = recvfrom(sd, msg, MAX_MSG, 0, (struct sockaddr *) &cliAddr, &cliLen);
		if(n<0) {
			syslog(LOG_NOTICE, "cannot receive data \n");
			continue;
		}
		/* print received message */
		//syslog(LOG_NOTICE, "from %u UDP%u : %s \n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port),msg);

		sendto(sd, "anoo", sizeof("anoo"), 0, (struct sockaddr *) &cliAddr, sizeof(cliAddr));

	}/* end of server infinite loop */

	return 0;

	syslog(LOG_NOTICE, "Server code ending...");
}

int main(int argc, char **argv)
{
	openlog(argv[0],LOG_NOWAIT|LOG_PID,LOG_USER);
	//disable stdout buffering
	setbuf(stdout, NULL);
	syslog(LOG_NOTICE, "----------Starting temperatire monitoring server----------");

	if (argc < 2) {
		syslog(LOG_ERR,"Please run the program with username: %s <username>\n",argv[0]);
		return -1; 
	}

	signal(SIGTERM,signal_handler); /* catch kill signal */
	signal(SIGINT,signal_handler); /* catch kill signal */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

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
