/* upnp_transport.c - UPnP AVTransport routines
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//#include <glib.h>

#include "../../libupnp/include/upnp.h"
#include "../../libupnp/include/ithread.h"
#include "../../libupnp/include/upnptools.h"


#include "logging.h"

#include "xmlescape.h"
#include "upnp.h"
#include "upnp_device.h"
#include "upnp_transport.h"
#include "output_gstreamer.h"

#define TRANSPORT_SERVICE "urn:upnp-org:serviceId:AVTransport"
#define TRANSPORT_TYPE "urn:schemas-upnp-org:service:AVTransport:1"
#define TRANSPORT_SCPD_URL "/upnp/rendertransportSCPD.xml"
#define TRANSPORT_CONTROL_URL "/upnp/control/rendertransport1"
#define TRANSPORT_EVENT_URL "/upnp/event/rendertransport1"

#include <netinet/in.h>
#include <sys/socket.h>
extern struct sockaddr_storage ctrlpt_ipaddr;

struct play_info my_play_info;
char CurrentURI[1024];
char CurrentURIMetaData[1024];
char TrackDurationFromServer[16];
extern char my_uuid[128];
extern void update_volume(void);
void change_var(struct action_event *event, int varnum,
		       char *new_value);

typedef enum {
	TRANSPORT_VAR_TRANSPORT_STATE,
	TRANSPORT_VAR_TRANSPORT_STATUS,
	TRANSPORT_VAR_PLAY_MEDIUM,
	TRANSPORT_VAR_REC_MEDIUM,
	TRANSPORT_VAR_PLAY_MEDIA,
	TRANSPORT_VAR_REC_MEDIA,
	TRANSPORT_VAR_CUR_PLAY_MODE,
	TRANSPORT_VAR_TRANSPORT_PLAY_SPEED,
	TRANSPORT_VAR_REC_MEDIUM_WR_STATUS,
	TRANSPORT_VAR_CUR_REC_QUAL_MODE,
	TRANSPORT_VAR_POS_REC_QUAL_MODE,
	TRANSPORT_VAR_NR_TRACKS,
	TRANSPORT_VAR_CUR_TRACK,
	TRANSPORT_VAR_CUR_TRACK_DUR,
	TRANSPORT_VAR_CUR_MEDIA_DUR,
	TRANSPORT_VAR_CUR_TRACK_META,
	TRANSPORT_VAR_CUR_TRACK_URI,
	TRANSPORT_VAR_AV_URI,
	TRANSPORT_VAR_AV_URI_META,
	TRANSPORT_VAR_NEXT_AV_URI,
	TRANSPORT_VAR_NEXT_AV_URI_META,
	TRANSPORT_VAR_REL_TIME_POS,
	TRANSPORT_VAR_ABS_TIME_POS,
	TRANSPORT_VAR_REL_CTR_POS,
	TRANSPORT_VAR_ABS_CTR_POS,
	TRANSPORT_VAR_LAST_CHANGE,
	TRANSPORT_VAR_AAT_SEEK_MODE,
	TRANSPORT_VAR_AAT_SEEK_TARGET,
	TRANSPORT_VAR_AAT_INSTANCE_ID,
	TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS,
	TRANSPORT_VAR_UNKNOWN,
	TRANSPORT_VAR_COUNT
} transport_variable;

typedef enum {
	TRANSPORT_CMD_GETCURRENTTRANSPORTACTIONS,
	TRANSPORT_CMD_GETDEVICECAPABILITIES,
	TRANSPORT_CMD_GETMEDIAINFO,
	TRANSPORT_CMD_GETPOSITIONINFO,
	TRANSPORT_CMD_GETTRANSPORTINFO,
	TRANSPORT_CMD_GETTRANSPORTSETTINGS,
	TRANSPORT_CMD_NEXT,
	TRANSPORT_CMD_PAUSE,
	TRANSPORT_CMD_PLAY,
	TRANSPORT_CMD_PREVIOUS,
	TRANSPORT_CMD_SEEK,
	TRANSPORT_CMD_SETAVTRANSPORTURI,
	TRANSPORT_CMD_SETPLAYMODE,
	TRANSPORT_CMD_STOP,
	TRANSPORT_CMD_SETNEXTAVTRANSPORTURI,
	TRANSPORT_CMD_UNKNOWN,
	TRANSPORT_CMD_COUNT
} transport_cmd ;

static const char *transport_variables[] = {
	[TRANSPORT_VAR_TRANSPORT_STATE] = "TransportState",
	[TRANSPORT_VAR_TRANSPORT_STATUS] = "TransportStatus",
	[TRANSPORT_VAR_PLAY_MEDIUM] = "PlaybackStorageMedium",
	[TRANSPORT_VAR_REC_MEDIUM] = "RecordStorageMedium",
	[TRANSPORT_VAR_PLAY_MEDIA] = "PossiblePlaybackStorageMedia",
	[TRANSPORT_VAR_REC_MEDIA] = "PossibleRecordStorageMedia",
	[TRANSPORT_VAR_CUR_PLAY_MODE] = "CurrentPlayMode",
	[TRANSPORT_VAR_TRANSPORT_PLAY_SPEED] = "TransportPlaySpeed",
	[TRANSPORT_VAR_REC_MEDIUM_WR_STATUS] = "RecordMediumWriteStatus",
	[TRANSPORT_VAR_CUR_REC_QUAL_MODE] = "CurrentRecordQualityMode",
	[TRANSPORT_VAR_POS_REC_QUAL_MODE] = "PossibleRecordQualityModes",
	[TRANSPORT_VAR_NR_TRACKS] = "NumberOfTracks",
	[TRANSPORT_VAR_CUR_TRACK] = "CurrentTrack",
	[TRANSPORT_VAR_CUR_TRACK_DUR] = "CurrentTrackDuration",
	[TRANSPORT_VAR_CUR_MEDIA_DUR] = "CurrentMediaDuration",
	[TRANSPORT_VAR_CUR_TRACK_META] = "CurrentTrackMetaData",
	[TRANSPORT_VAR_CUR_TRACK_URI] = "CurrentTrackURI",
	[TRANSPORT_VAR_AV_URI] = "AVTransportURI",
	[TRANSPORT_VAR_AV_URI_META] = "AVTransportURIMetaData",
	[TRANSPORT_VAR_NEXT_AV_URI] = "NextAVTransportURI",
	[TRANSPORT_VAR_NEXT_AV_URI_META] = "NextAVTransportURIMetaData",
	[TRANSPORT_VAR_REL_TIME_POS] = "RelativeTimePosition",
	[TRANSPORT_VAR_ABS_TIME_POS] = "AbsoluteTimePosition",
	[TRANSPORT_VAR_REL_CTR_POS] = "RelativeCounterPosition",
	[TRANSPORT_VAR_ABS_CTR_POS] = "AbsoluteCounterPosition",
	[TRANSPORT_VAR_LAST_CHANGE] = "LastChange",
	[TRANSPORT_VAR_AAT_SEEK_MODE] = "A_ARG_TYPE_SeekMode",
	[TRANSPORT_VAR_AAT_SEEK_TARGET] = "A_ARG_TYPE_SeekTarget",
	[TRANSPORT_VAR_AAT_INSTANCE_ID] = "A_ARG_TYPE_InstanceID",
	[TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS] = "CurrentTransportActions",	/* optional */
	[TRANSPORT_VAR_UNKNOWN] = NULL
};

