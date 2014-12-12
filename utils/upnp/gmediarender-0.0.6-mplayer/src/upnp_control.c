/* upnp_control.c - UPnP RenderingControl routines
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
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include "../../libupnp/include/upnp.h"
#include "../../libupnp/include/ithread.h"
#include "../../libupnp/include/upnptools.h"

#include "webserver.h"
#include "upnp.h"
#include "upnp_control.h"
#include "upnp_device.h"

#define CONTROL_SERVICE "urn:upnp-org:serviceId:RenderingControl"
#define CONTROL_TYPE "urn:schemas-upnp-org:service:RenderingControl:1"
#define CONTROL_SCPD_URL "/upnp/rendercontrolSCPD.xml"
#define CONTROL_CONTROL_URL "/upnp/control/rendercontrol1"
#define CONTROL_EVENT_URL "/upnp/event/rendercontrol1"

void update_volume(void);
extern char my_uuid[128];

typedef enum {
    CONTROL_VAR_AAT_EQCAP,
	CONTROL_VAR_VOLUME,
	CONTROL_VAR_LOUDNESS,
	CONTROL_VAR_AAT_INSTANCE_ID,
	CONTROL_VAR_MUTE,
	CONTROL_VAR_LAST_CHANGE,
	CONTROL_VAR_AAT_CHANNEL,
	CONTROL_VAR_VOLUME_DB,
	CONTROL_VAR_UNKNOWN,
	CONTROL_VAR_COUNT
} control_variable;

typedef enum {
	CONTROL_CMD_GET_LOUDNESS,
	CONTROL_CMD_GET_MUTE,
	CONTROL_CMD_GET_VOL,
	CONTROL_CMD_GET_VOL_DB,
	CONTROL_CMD_GET_VOL_DBRANGE,
	CONTROL_CMD_GET_EQCAP,
	CONTROL_CMD_SET_LOUDNESS,
	CONTROL_CMD_SET_MUTE,
	CONTROL_CMD_SET_VOL,
	CONTROL_CMD_SET_VOL_DB,
	CONTROL_CMD_UNKNOWN,
	CONTROL_CMD_COUNT
} control_cmd;

static struct action control_actions[];

static const char *control_variables[] = {
	[CONTROL_VAR_LAST_CHANGE] = "LastChange",
	[CONTROL_VAR_AAT_CHANNEL] = "A_ARG_TYPE_Channel",
	[CONTROL_VAR_AAT_INSTANCE_ID] = "A_ARG_TYPE_InstanceID",
	[CONTROL_VAR_AAT_EQCAP] = "A_ARG_TYPE_EQCapabilities",
	[CONTROL_VAR_MUTE] = "Mute",
	[CONTROL_VAR_VOLUME] = "Volume",
	[CONTROL_VAR_VOLUME_DB] = "VolumeDB",
	[CONTROL_VAR_LOUDNESS] = "Loudness",
	[CONTROL_VAR_UNKNOWN] = NULL
};

static const char *aat_channels[] =
{
	"Master",
	"LF",
	"RF",
	NULL
};

static struct param_range volume_range = { 0, 100, 1 };
static struct param_range volume_db_range = { -32768, 32767, 0 };

static struct var_meta control_var_meta[] = {
	[CONTROL_VAR_LAST_CHANGE] =		{ SENDEVENT_YES, DATATYPE_STRING, NULL, NULL },
	[CONTROL_VAR_AAT_CHANNEL] =		{ SENDEVENT_NO, DATATYPE_STRING, aat_channels, NULL },
	[CONTROL_VAR_AAT_INSTANCE_ID] =		{ SENDEVENT_NO, DATATYPE_UI4, NULL, NULL },
	[CONTROL_VAR_AAT_EQCAP] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[CONTROL_VAR_MUTE] =			{ SENDEVENT_NO, DATATYPE_BOOLEAN, NULL, NULL },
	[CONTROL_VAR_VOLUME] =			{ SENDEVENT_NO, DATATYPE_UI2, NULL, &volume_range },
	[CONTROL_VAR_VOLUME_DB] =		{ SENDEVENT_NO, DATATYPE_I2, NULL, NULL },
	[CONTROL_VAR_LOUDNESS] =		{ SENDEVENT_NO, DATATYPE_BOOLEAN, NULL, NULL },
	[CONTROL_VAR_UNKNOWN] =			{ SENDEVENT_NO, DATATYPE_UNKNOWN, NULL, NULL }
};

static char *control_values[] = {
	[CONTROL_VAR_LAST_CHANGE] = "<Event xmlns = \"urn:schemas-upnp-org:metadata-1-0/RCS/\">\r\n<InstanceID val=\"0\">\r\n</InstanceID>\r\n</Event>",
	[CONTROL_VAR_AAT_CHANNEL] = "",
	[CONTROL_VAR_AAT_INSTANCE_ID] = "0",
	[CONTROL_VAR_MUTE] = "0",
	[CONTROL_VAR_VOLUME] = "0",
	[CONTROL_VAR_VOLUME_DB] = "0",
	[CONTROL_VAR_LOUDNESS] = "0",
	[CONTROL_VAR_UNKNOWN] = NULL
};
static ithread_mutex_t control_mutex;

static struct argument *arguments_get_mute[] = {
	& (struct argument) { "InstanceID", PARAM_DIR_IN, CONTROL_VAR_AAT_INSTANCE_ID },
	& (struct argument) { "Channel", PARAM_DIR_IN, CONTROL_VAR_AAT_CHANNEL },
	& (struct argument) { "CurrentMute", PARAM_DIR_OUT, CONTROL_VAR_MUTE },
	NULL
};
static struct argument *arguments_set_mute[] = {
	& (struct argument) { "InstanceID", PARAM_DIR_IN, CONTROL_VAR_AAT_INSTANCE_ID },
	& (struct argument) { "Channel", PARAM_DIR_IN, CONTROL_VAR_AAT_CHANNEL },
	& (struct argument) { "DesiredMute", PARAM_DIR_IN, CONTROL_VAR_MUTE },
	NULL
};
static struct argument *arguments_get_vol[] = {
	& (struct argument) { "InstanceID", PARAM_DIR_IN, CONTROL_VAR_AAT_INSTANCE_ID },
	& (struct argument) { "Channel", PARAM_DIR_IN, CONTROL_VAR_AAT_CHANNEL },
	& (struct argument) { "CurrentVolume", PARAM_DIR_OUT, CONTROL_VAR_VOLUME },
	NULL
};
static struct argument *arguments_set_vol[] = {
	& (struct argument) { "InstanceID", PARAM_DIR_IN, CONTROL_VAR_AAT_INSTANCE_ID },
	& (struct argument) { "Channel", PARAM_DIR_IN, CONTROL_VAR_AAT_CHANNEL },
	& (struct argument) { "DesiredVolume", PARAM_DIR_IN, CONTROL_VAR_VOLUME },
	NULL
};
static struct argument *arguments_get_vol_db[] = {
	& (struct argument) { "InstanceID", PARAM_DIR_IN, CONTROL_VAR_AAT_INSTANCE_ID },
	& (struct argument) { "Channel", PARAM_DIR_IN, CONTROL_VAR_AAT_CHANNEL },
	& (struct argument) { "CurrentVolume", PARAM_DIR_OUT, CONTROL_VAR_VOLUME_DB },
	NULL
};
static struct argument *arguments_set_vol_db[] = {
	& (struct argument) { "InstanceID", PARAM_DIR_IN, CONTROL_VAR_AAT_INSTANCE_ID },
	& (struct argument) { "Channel", PARAM_DIR_IN, CONTROL_VAR_AAT_CHANNEL },
	& (struct argument) { "DesiredVolume", PARAM_DIR_IN, CONTROL_VAR_VOLUME_DB },
	NULL
};
static struct argument *arguments_get_vol_dbrange[] = {
	& (struct argument) { "InstanceID", PARAM_DIR_IN, CONTROL_VAR_AAT_INSTANCE_ID },
	& (struct argument) { "Channel", PARAM_DIR_IN, CONTROL_VAR_AAT_CHANNEL },
	& (struct argument) { "MinValue", PARAM_DIR_OUT, CONTROL_VAR_VOLUME_DB },
	& (struct argument) { "MaxValue", PARAM_DIR_OUT, CONTROL_VAR_VOLUME_DB },
	NULL
};
static struct argument *arguments_get_eqcap[] = {
	& (struct argument) { "InstanceID", PARAM_DIR_IN, CONTROL_VAR_AAT_INSTANCE_ID },
	& (struct argument) { "EQCapabilities", PARAM_DIR_OUT, CONTROL_VAR_AAT_EQCAP },
	NULL
};
static struct argument *arguments_get_loudness[] = {
	& (struct argument) { "InstanceID", PARAM_DIR_IN, CONTROL_VAR_AAT_INSTANCE_ID },
	& (struct argument) { "Channel", PARAM_DIR_IN, CONTROL_VAR_AAT_CHANNEL },
	& (struct argument) { "CurrentLoudness", PARAM_DIR_OUT, CONTROL_VAR_LOUDNESS },
	NULL
};
static struct argument *arguments_set_loudness[] = {
	& (struct argument) { "InstanceID", PARAM_DIR_IN, CONTROL_VAR_AAT_INSTANCE_ID },
	& (struct argument) { "Channel", PARAM_DIR_IN, CONTROL_VAR_AAT_CHANNEL },
	& (struct argument) { "DesiredLoudness", PARAM_DIR_IN, CONTROL_VAR_LOUDNESS },
	NULL
};


static struct argument **argument_list[] = {
	[CONTROL_CMD_GET_MUTE] =            	arguments_get_mute,
	[CONTROL_CMD_SET_MUTE] =            	arguments_set_mute,
	[CONTROL_CMD_GET_VOL] =             	arguments_get_vol,
	[CONTROL_CMD_SET_VOL] =             	arguments_set_vol,
	[CONTROL_CMD_GET_VOL_DB] =          	arguments_get_vol_db,
	[CONTROL_CMD_SET_VOL_DB] =          	arguments_set_vol_db,
	[CONTROL_CMD_GET_VOL_DBRANGE] =     	arguments_get_vol_dbrange,
	[CONTROL_CMD_GET_EQCAP] =     	        arguments_get_eqcap,
	[CONTROL_CMD_GET_LOUDNESS] =        	arguments_get_loudness,
	[CONTROL_CMD_SET_LOUDNESS] =        	arguments_set_loudness,
	[CONTROL_CMD_UNKNOWN] =			NULL
};



static int cmd_obtain_variable(struct action_event *event, int varnum,
			       char *paramname)
{
	char *value;

	value = upnp_get_string(event, "InstanceID");
	if (value == NULL) {
		return -1;
	}
	free(value);

	return upnp_append_variable(event, varnum, paramname);
}

static int get_mute(struct action_event *event)
{
	/* FIXME - Channel */
	return cmd_obtain_variable(event, CONTROL_VAR_MUTE, "CurrentMute");
}

