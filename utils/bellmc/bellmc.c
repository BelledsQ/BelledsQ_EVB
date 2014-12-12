#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>

//#define PORTNUM 5000
//#define GROUPIP "224.0.1.1"
#define PORTNUM 11608
#define GROUPIP "224.0.0.88"

#define TMP_PATH "/tmp"
#define CMD_BUF 64



int DEBUG_LEVEL = 2;

#define LOG(format,args...) do { if(DEBUG_LEVEL>5) { fprintf(stderr,format, ## args); } }while(0) //DEBUG = 6
#define DEBUG(format,args...) do { if(DEBUG_LEVEL>4) { fprintf(stderr,format, ## args); } }while(0) //DEBUG = 5
#define INFO(format,args...) do { if(DEBUG_LEVEL>3) { fprintf(stderr,format, ## args); } }while(0) //DEBUG = 4
#define WARN(format,args...) do { if(DEBUG_LEVEL>2) { fprintf(stderr,format, ## args); } }while(0) //DEBUG = 3
#define ERROR(format,args...) do { if(DEBUG_LEVEL>1) { fprintf(stderr,format, ## args); } }while(0) //DEBUG = 2
#define FATAL(format,args...) do { if(DEBUG_LEVEL>0) { fprintf(stderr,format, ## args); } }while(0) //DEBUG = 1



/*obtain ip address, which dhcp from router,*/
int get_dhcp_ip(unsigned char* ipStr)
{
	FILE* fp;
	char *cmd;
	cmd=(char*)malloc(CMD_BUF*sizeof(char));
	
	
	sprintf(cmd, "ifconfig wlan0 >%s/wlan0",TMP_PATH);
	system(cmd);
	memset(cmd, 0, CMD_BUF);
	sprintf(cmd, "cat %s/wlan0.log |grep -o \'\\<[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}\\>\' >%s/ipmsg",TMP_PATH,TMP_PATH);
	
	system(cmd);
	memset(cmd, 0, CMD_BUF);
	
	sprintf(cmd, "cat %s/ipmsg.log  |head -1 >%s/ipaddr",TMP_PATH,TMP_PATH);
	system(cmd);
	memset(cmd, 0, CMD_BUF);
	
	sprintf(cmd, "%s/ipaddr", TMP_PATH );
	fp = fopen(cmd,"a+");
	if(!fp)
	{
		printf("open ipaddr error!\n");
		return -1;
	}
	
	fgets(ipStr,CMD_BUF,fp);
	
	DEBUG("value:%s\n",  ipStr);
	if(ipStr[strlen(ipStr)-1]=='\n')
	{
		ipStr[strlen(ipStr)-1]='\0';
	}

	fclose(fp);
	free(cmd);
	return 1;
	//ifconfig wlan0 >wlan0.log 获取部分ip信息
	//cat wlan0.log |grep -o '\<[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\>' >ipmsg.log //保存ip类似消息
	//cat ipmsg.log |head -1  //取出第一行，ip地址信息
}

/*uci options, get a uci param value from the uci file*/
void uci_get(char* name,char* value)
{
        FILE* fp=NULL;

        char cmd[CMD_BUF];
	
        sprintf(cmd,"uci get %s |cat > %s/ledfile",name,TMP_PATH);
        system(cmd);
		memset(cmd, 0, CMD_BUF);
		sprintf(cmd, "%s/ledfile", TMP_PATH);
		
        fp = fopen(cmd,"a+");

		memset(cmd, 0, CMD_BUF);
		sprintf(cmd, "rm %s/ledfile", TMP_PATH);

		system(cmd);

        if(fp == NULL) 
        {
                FATAL("\nerror on open ledfile!\n");
                exit(1);
        }
        fgets(value,CMD_BUF,fp);
		
		DEBUG("entering into uci_get: name:%s,  value:%s\n", name , value);
		if(value[strlen(value)-1]=='\n')
		{
			value[strlen(value)-1]='\0';
		}

		fclose(fp);
}


int main(int argc, char* argv[])
{
	int sock_id;
	struct sockaddr_in addr;
	char broad[CMD_BUF] = "init,init,init";
	char buf_sn[CMD_BUF] = "belled,M01234567890,192.168.3.99";
	char buf_ip[CMD_BUF] = "666";
	socklen_t len;
	int ret;
	/* open a socket. only udp support multicast */
	if ((sock_id = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		perror("socket error");
		exit(1);
	}
	/* build address */
	memset((void*)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(GROUPIP); /* multicast group ip */ 
	addr.sin_port = htons(PORTNUM);
	
	len = sizeof(addr);
	
	if(argc==1||argc>3)
	{
		printf("please input:\n./multi <sn> <ipaddr>\n");
		exit(0);
	}
	#if 0
	//memset(buf_sn, 0, sizeof(char*strlen(buf_sn));
	//memset(buf_sn, 0, sizeof(char*strlen(buf_sn)));
	uci_get("wireless.@wifi-iface[0].sn",buf_sn);
	
	get_dhcp_ip(buf_ip);
	printf("buf_sn = %s, buf_ip = %s\n", buf_sn,buf_ip);
	
	memset(broad, 0, CMD_BUF);
	sprintf(broad, "belled,%s,%s", argv[1],argv[2]);
	printf("broad = %s\n", broad);
	#endif
	
	memset(broad, 0, CMD_BUF);
	sprintf(broad, "belled,%s,%s", argv[1],argv[2]);
	printf("belled,sn:%s,ipaddr:%s\n", argv[1],argv[2]);
	while (1) 
	{
		/* it's very easy, just send the data to the address:port */
		ret = sendto(sock_id, broad, strlen(broad), 0,(struct sockaddr *)&addr, len);
		if (ret < 0) 
		{
			perror("sendto error");
			printf("error\n");
			exit(1);
		}
		
		sleep(1); /* wait 2 sec. */
	}
	
	close(sock_id);
	
	return 0;
}