static char *transport_values[] = {
	[TRANSPORT_VAR_TRANSPORT_STATE] = "STOPPED",
	[TRANSPORT_VAR_TRANSPORT_STATUS] = "OK",
	[TRANSPORT_VAR_PLAY_MEDIUM] = "UNKNOWN",
	[TRANSPORT_VAR_REC_MEDIUM] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_PLAY_MEDIA] = "NETWORK,UNKNOWN",
	[TRANSPORT_VAR_REC_MEDIA] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_CUR_PLAY_MODE] = "NORMAL",
	[TRANSPORT_VAR_TRANSPORT_PLAY_SPEED] = "1",
	[TRANSPORT_VAR_REC_MEDIUM_WR_STATUS] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_CUR_REC_QUAL_MODE] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_POS_REC_QUAL_MODE] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_NR_TRACKS] = "1",
	[TRANSPORT_VAR_CUR_TRACK] = "1",
	[TRANSPORT_VAR_CUR_TRACK_DUR] = "00:00:00",
	[TRANSPORT_VAR_CUR_MEDIA_DUR] = "",
	[TRANSPORT_VAR_CUR_TRACK_META] = "",
	[TRANSPORT_VAR_CUR_TRACK_URI] = "",
	[TRANSPORT_VAR_AV_URI] = "",
	[TRANSPORT_VAR_AV_URI_META] = "",
	[TRANSPORT_VAR_NEXT_AV_URI] = "",
	[TRANSPORT_VAR_NEXT_AV_URI_META] = "",
	[TRANSPORT_VAR_REL_TIME_POS] = "00:00:00",
	[TRANSPORT_VAR_ABS_TIME_POS] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_REL_CTR_POS] = "2147483647",
	[TRANSPORT_VAR_ABS_CTR_POS] = "2147483647",
    [TRANSPORT_VAR_LAST_CHANGE] = "<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT/\"/>\r\n</Event>",
	[TRANSPORT_VAR_AAT_SEEK_MODE] = "TRACK_NR",
	[TRANSPORT_VAR_AAT_SEEK_TARGET] = "",
	[TRANSPORT_VAR_AAT_INSTANCE_ID] = "0",
	[TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS] = "Play,Stop,Pause,Seek",
	[TRANSPORT_VAR_UNKNOWN] = NULL
};

