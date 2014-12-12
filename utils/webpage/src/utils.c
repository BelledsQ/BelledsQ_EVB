/*************************************************************************
    > File Name: utils.c
    > Author: yuehb
    > Mail: yuehb@bellnett.com 
    > Created Time: Wed 09 Jul 2014 04:57:05 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "utils.h"
#include <sys/wait.h> 

/*
 *  LineToStr - Scans char and replaces the first "\n" with a "\0";
 *  If it finds a "\r", it does that to; (fix DOS brokennes) returns
 *	the length of the string;
 */
int LineToStr (char *string, size_t max) {
	size_t offset=0;
	while ((offset < max) && (string[offset] != '\n') && (string[offset] != '\r')) {
		offset++;
	}
	if (string[offset] == '\r') {
		string[offset]='\0';
		offset++;
	}
	if (string[offset] == '\n') {
		string[offset]='\0';
		offset++;
	}
	return (offset);
}


/*
 *  Reads file and returns contents
 *  @param	fd	file descriptor
 *  @return	contents of file or NULL if an error occurred
 */
char *fd2str(int fd)
{
	char *buf = NULL;
	size_t count = 0, n;

	do {
		buf = realloc(buf, count + MAX_OUTBUF_LEN);
		n = read(fd, buf + count, MAX_OUTBUF_LEN);
		if (n < 0) {
			free(buf);
			buf = NULL;
		}
		count += n;
	} while (n == MAX_OUTBUF_LEN);

	close(fd);
	if (buf)
		buf[count] = '\0';
	return buf;
}

/*	Reads file and returns contents
 *	@param	path	path to file
 *  @return	contents of file or NULL if an error occurred
 */
char *file2str(const char *path)
{
	int fd;

	if ((fd = open(path, O_RDONLY)) == -1) {
		perror(path);
		return NULL;
	}

	return fd2str(fd);
}

/* 
 *  Concatenates NULL-terminated list of arguments into a single
 *  commmand and executes it
 *	@param	argv	argument list
 *  @param	path	NULL, ">output", or ">>output"
 *	@param	timeout	seconds to wait before timing out or 0 for no timeout
 *  @param	ppid	NULL to wait for child termination or pointer to pid
 *	@return	return value of executed command or errno
 */
int _eval(char *const argv[], const char *path, int timeout, int *ppid)
{
	sigset_t set, omask;
	pid_t pid, ret;
	int status;
	int fd;
	int flags;
	int sig;

	/* Block SIGCHLD signal to avoid interaction with its handler */
	if (ppid == NULL) {
		sigemptyset(&set);
		sigaddset(&set, SIGCHLD);
		sigprocmask(SIG_BLOCK, &set, &omask);
	}

	switch (pid = fork()) {
	case -1:	/* error */
		perror("fork");
		return errno;
	case 0:		/* child */
		/* Reset signal handlers set for parent process */
		for (sig = 0; sig < (_NSIG-1); sig++)
			signal(sig, SIG_DFL);

		/* Unblock signals if called from signal handler */
		sigemptyset(&set);
		sigprocmask(SIG_SETMASK, &set, NULL);

		/* Clean up */
		ioctl(0, TIOCNOTTY, 0);
		close(STDIN_FILENO);
		setsid();

		fd = open("/dev/null", O_RDWR); /* stdin */

		/* Redirect stdout to <path> */
		if (path) {
			flags = O_WRONLY | O_CREAT;
			if (!strncmp(path, ">>", 2)) {
				/* append to <path> */
				flags |= O_APPEND;
				path += 2;
			} else if (!strncmp(path, ">", 1)) {
				/* overwrite <path> */
				flags |= O_TRUNC;
				path += 1;
			}
			if ((fd = open(path, flags, 0644)) < 0)
				perror(path);
			else {
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				close(fd);
			}
		} else {
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
		}

#if 0
	{
		char **ptr, tmp[1024] = "";
		for (ptr = (char **)argv; *ptr; ptr++)
			snprintf(tmp, sizeof(tmp), "%s%s ", tmp, *ptr);
		cprintf("<%d> %s\n", getpid(), tmp);
	}
#endif
		setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);
		alarm(timeout);

		/* execute command */
		execvp(argv[0], argv);
		perror(argv[0]);
		_exit(errno);
	default:	/* parent */
		if (ppid != NULL) {
			*ppid = pid;
			return 0;
		} else {
			do
				ret = waitpid(pid, &status, 0);
			while ((ret == -1) && (errno == EINTR));

			// Restore signals, errno should be preserved
						flags = errno;
			sigprocmask(SIG_SETMASK, &omask, NULL);
			errno = flags;

			if (ret != pid) {
				cprintf("<%d> waitpid: %s\n", pid, strerror(errno));
				perror("waitpid");
				return errno;
			}
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
		}
	}
}

