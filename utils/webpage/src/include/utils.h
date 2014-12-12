/*************************************************************************
    > File Name: utils.h
    > Author: yuehb
    > Mail: yuehb@bellnett.com 
    > Created Time: Wed 09 Jul 2014 05:01:55 PM CST
 ************************************************************************/

#define DEBUG 1

#ifdef DEBUG
#define cprintf(fmt, args...) do { \
    FILE *fp = fopen("/dev/console", "w"); \
    if (fp) { \
		fprintf(fp, fmt, ## args); \
		fclose(fp); \
	} \
} while (0)
#else
#define cprintf(fmt, args...) 
#endif

/*
 *  Reads file and returns contents
 *  @param	fd	file descriptor
 *	@return	contents of file or NULL if an error occurred
 */
extern char * fd2str(int fd);

/*
 *  Reads file and returns contents
 *  @param	path	path to file
 *  @return	contents of file or NULL if an error occurred
 */
extern char * file2str(const char *path);

/* Simple version of _eval() (no timeout and wait for child termination) */
#define eval(cmd, args...) ({ \
	const char * const argv[] = { cmd, ## args, NULL }; \
	_eval((char * const *)argv, ">/dev/null", 0, NULL); \
})


#define WPA2_PSK	0x01
#define WPA_PSK	    0x02
#define TKIP	    0x01
#define AES  	    0x02
#define MAX_UPLOAD_KB 1024*1024 
#define CMD_BUF  512
#define MAX_OUTBUF_LEN  512
#define TMP_UCI_FILE "/usr/web/cgi-bin/tmp_uci"
#define TMP_WLSCAN_FILE "/usr/web/cgi-bin/tmp_wlscan"
#define TMP_WLLINK_FILE "/usr/web/cgi-bin/linkinfo"
#define TMP_WLDHCP_FILE "/usr/web/cgi-bin/dhcpinfo"



typedef struct dhcpinfo{
	char ip[16];
	char subnet[16];
	char gateway[16];
	char dns1[16];
	char dns2[16];
}DHCPINFO; 

typedef struct apinfo{
	char ssid[64];
	char mac[16];
	char channel[4];
	char rssi[12];
	char encrypt[16];
	char tkip_aes[16];
	char is_connected;
	struct apinfo* next;
}APINFO;

char* uci_get(char* name);
void uci_set(char* name,char* value);
void uci_saveset(char* name,char* value);
void uci_commit();
void * safe_malloc (size_t size);
int LineToStr (char *string, size_t max);