static const char *transport_states[] = {
	"STOPPED",
	"PLAYING",
	"PAUSED_PLAYBACK",
	"TRANSITIONING",
	NULL
};

static const char *transport_stati[] = {
	NULL
};

static const char *media[] = {
	"NONE",
	"NETWORK",
	NULL
};

static const char *playspeeds[] = {
	"1",
	NULL
};

static const char *rec_write_stati[] = {
	NULL
};

static const char *rec_quality_modi[] = {
	NULL
};

static struct param_range track_range = {
	0,
	65535,
	1
};

static struct param_range track_nr_range = {
	0,
	65535,
	0
};

static const char *aat_seekmodi[] = {
	"TRACK_NR",
	"REL_TIME",
	"SECTION",
	NULL
};


static const char *playmodi[] = {
	"NORMAL",
	"REPEAT_ALL",
	"INTRO",
	NULL
};

static struct var_meta transport_var_meta[] = {
	[TRANSPORT_VAR_TRANSPORT_STATE] =		{ SENDEVENT_NO, DATATYPE_STRING, transport_states, NULL },
	[TRANSPORT_VAR_TRANSPORT_STATUS] =		{ SENDEVENT_NO, DATATYPE_STRING, transport_stati, NULL },
	[TRANSPORT_VAR_PLAY_MEDIUM] =			{ SENDEVENT_NO, DATATYPE_STRING, media, NULL },
	[TRANSPORT_VAR_REC_MEDIUM] =			{ SENDEVENT_NO, DATATYPE_STRING, media, NULL },
	[TRANSPORT_VAR_PLAY_MEDIA] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_REC_MEDIA] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_CUR_PLAY_MODE] =			{ SENDEVENT_NO, DATATYPE_STRING, playmodi, NULL, "NORMAL" },
	[TRANSPORT_VAR_TRANSPORT_PLAY_SPEED] =		{ SENDEVENT_NO, DATATYPE_STRING, playspeeds, NULL },
	[TRANSPORT_VAR_REC_MEDIUM_WR_STATUS] =		{ SENDEVENT_NO, DATATYPE_STRING, rec_write_stati, NULL },
	[TRANSPORT_VAR_CUR_REC_QUAL_MODE] =		{ SENDEVENT_NO, DATATYPE_STRING, rec_quality_modi, NULL },
	[TRANSPORT_VAR_POS_REC_QUAL_MODE] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_NR_TRACKS] =			{ SENDEVENT_NO, DATATYPE_UI4, NULL, &track_nr_range }, /* no step */
	[TRANSPORT_VAR_CUR_TRACK] =			{ SENDEVENT_NO, DATATYPE_UI4, NULL, &track_range },
	[TRANSPORT_VAR_CUR_TRACK_DUR] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_CUR_MEDIA_DUR] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_CUR_TRACK_META] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_CUR_TRACK_URI] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_AV_URI] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_AV_URI_META] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_NEXT_AV_URI] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_NEXT_AV_URI_META] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_REL_TIME_POS] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_ABS_TIME_POS] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_REL_CTR_POS] =			{ SENDEVENT_NO, DATATYPE_I4, NULL, NULL },
	[TRANSPORT_VAR_ABS_CTR_POS] =			{ SENDEVENT_NO, DATATYPE_I4, NULL, NULL },
	[TRANSPORT_VAR_LAST_CHANGE] =			{ SENDEVENT_YES, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_AAT_SEEK_MODE] =			{ SENDEVENT_NO, DATATYPE_STRING, aat_seekmodi, NULL },
	[TRANSPORT_VAR_AAT_SEEK_TARGET] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_AAT_INSTANCE_ID] =		{ SENDEVENT_NO, DATATYPE_UI4, NULL, NULL },
	[TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_UNKNOWN] =			{ SENDEVENT_NO, DATATYPE_UNKNOWN, NULL, NULL }
};

static struct argument *arguments_setavtransporturi[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "CurrentURI", PARAM_DIR_IN, TRANSPORT_VAR_AV_URI },
        & (struct argument) { "CurrentURIMetaData", PARAM_DIR_IN, TRANSPORT_VAR_AV_URI_META },
        NULL
};

static struct argument *arguments_setnextavtransporturi[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "NextURI", PARAM_DIR_IN, TRANSPORT_VAR_NEXT_AV_URI },
        & (struct argument) { "NextURIMetaData", PARAM_DIR_IN, TRANSPORT_VAR_NEXT_AV_URI_META },
        NULL
};

