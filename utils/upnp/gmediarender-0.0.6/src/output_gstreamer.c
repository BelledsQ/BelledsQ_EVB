/* output_gstreamer.c - Output module for GStreamer
 *
 * Copyright (C) 2005-2007   Ivo Clarysse
 *
 * Adapted to gstreamer-0.10 2006 David Siorpaes
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
#include <string.h>

#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "logging.h"
#include "upnp_connmgr.h"
#include "output_gstreamer.h"

static int IsPlay = 0;
static char *gsuri = NULL;
extern void change_var(struct action_event *event, int varnum,
		       char *new_value);

void output_set_uri(const char *uri)
{
    FILE *pFile;

	ENTER();
	//printf("%s: >>> setting uri to %s\n", __FUNCTION__, uri);

	if (gsuri != NULL)
	{
		free(gsuri);
	}
	gsuri = strdup(uri);
    pFile = fopen("/tmp/mpg123_status", "w");
    if(pFile)
        fclose(pFile);
    IsPlay = 0;
	LEAVE();
}

int output_play(void)
{
	char buf[1024];
    int fd = 0;
    int ret = 0;
    int result = -1;

	ENTER();
	//printf("%s: >>> play %s\n", __FUNCTION__, gsuri);

    memset(buf, 0x0, sizeof(buf));
    strcpy(buf, "q\n");
    if(strlen(buf))
    {
      fd = open("/tmp/mpg123_pipe",O_WRONLY| O_NONBLOCK, 0);
      if(fd != -1)
      {
        ret = write(fd,buf,strlen(buf)+1);
        if(ret <= 0)
          printf("write mpg123_pipe failed\n");
        close(fd);
      }
      else
        printf("open mpg123_pipe failed\n");
    }
    system("killall mpg123");
    memset(buf, 0x0, sizeof(buf));
    sprintf(buf, "/tmp/mpg123 %s &", gsuri);
  	system(buf);
    IsPlay = 1;
    result = 0;
	LEAVE();
	return result;
}

int output_stop(void)
{
	char buf[32];
    int fd = 0;
    int ret = 0;

    memset(buf, 0x0, sizeof(buf));
    strcpy(buf, "q\n");
    if(strlen(buf))
    {
      fd = open("/tmp/mpg123_pipe",O_WRONLY| O_NONBLOCK, 0);
      if(fd != -1)
      {
        ret = write(fd,buf,strlen(buf)+1);
        if(ret <= 0)
          printf("write mpg123_pipe failed\n");
        close(fd);
      }
      else
        printf("open mpg123_pipe failed\n");
    }
    system("killall mpg123");
    IsPlay = 0;
    return 0;
}

int output_pause(void)
{
    char buf[32];
    int fd = 0;
    int ret = 0;

    memset(buf, 0x0, sizeof(buf));
    strcpy(buf, "s\n");
    fd = open("/tmp/mpg123_pipe",O_WRONLY| O_NONBLOCK, 0);
    if(fd != -1)
    {
        ret = write(fd,buf,strlen(buf)+1);
        if(ret <= 0)
            printf("write mpg123_pipe failed\n");
        close(fd);
    }
    else
        printf("open mpg123_pipe failed\n");
    return 0;
}

int output_seek(double secs)
{
	
	char buf[1024];
    int fd = 0;
    int ret = 0;
    int result = -1;

	ENTER();
	printf("output_seek:%lf\n",secs);

    memset(buf, 0x0, sizeof(buf));
    strcpy(buf, "q\n");
    if(strlen(buf))
    {
      fd = open("/tmp/mpg123_pipe",O_WRONLY| O_NONBLOCK, 0);
      if(fd != -1)
      {
        ret = write(fd,buf,strlen(buf)+1);
        if(ret <= 0)
          printf("write mpg123_pipe failed\n");
        close(fd);
      }
      else
        printf("open mpg123_pipe failed\n");
    }
    system("killall mpg123");
    memset(buf, 0x0, sizeof(buf));
    sprintf(buf, "/tmp/mpg123 --secs %lf %s &", secs,gsuri);
	system(buf);
    IsPlay = 1;
    result = 0;
	LEAVE();
	return result;
}


int output_loop()
{
    FILE *pFile;
    char buff[1024];
    char sFilename[1024];
    char sTotal[16], sCurrent[16];
    char *vPtr;

    while(1)
    {
        if(IsPlay)
        {
            memset(sFilename, 0x0, sizeof(sFilename));
            memset(sTotal, 0x0, sizeof(sTotal));
            memset(sCurrent, 0x0, sizeof(sCurrent));
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
                        strcpy(sFilename, vPtr);
                    if(strlen(sFilename))
                    {
                        if(strstr(buff, "total"))
                            strcpy(sTotal, vPtr);
                        else if(strstr(buff, "current"))
                            strcpy(sCurrent, vPtr);
                        if(strlen(sTotal) && strlen(sCurrent))
                        {
                            if(!strcmp(sTotal, sCurrent))
                            {
                                IsPlay = 0;
                                change_var(NULL, 0, "STOPPED");
                            }
                        }

                    }
                }
                fclose(pFile);
            }
            else
            {
                printf("open file error\n");
            }
        }
        sleep(1);
    }
	return 0;
}


int output_gstreamer_init(void)
{
	ENTER();
	register_mime_type("audio/mpeg");
	LEAVE();
	return 0;
}
