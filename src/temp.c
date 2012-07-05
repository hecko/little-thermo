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

unsigned char version;

int main(int argc, char **argv)
{

	if (argc < 2) {
		printf("Please run the program with username: %s <username>\n",argv[0]);
		return -1; 
	}

	littleWire *myLittleWire = NULL;
	unsigned int adcValue;

	myLittleWire = littleWire_connect();

	if(myLittleWire == NULL){
		printf("Little Wire could not be found! Make sure you are running this as root!\n");
		exit(EXIT_FAILURE);
	}
	
	version = readFirmwareVersion(myLittleWire);
	printf("Little Wire firmware version: %d.%d\n",((version & 0xF0)>>4),(version&0x0F));
	
	pinMode(myLittleWire,PIN2,INPUT);

	CURL *curl_handle;
	CURLcode res;
	curl_handle = curl_easy_init();
	char *curl_data;

	while(1){
		adcValue=analogRead(myLittleWire, ADC_TEMP_SENS);
		if (curl_handle) {
			//prepare post data
			asprintf(&curl_data, "%s%s%s%f", "user=", argv[1], "&key=temp&val=", (float)((adcValue-273.0)*0.92859));
			//set URL and POST data
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, curl_data);
			curl_easy_setopt(curl_handle, CURLOPT_URL, "http://linode.blava.net/meter/");
			//set no progress meter
			curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
			res = curl_easy_perform(curl_handle);
			if(res != CURLE_OK) {
				fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
			}
		}
		printf("%s\n", curl_data);
		free(curl_data);
		delay(1000);
	}

	curl_easy_cleanup(curl_handle);
}