static struct argument *arguments_getmediainfo[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "NrTracks", PARAM_DIR_OUT, TRANSPORT_VAR_NR_TRACKS },
        & (struct argument) { "MediaDuration", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_MEDIA_DUR },
        & (struct argument) { "CurrentURI", PARAM_DIR_OUT, TRANSPORT_VAR_AV_URI },
        & (struct argument) { "CurrentURIMetaData", PARAM_DIR_OUT, TRANSPORT_VAR_AV_URI_META },
        & (struct argument) { "NextURI", PARAM_DIR_OUT, TRANSPORT_VAR_NEXT_AV_URI },
        & (struct argument) { "NextURIMetaData", PARAM_DIR_OUT, TRANSPORT_VAR_NEXT_AV_URI_META },
        & (struct argument) { "PlayMedium", PARAM_DIR_OUT, TRANSPORT_VAR_PLAY_MEDIUM },
        & (struct argument) { "RecordMedium", PARAM_DIR_OUT, TRANSPORT_VAR_REC_MEDIUM },
        & (struct argument) { "WriteStatus", PARAM_DIR_OUT, TRANSPORT_VAR_REC_MEDIUM_WR_STATUS },
        NULL
};

static struct argument *arguments_gettransportinfo[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "CurrentTransportState", PARAM_DIR_OUT, TRANSPORT_VAR_TRANSPORT_STATE },
        & (struct argument) { "CurrentTransportStatus", PARAM_DIR_OUT, TRANSPORT_VAR_TRANSPORT_STATUS },
        & (struct argument) { "CurrentSpeed", PARAM_DIR_OUT, TRANSPORT_VAR_TRANSPORT_PLAY_SPEED },
        NULL
};

static struct argument *arguments_getpositioninfo[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "Track", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_TRACK },
        & (struct argument) { "TrackDuration", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_TRACK_DUR },
        & (struct argument) { "TrackMetaData", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_TRACK_META },
        & (struct argument) { "TrackURI", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_TRACK_URI },
        & (struct argument) { "RelTime", PARAM_DIR_OUT, TRANSPORT_VAR_REL_TIME_POS },
        & (struct argument) { "AbsTime", PARAM_DIR_OUT, TRANSPORT_VAR_ABS_TIME_POS },
        & (struct argument) { "RelCount", PARAM_DIR_OUT, TRANSPORT_VAR_REL_CTR_POS },
        & (struct argument) { "AbsCount", PARAM_DIR_OUT, TRANSPORT_VAR_ABS_CTR_POS },
        NULL
};

static struct argument *arguments_getdevicecapabilities[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "PlayMedia", PARAM_DIR_OUT, TRANSPORT_VAR_PLAY_MEDIA },
        & (struct argument) { "RecMedia", PARAM_DIR_OUT, TRANSPORT_VAR_REC_MEDIA },
        & (struct argument) { "RecQualityModes", PARAM_DIR_OUT, TRANSPORT_VAR_POS_REC_QUAL_MODE },
	NULL
};

static struct argument *arguments_gettransportsettings[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "PlayMode", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_PLAY_MODE },
        & (struct argument) { "RecQualityMode", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_REC_QUAL_MODE },
	NULL
};

static struct argument *arguments_stop[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
	NULL
};
static struct argument *arguments_play[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "Speed", PARAM_DIR_IN, TRANSPORT_VAR_TRANSPORT_PLAY_SPEED },
	NULL
};
static struct argument *arguments_pause[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
	NULL
};

static struct argument *arguments_seek[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "Unit", PARAM_DIR_IN, TRANSPORT_VAR_AAT_SEEK_MODE },
        & (struct argument) { "Target", PARAM_DIR_IN, TRANSPORT_VAR_AAT_SEEK_TARGET },
	NULL
};
static struct argument *arguments_next[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
	NULL
};
static struct argument *arguments_previous[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
	NULL
};
static struct argument *arguments_setplaymode[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "NewPlayMode", PARAM_DIR_IN, TRANSPORT_VAR_CUR_PLAY_MODE },
	NULL
};
static struct argument *arguments_getcurrenttransportactions[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "Actions", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS },
	NULL
};


static struct argument **argument_list[] = {
	[TRANSPORT_CMD_SETAVTRANSPORTURI] =         arguments_setavtransporturi,
	[TRANSPORT_CMD_GETDEVICECAPABILITIES] =     arguments_getdevicecapabilities,
	[TRANSPORT_CMD_GETMEDIAINFO] =              arguments_getmediainfo,
	[TRANSPORT_CMD_SETNEXTAVTRANSPORTURI] =     arguments_setnextavtransporturi,
	[TRANSPORT_CMD_GETTRANSPORTINFO] =          arguments_gettransportinfo,
	[TRANSPORT_CMD_GETPOSITIONINFO] =           arguments_getpositioninfo,
	[TRANSPORT_CMD_GETTRANSPORTSETTINGS] =      arguments_gettransportsettings,
	[TRANSPORT_CMD_STOP] =                      arguments_stop,
	[TRANSPORT_CMD_PLAY] =                      arguments_play,
	[TRANSPORT_CMD_PAUSE] =                     arguments_pause,
	[TRANSPORT_CMD_SEEK] =                      arguments_seek,
	[TRANSPORT_CMD_NEXT] =                      arguments_next,
	[TRANSPORT_CMD_PREVIOUS] =                  arguments_previous,
	[TRANSPORT_CMD_SETPLAYMODE] =               arguments_setplaymode,
	[TRANSPORT_CMD_GETCURRENTTRANSPORTACTIONS] = arguments_getcurrenttransportactions,
	[TRANSPORT_CMD_UNKNOWN] =	NULL
};

