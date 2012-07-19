/*
Client for interacting with TemperMe.com hardware
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <usb.h>				// this is libusb, see http://libusb.sourceforge.net/
#include <syslog.h>
#include "opendevice.h"

void free_dev_serial(char **serial)
{
	int i;
	for(i = 0; serial[i] != NULL; ++i) {
		free(serial[i]);
	}
	free(serial);
}

char** list_dev_serial(int vendorID, int productID)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	usb_dev_handle *handle = NULL;
	char **serials = NULL;
	int nserials = 0;

	if ((serials = malloc(sizeof(void*))) == NULL) {
		syslog(LOG_ERR, "memory allocation fail!\n");
		exit(EXIT_FAILURE);
	}
	serials[0] = NULL;
	usb_find_busses();
	usb_find_devices();
	for(bus = usb_get_busses(); bus; bus = bus->next) {
		for(dev = bus->devices; dev; dev = dev->next) {  /* iterate over all devices on all busses */
			if(dev->descriptor.idVendor == vendorID
						&& dev->descriptor.idProduct == productID) {
				char serial[256];
				int len;
				handle = usb_open(dev); /* we need to open the device in order to query strings */
				if (!handle) {
					syslog(LOG_WARNING, "Warning: cannot open VID=0x%04x PID=0x%04x: %s\n", dev->descriptor.idVendor, dev->descriptor.idProduct, usb_strerror());
					continue;
				}
				len = 0;
				serial[0] = 0;
				if(dev->descriptor.iSerialNumber > 0){
					len = usbGetStringAscii(handle, dev->descriptor.iSerialNumber, serial, sizeof(serial));
				}
				if(len < 0) {
					syslog(LOG_WARNING, "Warning: cannot query manufacturer for VID=0x%04x PID=0x%04x: %s\n", dev->descriptor.idVendor, dev->descriptor.idProduct, usb_strerror());
				} else if(len > 0) {
					nserials++;
					if ((serials = realloc(serials, (nserials+1) * sizeof(void*))) == NULL ) {
						syslog(LOG_ERR, "memory allocation fail!\n");
						exit(EXIT_FAILURE);
					}
					serials[nserials-1] = malloc(len+1);
					strcpy(serials[nserials-1], serial);
					serials[nserials] = NULL;
				} else {
					syslog(LOG_WARNING, "Warning: device VID=0x%04x PID=0x%04x has no serial number.\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
				}
			} else {
			}
		}
	}
	return serials;
}

/* Example:
int main( int ac, char *av[] ) {
	char **serials;
	int i;

	serials = list_dev_serial(VENDOR_ID, PRODUCT_ID);
	printf("Found devices: ");
	for(i = 0; serials[i] != NULL; ++i) {
		printf("(%s)", serials[i]);
	}
	printf("\n");
	free_dev_serial(serials);
	return EXIT_SUCCESS;
}
*/
// vim: noet:sw=4:ts=4:sts=0
