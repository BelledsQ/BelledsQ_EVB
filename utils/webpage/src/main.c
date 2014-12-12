/*************************************************************************
    > File Name: main.c
    > Author: yuehb
    > Mail: yuehb@bellnett.com 
    > Created Time: Wed 09 Jul 2014 05:00:05 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include "cgi.h"
#include "utils.h"

#define UPGRADE_FW  "/tmp/firmware.bin"
void main(int argc,char* argv[],char *env[])
{
	int i=0;
	char *ref=getenv("HTTP_REFERER");
	cprintf("\napp=%s\n",argv[0]);
	cprintf("HTTP_REFERER=%s\n",ref);

	if(!strcmp(argv[0],"/usr/web/cgi-bin/SysUpgrade"))
	{
		char cmd[32]="rm -rf ";
		size_t	cl,offset;
		char* qs=NULL,*ct=NULL;
		int i;
		char*  boundary;
		
		system(strcat(cmd,UPGRADE_FW));
		
		printf("Content-type: text/html\r\n\r\n") ;

		if (getenv("CONTENT_LENGTH") == NULL) {
		/* No content length?! */
			printf("Content-type: text/html\r\n\r\n") ;
			printf("No Content Length in HTTP Header from client");
			return;
		}
		cl=atoi(getenv("CONTENT_LENGTH"));
		/* protect ourselves from 16M file uploads */
		if (cl > MAX_UPLOAD_KB * 16 ) {
			printf("Attempted to send content larger than allowed limits.");
			/* But we need to finish reading the content */
			while (	fread( &i, sizeof(int), 1, stdin) == 1 );
			return;
		}
		if (!(qs=malloc(cl+1))) {
			printf("Unable to Allocate memory for POST content.");
			return;
		}

		/* set the buffer to null, so that a browser messing with less 
		 *data than content_length won't buffer underrun us */
		memset(qs, 0 ,cl+1);

		if( (!fread(qs,cl,1,stdin) && (cl > 0) && !feof(stdin))) {
			printf( "Unable to read from stdin", 0);
			printf ("Content Length is %d ", cl);
			free(qs);
			return;
		}

		i=strlen(getenv("CONTENT_TYPE"))+1;
		ct=malloc(i);
		if(ct){
			memcpy(ct, getenv("CONTENT_TYPE"), i);
		}else{
			free(qs);
			return;
		}
		cprintf("Content-type:%s\n",ct);
	
		i=(int) NULL;
		if(ct!= NULL){
			while (i < strlen(ct) && (strncmp("boundary=", &ct[i], 9) != 0)) { i++; }
		}
		if(i == strlen(ct)) {
			/* no boundary informaiton found */
			printf("No Mime Boundary Information Found");
			goto exit_SysUpgrade;
		}

		boundary=&ct[i+7];
		/* add two leading -- to the boundary */
		boundary[0]='-';
		boundary[1]='-';	
		//cprintf("boundary=%s\n",boundary);
		cprintf("Content-Length:%d\n",cl);
		
		offset=0;
		while (offset < cl) {
			/* first look for boundary */
			while ((offset < cl) && (memcmp(&qs[offset], boundary, strlen(boundary)))) {
				offset++;
			}
			/* if we got here and we ran off the end, its an error		*/
			if(offset >= cl) { 
				printf("Malformed MIME Encoding");
				goto exit_SysUpgrade;
			}
			/* if the two characters following the boundary are --,		*/ 
			/* then we are at the end, exit					*/
			if(memcmp(&qs[offset+strlen(boundary)], "--", 2) == 0) {
				offset+=2;
				printf("nidayefileToUpload <br></body></html>");
				break;
			}
			int line;
			/* find where the offset should be */
			line=LineToStr(&qs[offset], cl-offset);
			offset+=line;
			
			/* Now we're going to look for content-disposition		*/ 
			line=LineToStr(&qs[offset], cl-offset);
			if (strncasecmp(&qs[offset], "Content-Disposition", 19) != 0) {
			/* hmm... content disposition was not where we expected it */
				printf("Content-Disposition Missing");
				goto exit_SysUpgrade;
			}
			char* envname;
			/* Found it, so let's go find "name="				*/
			if (!(envname=strstr(&qs[offset], "name="))) {
				/* now name= is missing?!				*/
				printf("Content-Disposition missing name tag");
				goto exit_SysUpgrade;
			}else{
				envname+=6;
			}

			char* filename;
			/* is there a filename tag?						*/
			if ((filename=strstr(&qs[offset], "filename="))!= NULL) {
				filename+=10;
			} else {
				filename=NULL;
			}

			/* make envname and filename ASCIIZ				*/
			i=0;
			while ((envname[i] != '"') && (envname[i] != '\0')) { i++; }
			envname[i] = '\0';
			if (filename) {
				i=0;
				while ((filename[i] != '"') && (filename[i] != '\0')) { i++; }
				filename[i] = '\0';
			}
			
			offset+=line;
			/* Ok, by some miracle, we have the name; let's skip till we	*/
			/* come to a blank line						*/
			line=LineToStr(&qs[offset], cl-offset);
			while (strlen(&qs[offset]) > 1) {
				offset+=line;
				line=LineToStr(&qs[offset], cl-offset);
			}
			offset+=line;
			int datastart;
			datastart=offset;
			/* And we go back to looking for a boundary */
			while ((offset < cl) && (memcmp(&qs[offset], boundary, strlen(boundary)))) {
				offset++;
			}
			/* strip [cr] lf */
			if ((qs[offset-1] == '\n') && (qs[offset-2] == '\r')) {
				offset-=2; 
			}else {
				offset-=1;
			}
			qs[offset]=0;

			/* ok, at this point, we know where the name is, and we know    */
			/* where the content is... we have to do one of two things      */
			/* based on whether its a file or not				*/
			if (filename==NULL) { /* its not a file, so its easy		*/
				printf("filename NULL");
			}else {	/* handle the fileupload case		*/
				if (offset-datastart) {  /* only if they uploaded */
					int fd;
					fd=open(UPGRADE_FW,O_RDWR | O_CREAT | O_TRUNC,S_IRWXU | S_IRWXG | S_IRWXO);
					if (fd == -1) {
						printf("Unable to open temp file");
						goto exit_SysUpgrade;
					}
					write(fd, &qs[datastart], offset-datastart);
					close(fd);
					cprintf("Write Firmware.bin OK.\n");
					printf("nidaye");//OK
					goto exit_SysUpgrade;
				}else
					printf("offset-datastart=0");
			}
		}