/* protects transport_values, and service-specific state */

static ithread_mutex_t transport_mutex;

enum _transport_state {
	TRANSPORT_STOPPED,
	TRANSPORT_PLAYING,
	TRANSPORT_TRANSITIONING,	/* optional */
	TRANSPORT_PAUSED_PLAYBACK,	/* optional */
	TRANSPORT_PAUSED_RECORDING,	/* optional */
	TRANSPORT_RECORDING,	/* optional */
	TRANSPORT_NO_MEDIA_PRESENT	/* optional */
};

static enum _transport_state transport_state = TRANSPORT_STOPPED;



static int get_media_info(struct action_event *event)
{
	char *value;
	int rc;
	ENTER();

	value = upnp_get_string(event, "InstanceID");
	if (value == NULL) {
		rc = -1;
		goto out;
	}
	free(value);

	rc = upnp_append_variable(event, TRANSPORT_VAR_NR_TRACKS,
				  "NrTracks");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_CUR_MEDIA_DUR,
				  "MediaDuration");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_AV_URI,
				  "CurrentURI");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_AV_URI_META,
				  "CurrentURIMetaData");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_NEXT_AV_URI,
				  "NextURI");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_NEXT_AV_URI_META,
				  "NextURIMetaData");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_REC_MEDIA,
				  "PlayMedium");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_REC_MEDIUM,
				  "RecordMedium");
	if (rc)
		goto out;

	rc = upnp_append_variable(event,
				  TRANSPORT_VAR_REC_MEDIUM_WR_STATUS,
				  "WriteStatus");
	if (rc)
		goto out;

      out:
	return rc;
}


static void notify_lastchange(struct action_event *event, char *value)
{
	const char *varnames[] = {
		"LastChange",
		NULL
	};
	char *varvalues[] = {
		NULL, NULL
	};
    int rc;
    char sUDN[128];

	varvalues[0] = value;

    memset(sUDN, 0x0, sizeof(sUDN));

    if(event)
    {
        rc = UpnpNotify(device_handle, event->request->DevUDN,
            event->request->ServiceID, varnames,
            (const char **) varvalues, 1);
    }
    else
    {
        sprintf(sUDN, "uuid:%s", my_uuid);
        rc = UpnpNotify(device_handle, sUDN,
            TRANSPORT_SERVICE, varnames,
            (const char **) varvalues, 1);
    }
}

static double  parse_upnp_time(const char *time_string) 
{
	int hour = 0;
	int minute = 0;
	int second = 0;
	sscanf(time_string, "%d:%02d:%02d", &hour, &minute, &second);
	const double  seconds = (hour * 3600 + minute * 60 + second);
	//const double  one_sec_unit = 1000000000LL;
	//return one_sec_unit * seconds;
	return seconds;
}

/* warning - does not lock service mutex */
void change_var(struct action_event *event, int varnum,
		       char *new_value)
{
	char buf[2048];
	char temp[2048];
    char *buf2=NULL;

	ENTER();

	if ((varnum < 0) || (varnum >= TRANSPORT_VAR_UNKNOWN)) {
		LEAVE();
		return;
	}
	if (new_value == NULL) {
		LEAVE();
		return;
	}

	
    memset(buf, 0x0, sizeof(buf));

    sprintf(buf, "<Event xmlns = \"urn:schemas-upnp-org:metadata-1-0/AVT/\">\r\n<InstanceID val=\"0\">\r\n");

	transport_values[varnum] = strdup(new_value);

	if(varnum == TRANSPORT_VAR_TRANSPORT_STATE)
	{
        memset(temp, 0x0, sizeof(temp));
        sprintf(temp, "<%s val=\"%s\"/>\r\n",
            transport_variables[varnum], xmlescape(transport_values[varnum], 1));
        strcat(buf, temp);

        memset(temp, 0x0, sizeof(temp));
	    if(!strcmp(new_value, "PAUSED_PLAYBACK"))
	    {
            strcat(buf, "<CurrentTransportActions val=\"Play,Stop\"/>\r\n");
	    }
	    else if(!strcmp(new_value, "PLAYING"))
	    {
            strcat(buf, "<CurrentTransportActions val=\"Stop,Pause\"/>\r\n");
	    }
	    else if(!strcmp(new_value, "STOPPED"))
	    {
            strcat(buf, "<CurrentTransportActions val=\"Play\"/>\r\n");
	    }

	}
    strcat(buf, "</InstanceID>\r\n</Event>");

    buf2 = xmlescape(buf, 1);

	notify_lastchange(event, buf2);

    free(buf2);

	LEAVE();

	return;
}