/*uci options, get a uci param value from the uci file*/
char* uci_get(char* name)
{
        FILE* fp=NULL;
        char cmd[CMD_BUF];
        sprintf(cmd,"rm %s ; uci get %s |cat > %s",TMP_UCI_FILE,name,TMP_UCI_FILE);
        system(cmd);
	
		char* uci_buf = file2str(TMP_UCI_FILE);
		if(strlen(uci_buf)>0){
				//cprintf("uci_buf=%s\n",uci_buf);
				uci_buf[strlen(uci_buf)-1]='\0';
		}else{
			free(uci_buf);
			return NULL;
		}
		return uci_buf;
}

void uci_set(char* name,char* value)
{
	char cmd[CMD_BUF];
	memset(cmd,0,CMD_BUF);
	
	sprintf(cmd, "uci set %s=%s", name,value);
	system(cmd);
}

void uci_saveset(char* name,char* value)
{
	char cmd[CMD_BUF];
	memset(cmd,0,CMD_BUF);
	sprintf(cmd, "uci set %s=%s && uci commit", name,value);
	system(cmd);
}

void uci_commit()
{
	system("uci commit");
}

void get_wl_dhcpinfo(DHCPINFO* dhcpinfo)
{
	char *udhcpc_argv[] = {"udhcpc", "-i", "wlan0-1","-s", "/usr/web/cgi-bin/dhcp.script", "-t","0","-C", "-q","-n",NULL};
	pid_t pid = 0;

    cprintf("********get_wl_dhcpinfo begin********\n");
	eval("rm",TMP_WLDHCP_FILE);
	_eval(udhcpc_argv, NULL, 0, &pid);

	sleep(2);
	char* data = file2str(TMP_WLDHCP_FILE);
	char* ip=NULL,*subnet=NULL,*gateway=NULL,*dns1=NULL,*dns2=NULL;
	cprintf("data:%s\n",data);
	if(data&&(ip=strstr(data,"ip:")))	
	{
		ip=ip+strlen("ip:");
		if(subnet=strstr(ip,"subnet:")){	
			subnet--;*subnet='\0';subnet=subnet+strlen("subnet:")+1;
			if(gateway=strstr(subnet,"gateway:")){	
				gateway--;*gateway='\0';gateway=gateway+strlen("gateway:")+1;
				if(dns1=strstr(gateway,"dns:")){	
					dns1--;*dns1='\0';dns1=dns1+strlen("dns:")+1;
					dns2=strchr(dns1,' ');
					if(dns2){*dns2='\0';dns2++;
					dns2[strlen(dns2)]='\0';}else{dns2=strchr(dns1,'\n');if(dns2)*dns2='\0';}
				}
			}
			
		}
	
		cprintf("IP:%sMask:%sGateway:%sDns1:%sDns2:%s\n",ip,subnet,gateway,dns1,dns2);
		if(ip)strcpy(dhcpinfo->ip,ip);
		if(subnet)strcpy(dhcpinfo->subnet,subnet);
		if(gateway)strcpy(dhcpinfo->gateway,gateway);
		if(dns1)strcpy(dhcpinfo->dns1,dns1);
		if(dns2)strcpy(dhcpinfo->dns2,dns2);
	}

	if(data)free(data);	
    cprintf("********get_wl_dhcpinfo end********\n");
}


void* safe_malloc (size_t size) 
{
	void * retval = NULL;
	retval = malloc(size);
	if (!retval) {
		cprintf("Failed to malloc %d bytes of memory: %s.  Bailing out", size, strerror(errno));
		exit(1);
	}
	return (retval);
}
#if 0
static char* find_token(char*data)
{
	char* tp=NULL;
	if((tp=strstr(data,"(on wlan0-1)")))
	{
		tp=tp-strlen("BSS 8c:be:be:20:ad:f2 ");
		*tp='\0';
		return tp;
	}
}