static int get_volume(struct action_event *event)
{
	/* FIXME - Channel */
	return cmd_obtain_variable(event, CONTROL_VAR_VOLUME,
				   "CurrentVolume");
}

static int get_volume_db(struct action_event *event)
{
	/* FIXME - Channel */
	return cmd_obtain_variable(event, CONTROL_VAR_VOLUME_DB,
				   "CurrentVolumeDB");
}

static int get_loudness(struct action_event *event)
{
	/* FIXME - Channel */
	return cmd_obtain_variable(event, CONTROL_VAR_LOUDNESS,
				   "CurrentLoudness");
}

static int set_volume(struct action_event *event)
{
	char *value;
    int fd = 0;
    int ret = 0;
    loff_t addr;
    unsigned val, orig_val;

	value = upnp_get_string(event, "DesiredVolume");
	if (value == NULL) {
		return -1;
	}

	ithread_mutex_lock(&control_mutex);

    fd = open("/dev/armem", O_RDWR);
    if (fd >= 0)
    {
        addr = 0xb80b0004 & 0xffffffff;
        lseek(fd, addr, SEEK_SET);
        if (read(fd, &orig_val, sizeof(orig_val)) != sizeof(orig_val))
            printf("read volume register failed\n");
        close(fd);
    }
    if((orig_val >= 0x0202) && (orig_val <= 0x0707))
        orig_val = 100;
    else if(orig_val == 0x0101)
        orig_val = 90;
    else if((orig_val == 0x0) || (orig_val == 0x1010))
        orig_val = 80;
    else if(orig_val == 0x1111)
        orig_val = 70;
    else if(orig_val == 0x1212)
        orig_val = 60;
    else if(orig_val == 0x1313)
        orig_val = 50;
    else if(orig_val == 0x1414)
        orig_val = 40;
    else if(orig_val == 0x1515)
        orig_val = 30;
    else if(orig_val == 0x1616)
        orig_val = 20;
    else if(orig_val == 0x1717)
        orig_val = 10;
    else
        orig_val = 0;
    val = atoi(value);
    if(((orig_val/10) == (val/10)) && (val > orig_val))
    {
        val+=10;
        if(val > 100)
            val = 100;
    }
    val /= 10;
    if(val == 10)
        val = 0x0202;
    else if(val == 9)
        val = 0x0101;
    else if(val == 8)
        val = 0x0;
    else if(val == 7)
        val = 0x1111;
    else if(val == 6)
        val = 0x1212;
    else if(val == 5)
        val = 0x1313;
    else if(val == 4)
        val = 0x1414;
    else if(val == 3)
        val = 0x1515;
    else if(val == 2)
        val = 0x1616;
    else if(val == 1)
        val = 0x1717;
    else
        val = 0x1f1f;
	fd = open("/dev/armem", O_RDWR);

	if (fd >= 0)
	{
	  addr = 0xb80b0004 & 0xffffffff;
      lseek(fd, addr, SEEK_SET);
	  write(fd, &val, sizeof(val));
      close(fd);
	}

	free(value);

	ithread_mutex_unlock(&control_mutex);

	return 0;
}