static int obtain_instanceid(struct action_event *event, int *instance)
{
	char *value;
	int rc = 0;

	ENTER();

	value = upnp_get_string(event, "InstanceID");
	if (value == NULL) {
		upnp_set_error(event, UPNP_SOAP_E_INVALID_ARGS,
			       "Missing InstanceID");
		return -1;
	}
	free(value);

	LEAVE();

	return rc;
}

/* UPnP action handlers */

static int set_avtransport_uri(struct action_event *event)
{
	char *value;
	int rc = 0;
	char *ptr = NULL;
	int i;

	ENTER();

	if (obtain_instanceid(event, NULL)) {
		LEAVE();
		return -1;
	}
	value = upnp_get_string(event, "CurrentURI");
	if (value == NULL) {
		LEAVE();
		return -1;
	}

	ithread_mutex_lock(&transport_mutex);

	memset(CurrentURI, 0x0, sizeof(CurrentURI));
	char* ctlip = NULL;
	if((ctlip = strstr(value,"127.0.0.1")))
	{
		char  *temp;
		temp=inet_ntoa(((struct sockaddr_in*)&ctrlpt_ipaddr)->sin_addr);
		printf("IP:%s\n",temp);
		ctlip += strlen("127.0.0.1");
		sprintf(CurrentURI,"http://%s%s",temp,ctlip);
		//sprintf(CurrentURI,"http://172.16.0.182:57645/external/audio/media/3838.mp3");
		output_set_uri(CurrentURI);
	}
	else{
	strcpy(CurrentURI, value);

	output_set_uri(value);
	}
    transport_state = TRANSPORT_STOPPED;

	free(value);

	value = upnp_get_string(event, "CurrentURIMetaData");
	memset(CurrentURIMetaData, 0x0, sizeof(CurrentURIMetaData));
	memset(TrackDurationFromServer, 0x0, sizeof(TrackDurationFromServer));
	if (value == NULL) {
		rc = -1;
	} else {
        strcpy(CurrentURIMetaData, value);

        ptr = strstr(CurrentURIMetaData, "duration=");
        if(ptr)
        {
            ptr+=strlen("duration=");
            if(*ptr == '\"') ptr++;
            strncpy(TrackDurationFromServer, ptr, 15);
            for(i=0;i<15;i++)
            {
                if((TrackDurationFromServer[i] == '.') ||(TrackDurationFromServer[i] == '\"'))
                {
                    TrackDurationFromServer[i] = '\0';
                    break;
                }
            }
        }

		free(value);
	}

	ithread_mutex_unlock(&transport_mutex);

	LEAVE();
	return rc;
}

static int set_next_avtransport_uri(struct action_event *event)
{
	char *value;

	ENTER();

	if (obtain_instanceid(event, NULL)) {
		LEAVE();
		return -1;
	}
	value = upnp_get_string(event, "NextURI");
	if (value == NULL) {
		LEAVE();
		return -1;
	}
	printf("%s: NextURI='%s'\n", __FUNCTION__, value);
	free(value);
	value = upnp_get_string(event, "NextURIMetaData");
	if (value == NULL) {
		LEAVE();
		return -1;
	}
	printf("%s: NextURIMetaData='%s'\n", __FUNCTION__, value);
	free(value);

	LEAVE();
	return 0;
}

static int get_transport_info(struct action_event *event)
{
	int rc;
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
		goto out;
	}

	rc = upnp_append_variable(event, TRANSPORT_VAR_TRANSPORT_STATE,
				  "CurrentTransportState");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_TRANSPORT_STATUS,
				  "CurrentTransportStatus");
	if (rc)
		goto out;

	rc = upnp_append_variable(event,
				  TRANSPORT_VAR_TRANSPORT_PLAY_SPEED,
				  "CurrentSpeed");
	if (rc)
		goto out;

      out:
	LEAVE();
	return rc;
}

static int get_transport_settings(struct action_event *event)
{
	int rc = 0;
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
		goto out;
	}

      out:
	LEAVE();
	return rc;
}

static int get_position_info(struct action_event *event)
{
	int rc;
    FILE *pFile;
    char buff[1024];
    char *vPtr;
    int count=0;

	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
		goto out;
	}
