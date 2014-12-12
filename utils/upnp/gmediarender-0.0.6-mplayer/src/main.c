/* main.c - Main program routines
 *
 * Copyright (C) 2005-2007   Ivo Clarysse
 *
 * This file is part of GMediaRender.
 *
 * GMediaRender is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GMediaRender is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMediaRender; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>

//#include <glib.h>

#include "../../libupnp/include/ithread.h"
#include "../../libupnp/include/upnp.h"

#include "logging.h"
#include "output_gstreamer.h"
#include "upnp.h"
#include "upnp_device.h"
#include "upnp_renderer.h"

#include <sys/ioctl.h>

static int show_version = FALSE;
static int show_help = FALSE;
static int show_devicedesc = FALSE;
static int show_connmgr_scpd = FALSE;
static int show_control_scpd = FALSE;
static int show_transport_scpd = FALSE;
static int show_queue_scpd = FALSE;
static char *ip_address = NULL;
static char *uuid = "GMediaRender-1_0-000-000-002";
static char *friendly_name = PACKAGE_NAME;
char my_uuid[128];
char my_friendly_name[128];


static void do_show_version(void)
{
	puts( PACKAGE_STRING "\n"
        	"This is free software. "
		"You may redistribute copies of it under the terms of\n"
		"the GNU General Public License "
		"<http://www.gnu.org/licenses/gpl.html>.\n"
		"There is NO WARRANTY, to the extent permitted by law."
	);
}
static void do_show_help(void)
{
    printf("Command format : gmediarender [option]\n");
    printf("--version : display version number.\n");
    printf("--help : display command help.\n");
    printf("--ip-address : assign IP address for this application.\n");
    printf("\t --ip-address xxx.xxx.xxx.xxx\n");
    printf("--uuid : assign uuid for this application.\n");
    printf("\t --uuid xxxxxxxxxxxx\n");
    printf("--friendly-name : assign friendly name for this application.\n");
    printf("\t --friendly-name xxxxxxxxxxxx\n");
    printf("--conf : assign configuration file for configuration\n");
    printf("\t -conf filename\n");
    printf("--dump-devicedesc : dump device descriptor XML and exit.\n");
    printf("--dump-connmgr-scpd : dump Connection Manager service description XML and exit.\n");
    printf("--dump-control-scpd : dump Rendering Control service description XML and exit.\n");
    printf("--dump-transport-scpd : Dump A/V Transport service description XML and exit.\n");
    printf("--dump-queue-scpd : Dump Queue service description XML and exit.\n");
    printf("\n");

}
static int process_cmdline(int argc, char **argv)
{
	int count = argc;
    int i;
    FILE *pFile;
    char buff[256];
    char *vPtr;


    if(count == 1)
        return 0;

    count--;

    for(i=1;i<=count;)
    {
        if(!strcmp("--version", argv[i]))
        {
            show_version = TRUE;
            return 0;
        }
        else if(!strcmp("--help", argv[i]))
        {
            show_help = TRUE;
            return 0;
        }
        else if(!strcmp("--ip-address", argv[i]))
        {
            ip_address = (char *)malloc(strlen(argv[i+1]));
            if(!ip_address)
                return -1;
            strcpy(ip_address, argv[i+1]);
            i+=2;
        }
        else if(!strcmp("--uuid", argv[i]))
        {
            strcpy(my_uuid, argv[i+1]);
            i+=2;
        }
        else if(!strcmp("--friendly-name", argv[i]))
        {
            strcpy(my_friendly_name, argv[i+1]);
            i+=2;
        }
        else if(!strcmp("--conf", argv[i]))
        {
            pFile = fopen(argv[i+1], "r");
            if(pFile)
            {
                while(!feof(pFile))
                {
                    memset(buff, 0x0, sizeof(buff));
                    fgets(buff,sizeof(buff),pFile);

                    if( buff[0] == 0 )
                        break;

                    vPtr=strchr(buff,0x0a);
                    if(vPtr)
                        *vPtr = 0;  // extract the line feed
                    else
                        break;      // No line feed, bad line.

                    vPtr = strchr(buff,'=');

                    /*
                    ** If this string doesn't have an "=" inserted, it's a malformed
                    ** string and we should just terminate.  If it does, Replace the
                    ** equal sign with a null to seperate the name and value strings,
                    ** so they can be copied directly
                    */

                    if(!vPtr)
                        break;
                    else
                        *vPtr++ = 0;


                    /*
                    ** Insert into the local structure
                    */
                    if(strstr(buff, "ip-address"))
                    {
                        ip_address = (char *)malloc(strlen(argv[i+1]));
                        if(!ip_address)
                            return -1;
                        strcpy(ip_address, vPtr);
                    }
                    else if(strstr(buff, "uuid"))
                    {
                        strcpy(my_uuid, vPtr);
                    }
                    else if(strstr(buff, "friendly-name"))
                    {
                        strcpy(my_friendly_name, vPtr);
                    }
                }
                fclose(pFile);
            }
			else
			{
				printf("open configuration file error\n");
			}
			i+=2;
			}
			else if(!strcmp("--dump-devicedesc", argv[i]))
			{
				show_devicedesc = TRUE;
				return 0;
			}
			else if(!strcmp("--dump-connmgr-scpd", argv[i]))
			{
				show_connmgr_scpd = TRUE;
				return 0;
			}
			else if(!strcmp("--dump-control-scpd", argv[i]))
			{
				show_control_scpd = TRUE;
				return 0;
			}
			else if(!strcmp("--dump-transport-scpd", argv[i]))
			{
				show_transport_scpd = TRUE;
				return 0;
			}
			else if(!strcmp("--dump-queue-scpd", argv[i]))
			{
				show_queue_scpd = TRUE;
				return 0;
			}
			else
			{
				return -1;
			}
		}
		return 0;

}

int main(int argc, char **argv)
{
	int rc;
	int result = EXIT_FAILURE;
	struct device *upnp_renderer;

    memset(my_uuid, 0x0, sizeof(my_uuid));
    memset(my_friendly_name, 0x0, sizeof(my_friendly_name));

	rc = process_cmdline(argc, argv);
	if (rc != 0) {
		goto out;
	}

	if (show_version) {
		do_show_version();
		exit(EXIT_SUCCESS);
	}
	if (show_help) {
		do_show_help();
		exit(EXIT_SUCCESS);
	}
	if (show_connmgr_scpd) {
		upnp_renderer_dump_connmgr_scpd();
		exit(EXIT_SUCCESS);
	}
	if (show_control_scpd) {
		upnp_renderer_dump_control_scpd();
		exit(EXIT_SUCCESS);
	}
	if (show_transport_scpd) {
		upnp_renderer_dump_transport_scpd();
		exit(EXIT_SUCCESS);
	}

    if(strlen(my_friendly_name) == 0)
    {
        strcpy(my_friendly_name, friendly_name);
    }
    if(strlen(my_uuid) == 0)
    {
        strcpy(my_uuid, uuid);
    }

	upnp_renderer = upnp_renderer_new(my_friendly_name, my_uuid);
	if (upnp_renderer == NULL) {
		goto out;
	}

	if (show_devicedesc) {
		fputs(upnp_get_device_desc(upnp_renderer), stdout);
		exit(EXIT_SUCCESS);
	}

	rc = output_gstreamer_init();
	if (rc != 0) {
		goto out;
	}

	rc = upnp_device_init(upnp_renderer, ip_address);
	if (rc != 0) {
		goto out;
	}

	printf("Ready for rendering..\n");
	output_loop();
	result = EXIT_SUCCESS;

out:
	return result;
}
