/* upnp.c - Generic UPnP routines
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
#include <errno.h>
#include <stdarg.h>

#include "../../libupnp/include/ixml.h"
#include "../../libupnp/include/ithread.h"

#include "logging.h"

#include "upnp.h"

static const char *param_datatype_names[] = {
        [DATATYPE_STRING] =     "string",
        [DATATYPE_BOOLEAN] =    "boolean",
        [DATATYPE_I2] =         "i2",
        [DATATYPE_I4] =         "i4",
        [DATATYPE_UI2] =        "ui2",
        [DATATYPE_UI4] =        "ui4",
        [DATATYPE_UNKNOWN] =    NULL
};

static void add_value_attribute(IXML_Document *doc, IXML_Element *parent,
                                char *attrname, char *value)
{
	ixmlElement_setAttribute(parent, attrname, value);
}

static void add_value_element(IXML_Document *doc, IXML_Element *parent,
                              char *tagname, char *value)
{
	IXML_Element *top;
	IXML_Node *child;

	top=ixmlDocument_createElement(doc, tagname);
	child=ixmlDocument_createTextNode(doc, value);

	ixmlNode_appendChild((IXML_Node *)top,(IXML_Node *)child);

	ixmlNode_appendChild((IXML_Node *)parent,(IXML_Node *)top);
}
static void add_value_element_int(IXML_Document *doc, IXML_Element *parent,
                                  char *tagname, int value)
{
	char *buf;

	asprintf(&buf,"%d",value);
	add_value_element(doc, parent, tagname, buf);
	free(buf);
}
static void add_value_element_long(IXML_Document *doc, IXML_Element *parent,
                                  char *tagname, long long value)
{
	char *buf;

	asprintf(&buf,"%lld",value);
	add_value_element(doc, parent, tagname, buf);
	free(buf);
}

static IXML_Element *gen_specversion(IXML_Document *doc, int major, int minor)
{
	IXML_Element *top;

	top=ixmlDocument_createElement(doc, "specVersion");

	add_value_element_int(doc, top, "major", major);
	add_value_element_int(doc, top, "minor", minor);

	return top;
}

static IXML_Element *gen_scpd_action(IXML_Document *doc, struct action *act,
                                     struct argument **arglist,
                                     const char **varnames)
{
	IXML_Element *top;
	IXML_Element *parent,*child;

	top=ixmlDocument_createElement(doc, "action");

	add_value_element(doc, top, "name", (char *)act->action_name);
	if (arglist) {
		struct argument *arg;
		int j;
		parent=ixmlDocument_createElement(doc, "argumentList");
		ixmlNode_appendChild((IXML_Node *)top,(IXML_Node *)parent);
		for(j=0; (arg=arglist[j]); j++) {
			child=ixmlDocument_createElement(doc, "argument");
			ixmlNode_appendChild((IXML_Node *)parent,(IXML_Node *)child);
			add_value_element(doc,child,"name",(char *)arg->name);
			add_value_element(doc,child,"direction",(arg->direction==PARAM_DIR_IN)?"in":"out");
			add_value_element(doc,child,"relatedStateVariable",(char *)varnames[arg->statevar]);
		}
	}
	return top;
}

static IXML_Element *gen_scpd_actionlist(IXML_Document *doc,
                                         struct service *srv)
{
	IXML_Element *top;
	IXML_Element *child;
	int i;

	top=ixmlDocument_createElement(doc, "actionList");
	for(i=0; i<srv->command_count; i++) {
		struct action *act;
		struct argument **arglist;
		const char **varnames;
		act=&(srv->actions[i]);
		arglist=srv->action_arguments[i];
		varnames=srv->variable_names;
		if (act) {
			child=gen_scpd_action(doc, act, arglist, varnames);
			ixmlNode_appendChild((IXML_Node *)top,(IXML_Node *)child);
		}
	}
	return top;
}

static IXML_Element *gen_scpd_statevar(IXML_Document *doc, const char *name, struct var_meta *meta)
{
	IXML_Element *top,*parent;
	const char **valuelist;
	const char *default_value;
	struct param_range *range;

	valuelist = meta->allowed_values;
	range = meta->allowed_range;
	default_value = meta->default_value;


	top=ixmlDocument_createElement(doc, "stateVariable");

	add_value_attribute(doc, top, "sendEvents",(meta->sendevents==SENDEVENT_YES)?"yes":"no");
	add_value_element(doc,top,"name",(char *)name);
	add_value_element(doc,top,"dataType",(char *)param_datatype_names[meta->datatype]);

	if (valuelist) {
		const char *allowed_value;
		int i;
		parent=ixmlDocument_createElement(doc, "allowedValueList");
		ixmlNode_appendChild((IXML_Node *)top,(IXML_Node *)parent);
		for(i=0; (allowed_value=valuelist[i]); i++) {
			add_value_element(doc,parent,"allowedValue",(char *)allowed_value);
		} 
	}
	if (range) {
		parent=ixmlDocument_createElement(doc, "allowedValueRange");
		ixmlNode_appendChild((IXML_Node *)top,(IXML_Node *)parent);
		add_value_element_long(doc,parent,"minimum",range->min);
		add_value_element_long(doc,parent,"maximum",range->max);
		if (range->step != 0L) {
			add_value_element_long(doc,parent,"step",range->step);
		}
	}
	if (default_value) {
		add_value_element(doc,top,"defaultValue",(char *)default_value);
	}
	return top;
}

static IXML_Element *gen_scpd_servicestatetable(IXML_Document *doc, struct service *srv)
{
	IXML_Element *top;
	IXML_Element *child;
	int i;

	top=ixmlDocument_createElement(doc, "serviceStateTable");
	for(i=0; i<srv->variable_count; i++) {
		struct var_meta *meta = &(srv->variable_meta[i]);
		const char *name = srv->variable_names[i];
		child=gen_scpd_statevar(doc,name,meta);
		ixmlNode_appendChild((IXML_Node *)top,(IXML_Node *)child);
	}
	return top;
}

static IXML_Document *generate_scpd(struct service *srv)
{
	IXML_Document *doc;
	IXML_Element *root;
	IXML_Element *child;

	doc = ixmlDocument_createDocument();

	root=ixmlDocument_createElementNS(doc, "urn:schemas-upnp-org:service-1-0","scpd");
	ixmlElement_setAttribute(root, "xmlns", "urn:schemas-upnp-org:service-1-0");
	ixmlNode_appendChild((IXML_Node *)doc,(IXML_Node *)root);

	child=gen_specversion(doc,1,0);
	ixmlNode_appendChild((IXML_Node *)root,(IXML_Node *)child);

	child=gen_scpd_actionlist(doc,srv);
	ixmlNode_appendChild((IXML_Node *)root,(IXML_Node *)child);

	child=gen_scpd_servicestatetable(doc,srv);
	ixmlNode_appendChild((IXML_Node *)root,(IXML_Node *)child);
	
	
	return doc;
}

static IXML_Element *gen_desc_iconlist(IXML_Document *doc, struct icon **icons)
{
	IXML_Element *top;
	IXML_Element *parent;
	struct icon *icon_entry;
	int i;

	top=ixmlDocument_createElement(doc, "iconList");

	for (i=0; (icon_entry=icons[i]); i++) {
		parent=ixmlDocument_createElement(doc, "icon");
		ixmlNode_appendChild((IXML_Node *)top,(IXML_Node *)parent);
		add_value_element(doc,parent,"mimetype",(char *)icon_entry->mimetype);
		add_value_element_int(doc,parent,"width",icon_entry->width);
		add_value_element_int(doc,parent,"height",icon_entry->height);
		add_value_element_int(doc,parent,"depth",icon_entry->depth);
		add_value_element(doc,parent,"url",(char *)icon_entry->url);
	}

	return top;
}

static IXML_Element *gen_desc_servicelist(struct device *device_def,
                                          IXML_Document *doc)
{
	int i;
	struct service *srv;
	IXML_Element *top;
	IXML_Element *parent;

	top=ixmlDocument_createElement(doc, "serviceList");

        for (i=0; (srv = device_def->services[i]); i++) {
		parent=ixmlDocument_createElement(doc, "service");
		ixmlNode_appendChild((IXML_Node *)top,(IXML_Node *)parent);
		add_value_element(doc,parent,"serviceType",srv->type);
		add_value_element(doc,parent,"serviceId",(char *)srv->service_name);
		add_value_element(doc,parent,"SCPDURL",(char *)srv->scpd_url);
		add_value_element(doc,parent,"controlURL",(char *)srv->control_url);
		add_value_element(doc,parent,"eventSubURL",(char *)srv->event_url);
        }

	return top;
}


static IXML_Document *generate_desc(struct device *device_def)
{
	IXML_Document *doc;
	IXML_Element *root;
	IXML_Element *child;
	IXML_Element *parent;

	doc = ixmlDocument_createDocument();

	root=ixmlDocument_createElementNS(doc, "urn:schemas-upnp-org:device-1-0","root");
	ixmlElement_setAttribute(root, "xmlns", "urn:schemas-upnp-org:device-1-0");
	ixmlNode_appendChild((IXML_Node *)doc,(IXML_Node *)root);
	child=gen_specversion(doc,1,0);
	ixmlNode_appendChild((IXML_Node *)root,(IXML_Node *)child);
	parent=ixmlDocument_createElement(doc, "device");
	ixmlNode_appendChild((IXML_Node *)root,(IXML_Node *)parent);
	add_value_element(doc,parent,"dlna:X_DLNADOC","DMR-1.50");
	add_value_element(doc,parent,"deviceType",(char *)device_def->device_type);
	add_value_element(doc,parent,"presentationURL",(char *)device_def->presentation_url);
	add_value_element(doc,parent,"friendlyName",(char *)device_def->friendly_name);
	add_value_element(doc,parent,"manufacturer",(char *)device_def->manufacturer);
	add_value_element(doc,parent,"manufacturerURL",(char *)device_def->manufacturer_url);
	add_value_element(doc,parent,"modelDescription",(char *)device_def->model_description);
	add_value_element(doc,parent,"modelName",(char *)device_def->model_name);
	add_value_element(doc,parent,"modelURL",(char *)device_def->model_url);
	add_value_element(doc,parent,"UDN",(char *)device_def->udn);
	add_value_element(doc,parent,"modelNumber",(char *)device_def->model_number);
	add_value_element(doc,parent,"serialNumber",(char *)device_def->serial_number);
	if (device_def->icons) {
		child=gen_desc_iconlist(doc,device_def->icons);
		ixmlNode_appendChild((IXML_Node *)parent,(IXML_Node *)child);
	}
	child=gen_desc_servicelist(device_def, doc);
	ixmlNode_appendChild((IXML_Node *)parent,(IXML_Node *)child);

	return doc;
}


struct service *find_service(struct device *device_def,
                             char *service_name)
{
	struct service *event_service;
	int serviceNum = 0;
	while (event_service =
	       device_def->services[serviceNum], event_service != NULL) {
		if (strcmp(event_service->service_name, service_name) == 0)
			return event_service;
		serviceNum++;
	}
	return NULL;
}

struct action *find_action(struct service *event_service,
				  char *action_name)
{
	struct action *event_action;
	int actionNum = 0;
	if (event_service == NULL)
		return NULL;
	while (event_action =
	       &(event_service->actions[actionNum]),
	       event_action->action_name != NULL) {
		if (strcmp(event_action->action_name, action_name) == 0)
			return event_action;
		actionNum++;
	}
	return NULL;
}

char *upnp_get_scpd(struct service *srv)
{
	char *result = NULL;
	IXML_Document *doc;

	doc = generate_scpd(srv);
	if (doc != NULL)
	{
       		result = ixmlDocumenttoString(doc);
		ixmlDocument_free(doc);
	}
	return result;
}

char *upnp_get_device_desc(struct device *device_def)
{
	char *result = NULL;
	IXML_Document *doc;

	doc = generate_desc(device_def);

	if (doc != NULL)
	{
       		result = ixmlDocumenttoString(doc);
		ixmlDocument_free(doc);
	}
	return result;
}