static char* parse_one(char* data,APINFO** ap_list)
{
	char* tp1=NULL,*tp2=NULL,*tp3=NULL;
	char* ssid=NULL,*mac=NULL,*channel=NULL,*rssi=NULL;
	int encrypt=0,tkpi_aes=0,is_connected=0;
	
	//cprintf("data:%s\n",data);
	if((tp1=strstr(data,"(on wlan0-1)")))
	{	
		tp3=tp1+strlen("(on wlan0-1)");
		tp1=tp1-strlen("BSS 8c:be:be:20:ad:f2 ");
		tp2=find_token(tp3);		

		mac=tp1+strlen("BSS ");
		if(tp3=strstr(mac," (on wlan0-1)"))
			*tp3='\0';
		//cprintf("MAC:%s\n",mac);
		tp1=tp3+1;
		
		if(strstr(tp1,"associated"))
		{
			//cprintf("Connected\n");
			is_connected=1;
		}

		if(tp1=strstr(tp1,"signal: "))
			rssi=tp1+strlen("signal: ");
		if(tp3=strstr(rssi," dBm"))
			*tp3='\0';
		//cprintf("RSSI:%s\n",rssi);
		tp1=tp3+1;

		
		if(tp1=strstr(tp1,"SSID: "))	
			ssid =tp1 + strlen("SSID: ");
		if(tp3=strchr(ssid,'\n'))
			*tp3='\0';
		//cprintf("SSID:%s\n",ssid);
		if(strlen(ssid)<=0)
		{
			if(tp2)*tp2='B';
			return tp2;
		}
		tp1=tp3+1;
		
		if(tp1=strstr(tp1,"DS Parameter set: channel "))	
			channel =tp1 + strlen("DS Parameter set: channel ");
		if(tp3=strchr(channel,'\n'))
			*tp3='\0';
		//cprintf("CHANNEL:%s\n",channel);
		tp1=tp3+1;
	
		if(tp3=strstr(tp1,"RSN:"))
		{
			//cprintf("WPA2\n");
			encrypt|=WPA2_PSK;
			if(tp3=strstr(tp1,"* Pairwise ciphers: "))
			{
				if(strstr(tp3,"CCMP"))
				{	
					//cprintf("aes\n");
					tkpi_aes|= AES;
				}	
				if((tp3,"TKIP"))
				{
					//cprintf("tkip\n");
					tkpi_aes|= TKIP;
				}
			}
		}
		if(strstr(tp1,"WPA:"))
		{
			//cprintf("WPA\n");
			encrypt|=WPA_PSK;
			if(tp3=strstr(tp1,"* Pairwise ciphers: "))
			{
				if(strstr(tp3,"CCMP"))
				{	
					//cprintf("aes\n");
					tkpi_aes |= AES;
				}	
				if((tp3,"TKIP"))
				{
					//cprintf("tkip\n");
					tkpi_aes |= TKIP;
				}
			}
		}
		
		char* encryption=NULL;
		//cprintf("encrypt=%d\n",encrypt);
		if((encrypt&WPA2_PSK)&&(encrypt&WPA_PSK))encryption="WPA/WPA2-PSK";
		else if((encrypt&WPA2_PSK))encryption="WPA2-PSK";
		else if((encrypt&WPA_PSK))encryption="WPA-PSK";
		else
			encryption="NONE";//WEP?
		
		char* _tkpi_aes=NULL;
		if((tkpi_aes&AES)&& (tkpi_aes&TKIP))_tkpi_aes="tkpi/aes";
		else if((tkpi_aes&TKIP))_tkpi_aes="tkpi";
		else if((tkpi_aes&AES))_tkpi_aes="aes";

		//cprintf("SSID:%s\nMAC:%s\nCHANNEL:%s\nRSSI:%s\nEncrypt:%s\nTKIP-AES:%s\n",ssid,mac,channel,rssi,encryption,_tkpi_aes);
		APINFO* tmp=(APINFO*)safe_malloc(sizeof(APINFO));	
		memset(tmp,0,sizeof(APINFO));
		strcpy(tmp->ssid,ssid);
		strcpy(tmp->mac,mac);
		strcpy(tmp->channel,channel);
		strcpy(tmp->rssi,rssi);
		strcpy(tmp->encrypt,encryption);
		if(_tkpi_aes)strcpy(tmp->tkip_aes,_tkpi_aes);
		tmp->is_connected = is_connected;
		
		if(NULL==*ap_list)
			*ap_list=tmp;
		else
		{
			APINFO* last=*ap_list;
			while(last->next)last=last->next;
			last->next=tmp;
		}
		/*
		APINFO* gg = *ap_list;
		while(gg)
		{
			cprintf("AAA:%s\n%s\n",gg->ssid,gg->mac);
			gg=gg->next;
		}*/
	}

	if(tp2)*tp2='B';
	return tp2;
}

