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

                delay(5000);
        }
	curl_easy_cleanup(curl_handle);
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
		pthread_t sender_child;
		pthread_create(&sender_child, NULL, Sender, NULL);
		pthread_join(sender_child, NULL);
		closelog();
		pthread_exit(NULL);
	} else {
		syslog(LOG_NOTICE, "Parent: Fork success.");
	}

	//syslog(LOG_NOTICE, "Starting server code...");

	//server code here
	/*
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(1111);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
		syslog(LOG_ERR,"ERROR on server binding");
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) 
		error("ERROR on accept");
	*/
	//server code ends

	syslog(LOG_NOTICE, "Parent will now exit.");
}