read_mpg123_info:
    memset(&my_play_info, 0x0, sizeof(struct play_info));

    pFile = fopen("/tmp/mpg123_status", "r");
    if(pFile)
    {
        while(!feof(pFile))
        {
            memset(buff, 0x0, sizeof(buff));
            fgets(buff,sizeof(buff),pFile);

            if( buff[0] == 0 )
                break;

            vPtr=strchr(buff,0x0d);
            if(vPtr)
                *vPtr = 0;  // extract the line feed
            else
                break;      // No line feed, bad line.

            vPtr = strchr(buff,'=');

            if(!vPtr)
                break;
            else
                *vPtr++ = 0;

            if(strstr(buff, "filename"))
            {
                strcpy(my_play_info.filename, vPtr);
            }
            else if(strstr(buff, "total"))
            {
                strcpy(my_play_info.total_duration_string, vPtr);
            }
            else if(strstr(buff, "current"))
            {
                strcpy(my_play_info.current_time_string, vPtr);
            }
        }
        fclose(pFile);
    }
    else
    {
        printf("open file error\n");
    }
    count++;

    if(strlen(TrackDurationFromServer))
    {
        strcpy(my_play_info.total_duration_string, TrackDurationFromServer);
    }

    if(strlen(my_play_info.total_duration_string) == 0)
    {
        if(count<11)
        {
            sleep(1);
            goto read_mpg123_info;
        }
        else
    		goto out;
    }
	rc = upnp_append_variable(event, TRANSPORT_VAR_CUR_TRACK, "Track");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_CUR_TRACK_DUR,
				  "TrackDuration");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_CUR_TRACK_META,
				  "TrackMetaData");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_CUR_TRACK_URI,
				  "TrackURI");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_REL_TIME_POS,
				  "RelTime");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_ABS_TIME_POS,
				  "AbsTime");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_REL_CTR_POS,
				  "RelCount");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_ABS_CTR_POS,
				  "AbsCount");
	if (rc)
		goto out;

      out:
	LEAVE();

	return rc;
}

static int get_device_caps(struct action_event *event)
{
	int rc = 0;
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
		goto out;
	}

      out:
	LEAVE();
	return rc;
}

static int stop(struct action_event *event)
{
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		return -1;
	}

	ithread_mutex_lock(&transport_mutex);
	switch (transport_state) {
	case TRANSPORT_STOPPED:
		break;
	case TRANSPORT_PLAYING:
	case TRANSPORT_TRANSITIONING:
	case TRANSPORT_PAUSED_RECORDING:
	case TRANSPORT_RECORDING:
	case TRANSPORT_PAUSED_PLAYBACK:
		output_stop();
		transport_state = TRANSPORT_STOPPED;
		change_var(event, TRANSPORT_VAR_TRANSPORT_STATE,
			   "STOPPED");
		break;

	case TRANSPORT_NO_MEDIA_PRESENT:
		/* action not allowed in these states - error 701 */
		upnp_set_error(event, UPNP_TRANSPORT_E_TRANSITION_NA,
			       "Transition not allowed");

		break;
	}
	ithread_mutex_unlock(&transport_mutex);

	LEAVE();

	return 0;
}


static int play(struct action_event *event)
{
	int rc = 0;

	ENTER();

	if (obtain_instanceid(event, NULL)) {
		LEAVE();
		return -1;
	}

	ithread_mutex_lock(&transport_mutex);
	switch (transport_state) {
	case TRANSPORT_PLAYING:
		break;
	case TRANSPORT_STOPPED:
		if (output_play()) {
			upnp_set_error(event, 704, "Playing failed");
			rc = -1;
		} else {
			transport_state = TRANSPORT_PLAYING;
			change_var(event, TRANSPORT_VAR_TRANSPORT_STATE,
				   "PLAYING");
            update_volume();
		}
		break;
	case TRANSPORT_PAUSED_PLAYBACK:
        output_pause();
		transport_state = TRANSPORT_PLAYING;
		change_var(event, TRANSPORT_VAR_TRANSPORT_STATE,
			   "PLAYING");
		break;
	case TRANSPORT_NO_MEDIA_PRESENT:
	case TRANSPORT_TRANSITIONING:
	case TRANSPORT_PAUSED_RECORDING:
	case TRANSPORT_RECORDING:
		/* action not allowed in these states - error 701 */
		upnp_set_error(event, UPNP_TRANSPORT_E_TRANSITION_NA,
			       "Transition not allowed");
		rc = -1;

		break;
	}
	ithread_mutex_unlock(&transport_mutex);

	LEAVE();

	return rc;
}

static int au_pause(struct action_event *event)
{
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		return -1;
	}

	ithread_mutex_lock(&transport_mutex);
	switch (transport_state) {
	case TRANSPORT_PLAYING:
	case TRANSPORT_PAUSED_PLAYBACK:
		output_pause();
		if(transport_state == TRANSPORT_PLAYING)
		{
            transport_state = TRANSPORT_PAUSED_PLAYBACK;
    		change_var(event, TRANSPORT_VAR_TRANSPORT_STATE, "PAUSED_PLAYBACK");
		}
		else
		{
            transport_state = TRANSPORT_PLAYING;
    		change_var(event, TRANSPORT_VAR_TRANSPORT_STATE, "PLAYING");
		}
		break;
	case TRANSPORT_RECORDING:
	case TRANSPORT_PAUSED_RECORDING:
	case TRANSPORT_STOPPED:
	case TRANSPORT_TRANSITIONING:
	case TRANSPORT_NO_MEDIA_PRESENT:
		/* action not allowed in these states - error 701 */
		upnp_set_error(event, UPNP_TRANSPORT_E_TRANSITION_NA,
			       "Transition not allowed");

		break;
	}
	ithread_mutex_unlock(&transport_mutex);

	LEAVE();

	return 0;
}

