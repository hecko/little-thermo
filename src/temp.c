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
#include <netdb.h>
#include <sys/un.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define SEND_INTERVAL 5000

unsigned char version;
littleWire *myLittleWire = NULL;
char *conf_user;

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

void InitTemp(void) {

        myLittleWire = littleWire_connect();

        if(myLittleWire == NULL){
                syslog(LOG_ERR, "Little Wire could not be found! Make sure you are running this as root!\n");
                exit(EXIT_FAILURE);
        }

        version = readFirmwareVersion(myLittleWire);
        syslog(LOG_ERR, "Little Wire firmware version: %d.%d\n",((version & 0xF0)>>4),(version&0x0F));

        pinMode(myLittleWire,PIN2,INPUT);
}

float ReadTemp(void) {
    unsigned int adcValue;
	adcValue=analogRead(myLittleWire, ADC_TEMP_SENS);
	return (float)((0.888*adcValue)-235.8);
}

//sender part which regularly sends data to the cloud 
void* Sender(void *arg) {

        CURL *curl_handle;
        CURLcode res;
        curl_handle = curl_easy_init();
        char *curl_data;

        while(1){
		float temp_c;
		temp_c = ReadTemp();
                if (curl_handle) {
                        //prepare post data
                        asprintf(&curl_data, "user=%s&key=temp&val=%f", conf_user, temp_c);
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
                //free(curl_data);

                delay(SEND_INTERVAL);
        }
	curl_easy_cleanup(curl_handle);
}

void *servlet(void *arg)                    /* servlet thread */
{	FILE *fp = (FILE*)arg;            /* get & convert the data */
	float temp_c;

	temp_c = ReadTemp();
	fprintf(fp, "%f", temp_c);                             /* echo it back */
	fclose(fp);                   /* close the client's channel */
	return 0;                           /* terminate the thread */
}

void* Server(void *arg) {
	#define LOCAL_SERVER_PORT 15000

	syslog(LOG_NOTICE, "Starting server code...");

	int sd, port;
	struct sockaddr_in addr;
    int optval;

	memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LOCAL_SERVER_PORT);

	sd=socket(AF_INET, SOCK_STREAM, 0);
	if(sd<0) {
		syslog(LOG_NOTICE, "cannot open socket \n");
		exit(1);
	}
	optval = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

	if ( bind(sd, (struct sockaddr*)&addr, sizeof(addr) ) != 0 )
		syslog(LOG_NOTICE, "problem with server bind");

	if ( listen(sd, 10) != 0 ) {
		perror("listen");
		syslog(LOG_NOTICE, "problem with server listener");	
		exit(1);
	} else {
        int as;
		pthread_t child;
		FILE *fp;

		while (1)                         /* process all incoming clients */
		{
			     /* accept connection */
	        if ( (as = accept(sd, 0, 0)) == -1 ) {
        		perror("accept");
        		syslog(LOG_NOTICE, "problem with socket accept");	
        		exit(1);
            }
			fp = fdopen(as, "r+");           /* convert into FILE* */
			pthread_create(&child, 0, servlet, fp);       /* start thread */
			pthread_detach(child);                      /* don't track it */
		}
	}

	syslog(LOG_NOTICE, "Server code ending...");
}

int main(int argc, char **argv)
{
	openlog(argv[0],LOG_NOWAIT|LOG_PID,LOG_USER);
	//disable stdout buffering
	setbuf(stdout, NULL);
	syslog(LOG_NOTICE, "----------Starting temperatire monitoring server----------");

	if (argc != 2) {
		syslog(LOG_ERR,"Please run the program with username: %s <username>\n",argv[0]);
		return -1; 
	}
    conf_user = argv[1];
    InitTemp();

	signal(SIGTERM,signal_handler); /* catch kill signal */
	signal(SIGINT,signal_handler); /* catch kill signal */
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
