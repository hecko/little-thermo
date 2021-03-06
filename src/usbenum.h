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

void init_usb();
void free_dev_serial(char **serial);
char** list_dev_serial(int vendorID, int productID);

// vim: noet:sw=4:ts=4:sts=0
