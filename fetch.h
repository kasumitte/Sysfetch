#ifndef FETCH_H
#define FETCH_H

#define MAX_LINES 13
#define MAXLEN 256
#define ANSI_RESET "\x1b[0m"
#define TEXT_COLOR "\x1b[1;94m"     
#define ERROR_COLOR "\x1b[0;31m"
#define WARNING_COLOR "\x1b[0;33m"
#include <stddef.h>


typedef struct SysInfo {
    unsigned long totalswap;      
    unsigned long totaldisk;      
    unsigned long totalram;       
    unsigned long occswap;        
    unsigned long occdisk;        
    unsigned long occram;         
    long uptime;                 
    char host_user[MAXLEN];       
    char packages[MAXLEN];        
    char kernel[MAXLEN];          
    char osname[MAXLEN];          
    char fstype[MAXLEN];
    char locale[MAXLEN];
    char shell[MAXLEN];           
    char term[MAXLEN];           
    char cpu[MAXLEN];            
    char gpu[MAXLEN];     
    char IP[MAXLEN];              
} SysInfo;

struct sysinfo;
struct utsname;
struct SysInfo;

void get_size(int *width, int *height);

void formatted_time(int time_sec, char *result, size_t result_size);

void formatted_output(double full_size, char *result, size_t result_size);

int proc_img(
        const char *filepath, 
        int term_width, int term_height, 
        char *fulldata, size_t bufsize,
        char lines[][MAXLEN], int lines_count, int start_text_y);

void define_system(char *osid, char *filepath); 

void trim_string(char *dst, const char *src, size_t max_len); 

void get_user_info(SysInfo *data);

void get_os_info(struct utsname *kernel, SysInfo *data, char *osid);

/* number of packages installed from specific distribution package manager */
void get_pack_info(SysInfo *info, char *osid);

void pack_count(const char *cmd, const char *manager, char *result, size_t result_size);

void get_env(SysInfo *data);

void get_hardware(SysInfo *data);

void get_IP(SysInfo *data);

int build_info(SysInfo *data, char lines[][MAXLEN]);

#endif