void get_wlscaninfo(APINFO** apinfo)
{
	char *scan_argv[] = {"iw","dev","wlan0-1","scan",NULL};
	pid_t pid = 0;
	_eval(scan_argv, ">"TMP_WLSCAN_FILE, 0, &pid);

    cprintf("+++++scan end++++++++\n");
	//system("rm "TMP_WLSCAN_FILE"; iw dev wlan0-1 scan |cat > "TMP_WLSCAN_FILE);

	char* data = file2str(TMP_WLSCAN_FILE);
	char* new = data;
	while(new)new=parse_one(new,apinfo);
	/*
	APINFO* apinfo1 = *apinfo; 
	while(apinfo1){
		cprintf("NEWApinfo:%s\n%s\n",apinfo1->ssid,apinfo1->mac);
		cprintf("%s\n%s\n",apinfo1->channel,apinfo1->rssi);
		cprintf("%s\n%s\n",apinfo1->encrypt,apinfo1->tkip_aes);
		cprintf("%d\n\n",apinfo1->is_connected);
		
		apinfo1=apinfo1->next;
	}*/

	free(data);
}
#else

static void get_linkinfo(char* mac)
{
	char* link = file2str(TMP_WLLINK_FILE);
	char *tp=NULL,*tp2=NULL;
	if(tp=strstr(link,"Connected to "))
	{
		tp+=strlen("Connected to ");
		if(tp2=strstr(tp," (on wlan0-1)"))
			*tp2='\0';
		//cprintf("MAC:%s\n",tp);
		strcpy(mac,tp);
	}
	free(link);
}