exit_SysUpgrade:
		if(ct)free(ct);
		if(qs)free(qs);
		return;
	}

	cgi_init();
	cgi_process_form();

	if(!strcmp(argv[0],"/usr/web/cgi-bin/sysupdate"))	
	{
		cprintf("Now,Redirect!\n");
		char* data=cgi_param("data");
		if(data && !strcmp(data,"reboot"))
		{
			cprintf("data=%s\n",data);
			cprintf("Now Redirect to index\n");
			printf("Content-type: text/html\r\n\r\n") ;
			printf("Now Redirect to index\n");
			sleep(5);
		}else if(data && !strcmp(data,"yes"))
		{
			cprintf("yes do it !\n");
			//system("sysupgrade  /tmp/firmware.bin");
			eval("/sbin/sysupgrade", UPGRADE_FW);
		}
		goto exit;
	}


	char* ts=cgi_param("ts");
	char* data=cgi_param("data");
	//if(ts&&data)
	if(data)
	{
		//cprintf("ts=%s\n",ts);
		cprintf("data=%s\n",data);
		char bell_version[128];
		memset(bell_version,0,128);
		FILE *fp_bell=NULL;
		fp_bell=fopen("/rom/etc/bell_version","r");
		if(NULL==fp_bell)
		{
			//return-1;
			strcpy(bell_version,"v1.0.00_r00");
		}		
		else
		{
			fgets(bell_version,128,fp_bell);
			fclose(fp_bell);
		}
		

		if(!strcmp(data,"<getSysInfo><Version/></getSysInfo>"))
		{
			printf("Content-type: text/html\r\n\r\n") ;
			printf("<getSysInfo><Version fw2=\"%s\"></Version><Return status=\"true\" ></Return></getSysInfo>",bell_version);
		}
		else if(!strcmp(data,"cacel_upgrade"))
		{
			system("rm -f "UPGRADE_FW);
			printf("Content-type: text/html\r\n\r\n") ;
			printf("success!");
		}
		else if(!strcmp(data,"latest_version"))
		{
			//GET Latest FW VERSION
			FILE*  stream;
			char   buf[128];
			memset(buf,0,sizeof(buf));
			char   request[128];
			memset(request,0,sizeof(request));
			char* server=NULL;
			server = uci_get("system.remote_serv.addr");
			if(server){
				sprintf(request,"curl http://%s/q/api/get_version.php?sn=`fts getsn`\\&fun=1",server);
				free(server);
			}
			//stream = popen( "curl http://121.40.90.86:8000/q/api/get_version.php?sn=11111111111111111\\&fun=1", "r" );

			stream = popen( request, "r" );
			fread(buf, sizeof(char), sizeof(buf), stream); 
			pclose(stream);
			if(strlen(buf)>0)cprintf("buf:%s\n",buf);
			else strcpy(buf,bell_version);
			printf("Content-type: text/html\r\n\r\n") ;
			printf("<getSysInfo><Version fw2=\"%s\"></Version><Return status=\"true\" ></Return></getSysInfo>",buf);
			//system("curl http://121.40.90.86:8000/q/api/get_version.php?sn=11111111111111111\\&fun=2  >/tmp/verinfo");
		}
		else if(!strcmp(data,"autoupgradeFW"))
		{
			//download FW
			FILE*  stream;
			char   buf[128];
			memset(buf,0,128);
			char   request[128];
			memset(request,0,sizeof(request));
			char* server=NULL;
			server = uci_get("system.remote_serv.addr");
			if(server){
				sprintf(request,"curl http://%s/q/api/get_version.php?sn=`fts getsn`\\&fun=2",server);
				free(server);
			}
			//stream = popen( "curl http://121.40.90.86:8000/q/api/get_version.php?sn=11111111111111111\\&fun=2", "r" );
			stream = popen( request, "r" );

			fread(buf, sizeof(char), 256, stream); 
			pclose(stream);
			system("rm -f"UPGRADE_FW);

			char* seq =NULL;
			seq = strchr(buf,',');
			if(seq){
				*seq = '\0';seq++;
				cprintf("url:%s\n",buf);
				cprintf("checksum:%s\n",seq);
				char cmd[128];memset(cmd,0,128);
				sprintf(cmd,"wget %s -O /tmp/firmware.bin",buf);
				cprintf("cmd:%s\n",cmd);
				system(cmd);

				char checksum[64];
				memset(checksum,0,64);
				stream = popen( "md5sum /tmp/firmware.bin", "r" );
				fread(checksum, sizeof(char), 64, stream); 
				pclose(stream);
				
				cprintf("new checksum:%s\n",checksum);
				printf("Content-type: text/html\r\n\r\n") ;
				if(strncmp(checksum,seq,strlen(seq))==0)
					printf("success!");
				else
					printf("fail");
					
			}
			else
			{
				printf("Content-type: text/html\r\n\r\n") ;
				printf("fail");
			}
		}
		else if(!strcmp(data,"<getSysInfo><airplay/></getSysInfo>")){
			char* airplay = uci_get("airplay.unix");
			printf("Content-type: text/html\r\n\r\n") ;
			printf("<getSysInfo><airplay name=\"%s\"></airplay><Return status=\"true\" ></Return></getSysInfo>",airplay);
			if(airplay)free(airplay);
		}
		else if(!strcmp(data,"<getSysInfo><SSID/></getSysInfo>")){
			char* ssid = uci_get("wireless.@wifi-iface[0].ssid");
			char* encrypt = uci_get("wireless.@wifi-iface[0].encryption");
			char* password = uci_get("wireless.@wifi-iface[0].key");
			char* new_encrypt=NULL;
			cprintf("ssid=%s,encrypt=%s,password=%s\n",ssid,encrypt,password);
			if(!strcmp(encrypt,"none"))new_encrypt="NONE";
			else if(!strcmp(encrypt,"psk"))new_encrypt="WPA";
			else if(!strcmp(encrypt,"psk2"))new_encrypt="WPA2";
			else if(!strcmp(encrypt,"psk-mixed"))new_encrypt="WPA/WPA2";
			printf("Content-type: text/html\r\n\r\n") ;
			printf("<getSysInfo><SSID name=\"%s\" encrypt=\"%s\" channel=\"\" password=\"%s\" encrypt_len=\"\" format=\"\" mac=\"845dd7a00504\"></SSID><Return status=\"true\" ></Return></getSysInfo>",ssid,new_encrypt,password);
			if(ssid)free(ssid);
			if(encrypt)free(encrypt);
			if(password)free(password);
			//printf("<getSysInfo><SSID name=\"airmusic_A00504\" encrypt=\"NONE\" channel=\"\" password=\"\" encrypt_len=\"\" format=\"\" mac=\"845dd7a00504\"></SSID><Return status=\"true\" ></Return></getSysInfo>");
		}
		else if(!strcmp(data,"<getSysInfo><WorkMode/><Client/></getSysInfo>")){
			printf("Content-type: text/html\r\n\r\n") ;
			printf("<getSysInfo><WorkMode value=\"1\"></WorkMode><Client enable=\"ON\"></Client><Return status=\"true\" ></Return></getSysInfo>");
		}
		else if(!strcmp(data,"<getSysInfo><APList/><RemoteAP/></getSysInfo>")){
			char res[4096];
			int len =0;
				
			len += snprintf(res+len,sizeof(res),"<getSysInfo><APList>");
			APINFO* gg =NULL,*con=NULL,*begin=NULL; 
			//get_wlscaninfo(&gg);
			get_scaninfo(&gg);
			begin=gg;
			while(gg)
			{
				if(gg->is_connected)
					con=gg;
						
				len += snprintf(res+len,sizeof(res),"<AP name=\"%s\" mac=\"%s\" channel=\"%s\" rssi=\"%s\" encrypt=\"%s\" tkip_aes=\"%s\" ></AP>",gg->ssid,gg->mac,gg->channel,gg->rssi,gg->encrypt,gg->tkip_aes);
				 gg=gg->next;
			}

			char* encrypt = uci_get("wireless.@wifi-iface[1].encryption");
			char*passwd=NULL;
			if(!strncasecmp(encrypt,"WEP",3))
				passwd = uci_get("wireless.@wifi-iface[1].key1");
			else
				passwd = uci_get("wireless.@wifi-iface[1].key");
			if(encrypt)cprintf("encrypt:%s\n",encrypt);
			if(passwd)cprintf("passwd:%s\n",passwd);
			if(con){
				DHCPINFO tmp;
				get_wl_dhcpinfo(&tmp);
				//cprintf("DHCPINFO:\n%s\n%s\n%s\n%s\n%s\n",tmp.ip,tmp.subnet,tmp.gateway,tmp.dns1,tmp.dns2);
				len += snprintf(res+len,sizeof(res),"</APList><RemoteAP name=\"%s\" encrypt=\"%s\" channel=\"%s\" password=\"%s\" status=\"0\" ip=\"%s\" mask=\"%s\" gateway=\"%s\" dns1=\"%s\" dns2=\"%s\"></RemoteAP><Return status=\"true\" ></Return></getSysInfo>",con->ssid,con->encrypt,con->channel,passwd,tmp.ip,tmp.subnet,tmp.gateway,tmp.dns1,tmp.dns2);
			}else{
				char* ssid = uci_get("wireless.@wifi-iface[1].ssid");
				len += snprintf(res+len,sizeof(res),"</APList><RemoteAP name=\"%s\" encrypt=\"%s\" channel=\"\" password=\"%s\" status=\"1\" ip=\"\" mask=\"\" gateway=\"\" dns1=\"\" dns2=\"\"></RemoteAP><Return status=\"true\" ></Return></getSysInfo>",ssid,encrypt,passwd);
				if(ssid)free(ssid);
			}

			if(encrypt)free(encrypt);
			if(passwd)free(passwd);
			
			while(begin){
				gg=begin;
				free(gg);
				begin=begin->next;
			}

			cprintf("msg=%s\n",res);
			printf("Content-type: text/html\r\n\r\n") ;
			printf("%s",res);
			/*
			if(is_AP_connected==49)
			printf("<getSysInfo><APList><AP name=\"Avalaa\" mac=\"0025129C7152\" channel=\"1\" rssi=\"-54\" encrypt=\"WPA2-PSK\" tkip_aes=\"tkip/aes\" ></AP><AP name=\"bellnet-zhands\" mac=\"001111223311\" channel=\"11\" rssi=\"-66\" encrypt=\"NONE\" tkip_aes=\"\" ></AP><AP name=\"bellnet-a00a5b\" mac=\"845DD7A00A5B\" channel=\"11\" rssi=\"-62\" encrypt=\"NONE\" tkip_aes=\"\" ></AP><AP name=\"NETGEAR\" mac=\"28107BEEBD9D\" channel=\"11\" rssi=\"-52\" encrypt=\"WPA/WPA2-PSK\" tkip_aes=\"tkip/aes\" ></AP><AP name=\"OpenWrt_LED\" mac=\"001122334455\" channel=\"11\" rssi=\"-58\" encrypt=\"NONE\" tkip_aes=\"\" ></AP><AP name=\"bellnet-a00a5f\" mac=\"845DD7A00A5F\" channel=\"11\" rssi=\"-67\" encrypt=\"NONE\" tkip_aes=\"\" ></AP></APList><RemoteAP name=\"NETGEAR\" encrypt=\"WPA/WPA2\" channel=\"11\" password=\"bellmusic\" status=\"0\" ip=\"192.168.3.84\" mask=\"255.255.255.0\" gateway=\"192.168.3.254\" dns1=\"221.228.255.1\" dns2=\"208.67.222.222\"></RemoteAP><Return status=\"true\" ></Return></getSysInfo>");
			else{
			printf("<getSysInfo><APList><AP name=\"TEST\" mac=\"0025129C7151\" channel=\"1\" rssi=\"-54\" encrypt=\"WEP\" tkip_aes=\"\" ></AP><AP name=\"Avalaa\" mac=\"0025129C7152\" channel=\"1\" rssi=\"-54\" encrypt=\"WEP\" tkip_aes=\"\" ></AP><AP name=\"bellnet-zhands\" mac=\"001111223311\" channel=\"11\" rssi=\"-66\" encrypt=\"NONE\" tkip_aes=\"\" ></AP><AP name=\"bellnet-a00a5b\" mac=\"845DD7A00A5B\" channel=\"11\" rssi=\"-62\" encrypt=\"NONE\" tkip_aes=\"\" ></AP><AP name=\"NETGEAR\" mac=\"28107BEEBD9D\" channel=\"11\" rssi=\"-52\" encrypt=\"WPA/WPA2-PSK\" tkip_aes=\"tkip/aes\" ></AP><AP name=\"OpenWrt_LED\" mac=\"001122334455\" channel=\"11\" rssi=\"-58\" encrypt=\"NONE\" tkip_aes=\"\" ></AP><AP name=\"bellnet-a00a5f\" mac=\"845DD7A00A5F\" channel=\"11\" rssi=\"-67\" encrypt=\"NONE\" tkip_aes=\"\" ></AP></APList><RemoteAP name=\"Avalaa\" encrypt=\"WPA2\" channel=\"\" password=\"345671234\" status=\"1\" ip=\"\" mask=\"\" gateway=\"\" dns1=\"\" dns2=\"\"></RemoteAP><Return status=\"true\" ></Return></getSysInfo>");
			}*/
		}

		//POST
		//else if(!strcmp(data,"<setSysInfo><airplay name=\"22\"/></setSysInfo>")){
		else if(strstr(data,"<setSysInfo><airplay")){
			char* name=NULL,*end;
			if(name=strchr(data,'\"')){
			 	name++;
				if(end=strchr(name,'\"'))*end='\0';
				{
					uci_saveset("airplay.unix",name);
					eval("/usr/web/cgi-bin/airplay");
				}
			}
			printf("Content-type: text/html\r\n\r\n") ;
			//printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"true\" delay=\"30\"></Return></setSysInfo>");
			//printf("\r\n\r\nContent-type:text/html\r\n\r\n");
			printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"true\" ></Return></setSysInfo>");
			//printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"false\" >wpa encrypt set error!</Return></setSysInfo>");
		}
		//else if(!strcmp(data,"<setSysInfo><SSID name=\"airmusic_A00504\" encrypt=\"NONE\" channel=\"\" password=\"\" encrypt_len=\"\" format=\"\" mac=\"845dd7a00504\" tkip_aes=\"aes\"/></setSysInfo>")){
		else if(strstr(data,"<setSysInfo><SSID")){
			char*name=NULL,*encrypt=NULL,*password=NULL,*end=NULL;
			if(name=strstr(data,"name=")){
				name=name+strlen("name=")+1;
				end=strchr(name,'\"');*end='\0';
				encrypt=end+1;
				if(encrypt=strstr(encrypt,"encrypt=")){
					encrypt=encrypt+strlen("encrypt=")+1;
					end=strchr(encrypt,'\"');*end='\0';
					password=end+1;
					if(password=strstr(password,"password=")){
						password=password+strlen("password=")+1;
						end=strchr(password,'\"');*end='\0';
						cprintf("name=%s,encrypt=%s,password=%s\n",name,encrypt,password);
						uci_saveset("wireless.@wifi-iface[0].ssid",name);
						if(!strcmp(encrypt,"NONE")){
							uci_saveset("wireless.@wifi-iface[0].encryption","none");
							uci_saveset("wireless.@wifi-iface[0].key","");
						}else if(!strcmp(encrypt,"WPA")){
							uci_saveset("wireless.@wifi-iface[0].encryption","psk");
							uci_saveset("wireless.@wifi-iface[0].key",password);
						}else if(!strcmp(encrypt,"WPA2")){
							uci_saveset("wireless.@wifi-iface[0].encryption","psk2");
							uci_saveset("wireless.@wifi-iface[0].key",password);
						}else{
							uci_saveset("wireless.@wifi-iface[0].encryption","psk-mixed");
							uci_saveset("wireless.@wifi-iface[0].key",password);
						}
					}
				}
			}
			printf("Content-type: text/html\r\n\r\n") ;
			printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"true\" delay=\"20\"></Return></setSysInfo>");
			printf("\r\n\r\nContent-type:text/html\r\n\r\n");
			printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"true\" ></Return></setSysInfo>");
			//printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"false\" >wpa encrypt set error!</Return></setSysInfo>");
			//system("PATH=/bin:/sbin:/usr/bin:/usr/sbin /etc/init.d/network restart");
			eval("/etc/init.d/network","restart");
		}
		else if(!strcmp(data,"<setSysInfo><Client enable=\"OFF\"/></setSysInfo>")){
			printf("Content-type: text/html\r\n\r\n") ;
			printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"true\" ></Return></setSysInfo>");
			//printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"false\" >wpa encrypt set error!</Return></setSysInfo>");
			eval("ifconfig","wlan0-1","down");
		}
		else if(!strcmp(data,"<setSysInfo><Client enable=\"ON\"/></setSysInfo>")){
			eval("ifconfig","wlan0-1","up");
			sleep(5);
			printf("Content-type: text/html\r\n\r\n") ;
			printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"true\" ></Return></setSysInfo>");
			//printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"false\" >wpa encrypt set error!</Return></setSysInfo>");
		}
		//else if(!strcmp(data,"<setSysInfo><JoinWireless><AP name=\"NETGEAR\" mac=\"28107BEEBD9D\" channel=\"11\" rssi=\"-52\" encrypt=\"WPA/WPA2-PSK\" tkip_aes=\"tkip/aes\" password=\"bellmusic\"/></JoinWireless></setSysInfo>")){
		else if(strstr(data,"<setSysInfo><JoinWireless")){
			char *ssid=NULL,*encrypt=NULL,*password=NULL,*channel=NULL,*tp=NULL;
			if(ssid = strstr(data,"AP name=\""))
			{
				ssid+=strlen("AP name=\"");
				tp=strchr(ssid,'\"');
				*tp ='\0';tp++;
			}
			if(channel = strstr(tp,"channel=\""))
			{
				channel +=strlen("channel=\"");
				tp=strchr(channel,'\"');
				*tp ='\0';tp++;
			}
			if(encrypt = strstr(tp,"encrypt=\""))
			{
				encrypt += strlen("encrypt=\"");
				tp=strchr(encrypt,'\"');
				*tp ='\0';tp++;
			}
			if(password = strstr(tp,"password=\""))
			{
				password +=strlen("password=\"");
				tp=strchr(password,'\"');
				*tp ='\0';tp++;
			}

			if(strlen(ssid) > 0){cprintf("SSID:%s\n",ssid);uci_saveset("wireless.@wifi-iface[1].ssid",ssid);}
			if(strlen(encrypt) > 0){
				cprintf("ENC:%s\n",encrypt);
				char* encryption=NULL;

				if(!strcmp(encrypt,"WPA/WPA2-PSK"))
					encryption="psk-mixed";
				else if (!strcmp(encrypt,"WPA2-PSK"))
					encryption="psk2-psk";
				else if (!strcmp(encrypt,"WPA-PSK"))
					encryption="psk-psk";
				else if (!strncmp(encrypt,"WEP",3))
					encryption="wep-open";
				else
					encryption="none";

				uci_saveset("wireless.@wifi-iface[1].encryption",encryption);
			}
		
			if(password&&strlen(password) > 0)
			{
				cprintf( "PASSWD:%s\n",password);
				if(!strncmp(encrypt,"WEP",3))
				{
					uci_saveset("wireless.@wifi-iface[1].key","1");
					uci_saveset("wireless.@wifi-iface[1].key1",password);
				}
				else{
					uci_saveset("wireless.@wifi-iface[1].key1","");
					uci_saveset("wireless.@wifi-iface[1].key",password);
				}
			}

			if(strlen(channel) > 0){cprintf("Channel:%s\n",channel);uci_saveset("wireless.radio0.channel",channel);}
			
			eval("/etc/init.d/network","restart");

			printf("Content-type: text/html\r\n\r\n") ;
			printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"true\" delay=\"20\"></Return></setSysInfo>");
			printf("\r\n\r\nContent-type:text/html\r\n\r\n");
			printf("<?xml version=\"1.0\" encoding=\"utf-8\"?><setSysInfo><Return status=\"true\" ></Return></setSysInfo>");
		}
		else if(!strcmp(data,"<getSysInfo><RemoteAP/></getSysInfo>")){
			//Seems no need to do 
			printf("Content-type: text/html\r\n\r\n") ;
			printf("<getSysInfo><RemoteAP name=\"NETGEAR\" encrypt=\"WPA/WPA2\" channel=\"11\" password=\"bellmusic\" status=\"0\" ip=\"192.168.3.84\" mask=\"255.255.255.0\" gateway=\"192.168.3.254\" dns1=\"\" dns2=\"\"></RemoteAP><Return status=\"true\" ></Return></getSysInfo>");
		}
	}

	
exit:
	cprintf("\n");
	cgi_end();
}