static int seek(struct action_event *event)
{
	int rc = 0;
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
	}
	
	#if 1
	ithread_mutex_lock(&transport_mutex);
	printf("Seeeeeeek:transport_state = %d\n",transport_state);

	char *unit = upnp_get_string(event, "Unit");
	if(unit)printf("unit=%s\n",unit);
	if((unit) && (strcmp(unit, "REL_TIME") == 0)) {
		// This is the only thing we support right now.
		char *target = upnp_get_string(event, "Target");
		printf("target=%s\n",target);
		double secs = parse_upnp_time(target);
		if(output_seek(secs)==0)
		{
			printf("transport_values:%s\n",transport_values[TRANSPORT_VAR_REL_TIME_POS]);	
			transport_values[TRANSPORT_VAR_REL_TIME_POS]=strdup(target);
			change_var(event, TRANSPORT_VAR_TRANSPORT_STATE, "PLAYING");
		}
		if(target)free(target);
	}

	if(unit)free(unit);
	ithread_mutex_unlock(&transport_mutex);
	#else
	ithread_mutex_lock(&transport_mutex);
	switch (transport_state) {
	case TRANSPORT_PLAYING:
        change_var(event, TRANSPORT_VAR_TRANSPORT_STATE, "PLAYING");
        break;
	case TRANSPORT_PAUSED_PLAYBACK:
        change_var(event, TRANSPORT_VAR_TRANSPORT_STATE, "PAUSED_PLAYBACK");
        break;
    default:
		/* action not allowed in these states - error 701 */
		upnp_set_error(event, UPNP_TRANSPORT_E_TRANSITION_NA,
			       "Transition not allowed");
		break;
	}
	ithread_mutex_unlock(&transport_mutex);
	#endif
	LEAVE();

	return rc;
}

static int next(struct action_event *event)
{
	int rc = 0;

	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
	}

	LEAVE();

	return rc;
}

static int previous(struct action_event *event)
{
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		return -1;
	}

	LEAVE();

	return 0;
}


static struct action transport_actions[] = {
	[TRANSPORT_CMD_GETCURRENTTRANSPORTACTIONS] = {"GetCurrentTransportActions", NULL},	/* optional */
	[TRANSPORT_CMD_GETDEVICECAPABILITIES] =     {"GetDeviceCapabilities", get_device_caps},
	[TRANSPORT_CMD_GETMEDIAINFO] =              {"GetMediaInfo", get_media_info},
	[TRANSPORT_CMD_SETAVTRANSPORTURI] =         {"SetAVTransportURI", set_avtransport_uri},	/* RC9800i */
	[TRANSPORT_CMD_SETNEXTAVTRANSPORTURI] =     {"SetNextAVTransportURI", set_next_avtransport_uri},
	[TRANSPORT_CMD_GETTRANSPORTINFO] =          {"GetTransportInfo", get_transport_info},
	[TRANSPORT_CMD_GETPOSITIONINFO] =           {"GetPositionInfo", get_position_info},
	[TRANSPORT_CMD_GETTRANSPORTSETTINGS] =      {"GetTransportSettings", get_transport_settings},
	[TRANSPORT_CMD_STOP] =                      {"Stop", stop},
	[TRANSPORT_CMD_PLAY] =                      {"Play", play},
	[TRANSPORT_CMD_PAUSE] =                     {"Pause", au_pause},	/* optional */
	[TRANSPORT_CMD_SEEK] =                      {"Seek", seek},
	[TRANSPORT_CMD_NEXT] =                      {"Next", next},
	[TRANSPORT_CMD_PREVIOUS] =                  {"Previous", previous},
	[TRANSPORT_CMD_SETPLAYMODE] =               {"SetPlayMode", NULL},	/* optional */
	[TRANSPORT_CMD_UNKNOWN] =                  {NULL, NULL}
};


struct service transport_service = {
        .service_name =         TRANSPORT_SERVICE,
        .type =                 TRANSPORT_TYPE,
	.scpd_url =		TRANSPORT_SCPD_URL,
        .control_url =		TRANSPORT_CONTROL_URL,
        .event_url =		TRANSPORT_EVENT_URL,
        .actions =              transport_actions,
        .action_arguments =     argument_list,
        .variable_names =       transport_variables,
        .variable_values =      transport_values,
        .variable_meta =        transport_var_meta,
        .variable_count =       TRANSPORT_VAR_UNKNOWN,
        .command_count =        TRANSPORT_CMD_UNKNOWN,
        .service_mutex =        &transport_mutex
};