static struct action control_actions[] = {
	[CONTROL_CMD_GET_MUTE] =            	{"GetMute", get_mute}, /* optional */
	[CONTROL_CMD_SET_MUTE] =            	{"SetMute", NULL}, /* optional */
	[CONTROL_CMD_GET_VOL] =             	{"GetVolume", get_volume}, /* optional */
	[CONTROL_CMD_SET_VOL] =             	{"SetVolume", set_volume}, /* optional */
	[CONTROL_CMD_GET_VOL_DB] =          	{"GetVolumeDB", get_volume_db}, /* optional */
	[CONTROL_CMD_SET_VOL_DB] =          	{"SetVolumeDB", NULL}, /* optional */
	[CONTROL_CMD_GET_VOL_DBRANGE] =     	{"GetVolumeDBRange", NULL}, /* optional */
	[CONTROL_CMD_GET_EQCAP] =            	{"GetEQCapabilities", NULL}, /* optional */
	[CONTROL_CMD_GET_LOUDNESS] =        	{"GetLoudness", get_loudness}, /* optional */
	[CONTROL_CMD_SET_LOUDNESS] =        	{"SetLoudness", NULL}, /* optional */
	[CONTROL_CMD_UNKNOWN] =			{NULL, NULL}
};


struct service control_service = {
	.service_name =	CONTROL_SERVICE,
	.type =	CONTROL_TYPE,
    .scpd_url = CONTROL_SCPD_URL,
    .control_url = CONTROL_CONTROL_URL,
    .event_url = CONTROL_EVENT_URL,
	.actions =	control_actions,
	.action_arguments =	argument_list,
	.variable_names =	control_variables,
	.variable_values =	control_values,
	.variable_meta =	control_var_meta,
	.variable_count =	CONTROL_VAR_UNKNOWN,
	.command_count =	CONTROL_CMD_UNKNOWN,
	.service_mutex =	&control_mutex
};