static char* parse_line(char* data,APINFO** ap_list,const char* linkinfo)
{
	char *tp=NULL,*tp1=NULL,*tp2=NULL,*tp3=NULL;
	char* ssid=NULL,*mac=NULL,*channel=NULL,*rssi=NULL,*encrypt=NULL,*tkip_aes=NULL;

	//cprintf("data=%s\n",data);
	if(data &&(tp1=strstr(data,"Cell ")))
	{
		tp3=tp1+strlen("Cell ");
		if(tp2=strstr(tp3,"Cell "))
			*tp2='\0';
		else
			tp2=NULL;
		
		if(mac=strstr(tp1,"Address: "))
		{
			mac+=strlen("Address: ");
			tp=strchr(mac,'\n');
			*tp='\0';
			tp1=tp+1;
		}
		//cprintf("MAC=%s\n",mac);

		if(ssid=strstr(tp1,"ESSID: \""))
		{
			ssid+=strlen("ESSID: \"");
			tp=strchr(ssid,'\"');
			*tp='\0';
			tp1=tp+1;
		}
		//cprintf("SSID=%s\n",ssid);

		if(channel=strstr(tp1,"Channel: "))
		{
			channel+=strlen("Channel: ");
			tp=strchr(channel,'\n');
			*tp='\0';
			tp1=tp+1;
		}
		//cprintf("Channel=%s\n",channel);

		if(rssi=strstr(tp1,"Signal: "))
		{
			rssi+=strlen("Signal: ");
			tp=strchr(rssi,' ');
			*tp='\0';
			tp1=tp+1;
		}
		//cprintf("RSSI=%s\n",rssi);

		if(encrypt=strstr(tp1,"Encryption: "))
		{
			encrypt+=strlen("Encryption: ");
			tp=strchr(encrypt,'(');
			if(tp)
			{
				*tp='\0';
				tp1=tp+1;
			}else{
				tp=strchr(encrypt,'\n');
				*tp='\0';
				tp1=NULL;
			}
		}
		//cprintf("Encryption=%s\n",encrypt);
		
		if(tp1)
		{
			tp=strchr(tp1,')');
			*tp='\0';
			tkip_aes=tp1;
			//cprintf("TKIP_AES=%s\n",tkip_aes);
		}

		if(ssid==NULL)
		{
			if(tp2)*tp2='C';
			return tp2;
		}

		char* encryption=NULL;
		char*  _tkpi_aes=NULL;
		int is_connected=0;
		
		if(linkinfo && (!strcasecmp(mac,linkinfo)))
			is_connected=1;
		
		if(!strcmp(encrypt,"mixed WPA/WPA2 PSK "))
			encryption="WPA/WPA2-PSK";
		else if (!strcmp(encrypt,"WPA2 PSK "))
			encryption="WPA2-PSK";
		else if (!strcmp(encrypt,"WPA PSK "))
			encryption="WPA-PSK";
		else if (!strncmp(encrypt,"WEP",3))
			encryption="WEP";
		else
			encryption="NONE";
	
		if(tkip_aes)
		{
			char* p=strstr(tkip_aes,"TKIP");
			char* q=strstr(tkip_aes,"CCMP");
			if(p&&q)
				_tkpi_aes="tkpi/aes";
			else if(p)
				_tkpi_aes="tkpi";
			else if(q)
				_tkpi_aes="aes";
		}

		APINFO* tmp=(APINFO*)safe_malloc(sizeof(APINFO));	
		memset(tmp,0,sizeof(APINFO));
		strcpy(tmp->ssid,ssid);
		strcpy(tmp->mac,mac);
		strcpy(tmp->channel,channel);
		strcpy(tmp->rssi,rssi);
		strcpy(tmp->encrypt,encryption);
		if(_tkpi_aes)strcpy(tmp->tkip_aes,_tkpi_aes);
		tmp->is_connected = is_connected;
		
		if(NULL==*ap_list)
			*ap_list=tmp;
		else
		{
			APINFO* last=*ap_list;
			while(NULL!=last->next)last=last->next;
			last->next=tmp;
		}
		
	}
	
	if(tp2)*tp2='C';
	return tp2;
}

void get_scaninfo(APINFO** apinfo)
{
	char *scan_argv[] = {"iwinfo","wlan0-1","scan",NULL};
	pid_t pid = 0;
	_eval(scan_argv, ">"TMP_WLSCAN_FILE, 0, &pid);

	char *link_argv[] = {"iw","dev","wlan0-1","link",NULL};
	_eval(link_argv, ">"TMP_WLLINK_FILE, 0, &pid);
	
	sleep(3);
    cprintf("********scan end********\n");
	char linkinfo[32];
	memset(linkinfo,0,32);
	get_linkinfo(linkinfo);		
	cprintf("linkinfo=%s\n",linkinfo);
	
	char* data = NULL,*new=NULL;
	int Max_retry_counter = 10;
  while(Max_retry_counter--){	
	data = file2str(TMP_WLSCAN_FILE);
	new = data;
	if(strlen(data)>0){
		cprintf("data=%s\n",data);
		break;
	}
	 else{
		cprintf("Invalid data\n");
		sleep(1);
	 }
	}

	while(NULL!=new)
	{
		new=parse_line(new,apinfo,linkinfo);
	}
	#if 0
	APINFO* apinfo1 = *apinfo; 
	while(apinfo1){
		cprintf("NEWApinfo:%s\n%s\n",apinfo1->ssid,apinfo1->mac);
		cprintf("%s\n%s\n",apinfo1->channel,apinfo1->rssi);
		cprintf("%s\n%s\n",apinfo1->encrypt,apinfo1->tkip_aes);
		cprintf("%d\n\n",apinfo1->is_connected);
		
		apinfo1=apinfo1->next;
	}
	#endif

	free(data);
    cprintf("********get_scaninfo end********\n");
}
#endif
