/* upnp_connmgr.c - UPnP Connection Manager routines
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
#include "../../libupnp/include/upnp.h"
#include "../../libupnp/include/ithread.h"
#include "../../libupnp/include/upnptools.h"

#include "logging.h"

#include "upnp.h"
#include "upnp_device.h"
#include "upnp_connmgr.h"

#define CONNMGR_SERVICE "urn:upnp-org:serviceId:ConnectionManager"
#define CONNMGR_TYPE	"urn:schemas-upnp-org:service:ConnectionManager:1"
#define CONNMGR_SCPD_URL "/upnp/renderconnmgrSCPD.xml"
#define CONNMGR_CONTROL_URL "/upnp/control/renderconnmgr1"
#define CONNMGR_EVENT_URL "/upnp/event/renderconnmgr1"


typedef enum {
	CONNMGR_VAR_AAT_CONN_MGR,
	CONNMGR_VAR_SINK_PROTO_INFO,
	CONNMGR_VAR_AAT_CONN_STATUS,
	CONNMGR_VAR_AAT_AVT_ID,
	CONNMGR_VAR_AAT_DIR,
	CONNMGR_VAR_AAT_RCS_ID,
	CONNMGR_VAR_AAT_PROTO_INFO,
	CONNMGR_VAR_AAT_CONN_ID,
	CONNMGR_VAR_SRC_PROTO_INFO,
	CONNMGR_VAR_CUR_CONN_IDS,
	CONNMGR_VAR_UNKNOWN,
	CONNMGR_VAR_COUNT
} connmgr_variable;

typedef enum {
	CONNMGR_CMD_GETCURRENTCONNECTIONIDS,
	CONNMGR_CMD_SETCURRENTCONNECTIONINFO,
	CONNMGR_CMD_GETPROTOCOLINFO,
	CONNMGR_CMD_UNKNOWN,
	CONNMGR_CMD_COUNT
} connmgr_cmd;

static struct action connmgr_actions[];

static struct argument *arguments_getprotocolinfo[] = {
	& (struct argument) { "Source", PARAM_DIR_OUT, CONNMGR_VAR_SRC_PROTO_INFO },
	& (struct argument) { "Sink", PARAM_DIR_OUT, CONNMGR_VAR_SINK_PROTO_INFO },
        NULL
};
static struct argument *arguments_getcurrentconnectionids[] = {
	& (struct argument) { "ConnectionIDs", PARAM_DIR_OUT, CONNMGR_VAR_CUR_CONN_IDS },
        NULL
};
static struct argument *arguments_setcurrentconnectioninfo[] = {
	& (struct argument) { "ConnectionID", PARAM_DIR_IN, CONNMGR_VAR_AAT_CONN_ID },
	& (struct argument) { "RcsID", PARAM_DIR_OUT, CONNMGR_VAR_AAT_RCS_ID },
	& (struct argument) { "AVTransportID", PARAM_DIR_OUT, CONNMGR_VAR_AAT_AVT_ID },
	& (struct argument) { "ProtocolInfo", PARAM_DIR_OUT, CONNMGR_VAR_AAT_PROTO_INFO },
	& (struct argument) { "PeerConnectionManager", PARAM_DIR_OUT, CONNMGR_VAR_AAT_CONN_MGR },
	& (struct argument) { "PeerConnectionID", PARAM_DIR_OUT, CONNMGR_VAR_AAT_CONN_ID },
	& (struct argument) { "Direction", PARAM_DIR_OUT, CONNMGR_VAR_AAT_DIR },
	& (struct argument) { "Status", PARAM_DIR_OUT, CONNMGR_VAR_AAT_CONN_STATUS },
        NULL
};

static struct argument **argument_list[] = {
	[CONNMGR_CMD_GETPROTOCOLINFO] =			arguments_getprotocolinfo,           
	[CONNMGR_CMD_GETCURRENTCONNECTIONIDS] =		arguments_getcurrentconnectionids,
	[CONNMGR_CMD_SETCURRENTCONNECTIONINFO] =	arguments_setcurrentconnectioninfo,
	[CONNMGR_CMD_UNKNOWN]	=	NULL
};

static const char *connmgr_variables[] = {
	[CONNMGR_VAR_SRC_PROTO_INFO] = "SourceProtocolInfo",
	[CONNMGR_VAR_SINK_PROTO_INFO] = "SinkProtocolInfo",
	[CONNMGR_VAR_CUR_CONN_IDS] = "CurrentConnectionIDs",
	[CONNMGR_VAR_AAT_CONN_STATUS] = "A_ARG_TYPE_ConnectionStatus",
	[CONNMGR_VAR_AAT_CONN_MGR] = "A_ARG_TYPE_ConnectionManager",
	[CONNMGR_VAR_AAT_DIR] = "A_ARG_TYPE_Direction",
	[CONNMGR_VAR_AAT_PROTO_INFO] = "A_ARG_TYPE_ProtocolInfo",
	[CONNMGR_VAR_AAT_CONN_ID] = "A_ARG_TYPE_ConnectionID",
	[CONNMGR_VAR_AAT_AVT_ID] = "A_ARG_TYPE_AVTransportID",
	[CONNMGR_VAR_AAT_RCS_ID] = "A_ARG_TYPE_RcsID",
	[CONNMGR_VAR_UNKNOWN] = NULL
};

static char *connmgr_values[] = {
	[CONNMGR_VAR_SRC_PROTO_INFO] = "",
	[CONNMGR_VAR_SINK_PROTO_INFO] = "http-get:*:audio/mpeg:*",
	[CONNMGR_VAR_CUR_CONN_IDS] = "0",
	[CONNMGR_VAR_AAT_CONN_STATUS] = "Unknown",
	[CONNMGR_VAR_AAT_CONN_MGR] = "/",
	[CONNMGR_VAR_AAT_DIR] = "Input",
	[CONNMGR_VAR_AAT_PROTO_INFO] = ":::",
	[CONNMGR_VAR_AAT_CONN_ID] = "-1",
	[CONNMGR_VAR_AAT_AVT_ID] = "0",
	[CONNMGR_VAR_AAT_RCS_ID] = "0",
	[CONNMGR_VAR_UNKNOWN] = NULL
};

static const char *connstatus_values[] = {
	"OK",
	"ContentFormatMismatch",
	"InsufficientBandwidth",
	"UnreliableChannel",
	"Unknown",
	NULL
};
static const char *direction_values[] = {
	"Input",
	"Output",
	NULL
};

static struct var_meta connmgr_var_meta[] = {
	[CONNMGR_VAR_SRC_PROTO_INFO] =	{ SENDEVENT_YES, DATATYPE_STRING, NULL, NULL },
	[CONNMGR_VAR_SINK_PROTO_INFO] =	{ SENDEVENT_YES, DATATYPE_STRING, NULL, NULL },
	[CONNMGR_VAR_CUR_CONN_IDS] =	{ SENDEVENT_YES, DATATYPE_STRING, NULL, NULL },
	[CONNMGR_VAR_AAT_CONN_STATUS] =	{ SENDEVENT_NO, DATATYPE_STRING, connstatus_values, NULL },
	[CONNMGR_VAR_AAT_CONN_MGR] =	{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[CONNMGR_VAR_AAT_DIR] =		{ SENDEVENT_NO, DATATYPE_STRING, direction_values, NULL },
	[CONNMGR_VAR_AAT_PROTO_INFO] =	{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[CONNMGR_VAR_AAT_CONN_ID] =	{ SENDEVENT_NO, DATATYPE_I4, NULL, NULL },
	[CONNMGR_VAR_AAT_AVT_ID] =	{ SENDEVENT_NO, DATATYPE_I4, NULL, NULL },
	[CONNMGR_VAR_AAT_RCS_ID] =	{ SENDEVENT_NO, DATATYPE_I4, NULL, NULL },
	[CONNMGR_VAR_UNKNOWN] =		{ SENDEVENT_NO, DATATYPE_UNKNOWN, NULL, NULL }
};

static ithread_mutex_t connmgr_mutex;


struct mime_type;
static struct mime_type {
	const char *mime_type;
	struct mime_type *next;
} *supported_types = NULL;

static void register_mime_type_internal(const char *mime_type)
{
	struct mime_type *entry;

	for (entry = supported_types; entry; entry = entry->next) {
		if (strcmp(entry->mime_type, mime_type) == 0) {
			return;
		}
	}
	printf("Registering support for '%s'\n", mime_type);

	entry = malloc(sizeof(struct mime_type));
	entry->mime_type = strdup(mime_type);
	entry->next = supported_types;
	supported_types = entry;
}
void register_mime_type(const char *mime_type)
{
	register_mime_type_internal(mime_type);
	//if (strcmp("audio/mpeg", mime_type) == 0) 
	{
		register_mime_type_internal("audio/x-mpeg");
		// BubbleUPnP uses audio/x-scpl as an indicator to know if the
		// renderer can handle it (otherwise it will proxy).
		// Simple claim: if we can handle mpeg, then we can handle
		// shoutcast.
		// (For more accurate answer: we'd to check if all of
		// mpeg, aac, aacp, ogg are supported).
		register_mime_type_internal("audio/x-scpls");

		// This is apparently something sent by the spotifyd
		// https://gitorious.org/spotifyd
		//register_mime_type("audio/L16;rate=44100;channels=2");
		register_mime_type_internal("audio/L16;rate=44100;channels=2");
	}

	// Some workaround: some controllers seem to match the version without
	// x-, some with; though the mime-type is correct with x-, these formats
	// seem to be common enough to sometimes be used without.
	// If this works, we should probably collect all of these
	// in a set emit always both, foo/bar and foo/x-bar, as it is a similar
	// work-around as seen above with mpeg -> x-mpeg.
	//if (strcmp("audio/x-alac", mime_type) == 0) 
	{
	  register_mime_type_internal("audio/alac");
	}
	//if (strcmp("audio/x-aiff", mime_type) == 0) 
	{
	  register_mime_type_internal("audio/aiff");
	}
	//if (strcmp("audio/x-m4a", mime_type) == 0) 
	{
	  register_mime_type_internal("audio/m4a");
	  register_mime_type_internal("audio/mp4");
	  register_mime_type_internal("audio/aaC");
	  register_mime_type_internal("audio/aac-adts");
	  register_mime_type_internal("audio/mp4a");
	}
}

int connmgr_init(void)
{
	struct mime_type *entry;
	char *buf = NULL;
	char *p;
	int offset;
	int bufsize = 0;
	int result = -1;

	ENTER();

	buf = malloc(bufsize);
	p = buf;
	if (buf == NULL) {
		fprintf(stderr, "%s: initial malloc failed\n",
			__FUNCTION__);
		goto out;
	}

	for (entry = supported_types; entry; entry = entry->next) {
		bufsize += strlen(entry->mime_type) + 1 + 8 + 3 + 2;
		offset = p - buf;
		buf = realloc(buf, bufsize);
		if (buf == NULL) {
			fprintf(stderr, "%s: realloc failed\n",
				__FUNCTION__);
			goto out;
		}
		p = buf;
		p += offset;
		strncpy(p, "http-get:*:", 11);
		p += 11;
		strncpy(p, entry->mime_type, strlen(entry->mime_type));
		p += strlen(entry->mime_type);
		strncpy(p, ":*,", 3);
		p += 3;
	}
	if (p > buf) {
		p--;
		*p = '\0';
	}
	*p = '\0';

	connmgr_values[CONNMGR_VAR_SINK_PROTO_INFO] = buf;

	result = 0;
out:
	LEAVE();
	return result;
}


static int get_protocol_info(struct action_event *event)
{
	ENTER();
	upnp_append_variable(event, CONNMGR_VAR_SRC_PROTO_INFO, "Source");
	upnp_append_variable(event, CONNMGR_VAR_SINK_PROTO_INFO, "Sink");
	LEAVE();
	return event->status;
}

static int get_current_conn_ids(struct action_event *event)
{
	int rc;
	ENTER();
	upnp_add_response(event, "ConnectionIDs", "0");
	LEAVE();
	return rc;
}

static int get_current_conn_info(struct action_event *event)
{
	int rc;
	char *value;

	ENTER();
	value = upnp_get_string(event, "ConnectionID");
	if (value == NULL) {
		rc = -1;
		goto out;
	}
	printf("%s: ConnectionID='%s'\n", __FUNCTION__, value);
	free(value);

	rc = upnp_append_variable(event, CONNMGR_VAR_AAT_RCS_ID, "RcsID");
	if (rc)
		goto out;
	rc = upnp_append_variable(event, CONNMGR_VAR_AAT_AVT_ID,
				  "AVTransportID");
	if (rc)
		goto out;
	rc = upnp_append_variable(event, CONNMGR_VAR_AAT_PROTO_INFO,
				  "ProtocolInfo");
	if (rc)
		goto out;
	rc = upnp_append_variable(event, CONNMGR_VAR_AAT_CONN_MGR,
				  "PeerConnectionManager");
	if (rc)
		goto out;
	rc = upnp_append_variable(event, CONNMGR_VAR_AAT_CONN_ID,
				  "PeerConnectionID");
	if (rc)
		goto out;
	rc = upnp_append_variable(event, CONNMGR_VAR_AAT_DIR, "Direction");
	if (rc)
		goto out;
	rc = upnp_append_variable(event, CONNMGR_VAR_AAT_CONN_STATUS,
				  "Status");
	if (rc)
		goto out;

      out:
	LEAVE();
	return rc;
}


static struct action connmgr_actions[] = {
	[CONNMGR_CMD_GETPROTOCOLINFO] =		{"GetProtocolInfo", get_protocol_info},
	[CONNMGR_CMD_GETCURRENTCONNECTIONIDS] =	{"GetCurrentConnectionIDs", get_current_conn_ids},
	[CONNMGR_CMD_SETCURRENTCONNECTIONINFO] ={"GetCurrentConnectionInfo", get_current_conn_info},
	[CONNMGR_CMD_UNKNOWN] =			{NULL, NULL}
};

struct service connmgr_service = {
        .service_name =		CONNMGR_SERVICE,
        .type =			CONNMGR_TYPE,
	.scpd_url =		CONNMGR_SCPD_URL,
	.control_url =		CONNMGR_CONTROL_URL,
	.event_url =		CONNMGR_EVENT_URL,
        .actions =		connmgr_actions,
        .action_arguments =     argument_list,
        .variable_names =       connmgr_variables,
        .variable_values =      connmgr_values,
        .variable_meta =        connmgr_var_meta,
        .variable_count =       CONNMGR_VAR_UNKNOWN,
        .command_count =        CONNMGR_CMD_UNKNOWN,
        .service_mutex =        &connmgr_mutex
};