void control_init(void)
{
}

void update_volume(void)
{
	char buf[2048];
    char *buf2=NULL;
    int rc;
    int fd = 0;
    char volume[32];
    loff_t addr;
    unsigned val = 0;


	const char *varnames[] = {
		"LastChange",
		NULL
	};
	char *varvalues[] = {
		NULL, NULL
	};
    char sUDN[128];

    memset(buf, 0x0, sizeof(buf));

    memset(sUDN, 0x0, sizeof(sUDN));

    sprintf(sUDN, "uuid:%s", my_uuid);

    memset(volume, 0x0, sizeof(volume));

    fd = open("/dev/armem", O_RDWR);

    if (fd >= 0)
    {
        addr = 0xb80b0004 & 0xffffffff;
        lseek(fd, addr, SEEK_SET);
        if (read(fd, &val, sizeof(val)) != sizeof(val))
            printf("read volume register failed\n");
        close(fd);
    }
    if((val >= 0x0202) && (val <= 0x0707))
        sprintf(volume, "100");
    else if(val == 0x0101)
        sprintf(volume, "90");
    else if((val == 0x0) || (val == 0x1010))
        sprintf(volume, "80");
    else if(val == 0x1111)
        sprintf(volume, "70");
    else if(val == 0x1212)
        sprintf(volume, "60");
    else if(val == 0x1313)
        sprintf(volume, "50");
    else if(val == 0x1414)
        sprintf(volume, "40");
    else if(val == 0x1515)
        sprintf(volume, "30");
    else if(val == 0x1616)
        sprintf(volume, "20");
    else if(val == 0x1717)
        sprintf(volume, "10");
    else
        sprintf(volume, "0");
    sprintf(buf, "<Event xmlns = \"urn:schemas-upnp-org:metadata-1-0/RCS/\">\r\n<InstanceID val=\"0\">\r\n<Volume val=\"%s\" channel=\"Master\"/>\r\n</InstanceID>\r\n</Event>", volume);

    buf2 = xmlescape(buf, 1);

	varvalues[0] = buf2;

    rc = UpnpNotify(device_handle, sUDN,
             CONTROL_SERVICE, varnames,
             (const char **) varvalues, 1);

    free(buf2);

	return;
}
