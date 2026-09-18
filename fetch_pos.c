#include "fetch.h"
#include <stddef.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <pwd.h>

typedef struct SysInfo SysInfo;

#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include "stb_image.h"


void get_size(int *width, int *height) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        perror(
            WARNING_COLOR"WARNING: Error determining terminal size. Using default values"ANSI_RESET
        );
        /* default size */
        *width = 55;
        *height = 30;
        return;
    }

    *width = ws.ws_col;
    *height = ws.ws_row;
}


int proc_img(const char *filepath, 
            int term_width, int term_height, 
            char *fulldata, size_t bufsize, 
            char lines[][MAXLEN], int line_count, int text_start_y) {
    
    const char *syms = " #+@/`os";
    int curs = 0;

    int img_width, img_height, n, des_channels = 4;
    unsigned char *img_data = stbi_load(filepath, &img_width, &img_height, &n, des_channels);

    if (img_data == NULL) {
        return 0;
    }

    int max_x = term_width / 2;
    int max_y = (term_height * 85) / 100;

    for (int y = 0; y < term_height; y++) {
        for (int x = 0; x < term_width; x++) {
            if ((x < max_x) && (y < max_y)) {
                int img_x = (x * img_width) / max_x;
                int img_y = (y * img_height) / max_y;
                
                int offset = (img_y * img_width + img_x) * des_channels;

                unsigned char r = img_data[offset];
                unsigned char g = img_data[offset + 1];
                unsigned char b = img_data[offset + 2];
                unsigned char a = img_data[offset + 3];

                if (a < 25) {
                    fulldata[curs++] = ' ';
                    continue;
                }

                int gray = (int) (0.2126f * r + 0.7152f * g + 0.0722f * b);
                int idx = (gray * (strlen(syms) - 1) / 255);
                
                /* apply ANSI escape codes to make symbols colorful */
                int bytes_written = snprintf(
                    fulldata + curs, bufsize - curs, "\x1b[38;2;%d;%d;%dm%c", r, g, b, syms[idx]
                );
                curs += bytes_written;
            }
            else {
                /* fill the (x >= max_x) part with data */
                int i = y - text_start_y;

                if (i < line_count && i >= 0) {
                    int bytes = snprintf(fulldata + curs, bufsize - curs, "%s", lines[i]);
                    curs += bytes;
                    break;
                } else {
                    fulldata[curs++] = ' ';
                }
            }
        }
        strcpy(fulldata + curs, ANSI_RESET);
        curs += strlen(ANSI_RESET);

        fulldata[curs++] = '\n';
    }

    fulldata[curs] = '\0';

    stbi_image_free(img_data);
    return 1;
}


int build_info(SysInfo *data, char lines[][MAXLEN]) {
    int lines_count = 13;
    char time[MAXLEN];
    char occram[MAXLEN];
    char totalram[MAXLEN];
    char occswap[MAXLEN];
    char totalswap[MAXLEN];
    char occdisk[MAXLEN];
    char totaldisk[MAXLEN];
    
    formatted_time(data->uptime, time, sizeof(time));
    formatted_output(data->occram, occram, sizeof(occram));
    formatted_output(data->totalram, totalram, sizeof(totalram));
    formatted_output(data->occswap, occswap, sizeof(occswap));
    formatted_output(data->totalswap, totalswap, sizeof(totalswap));
    formatted_output(data->occdisk, occdisk, sizeof(occdisk));
    formatted_output(data->totaldisk, totaldisk, sizeof(totaldisk));

    snprintf(lines[0], MAXLEN, TEXT_COLOR"%s"ANSI_RESET, data->host_user);     
    snprintf(lines[1], MAXLEN, TEXT_COLOR"OS"ANSI_RESET": %s", data->osname);
    snprintf(lines[2], MAXLEN, TEXT_COLOR"Kernel"ANSI_RESET": %s", data->kernel);
    snprintf(lines[3], MAXLEN, TEXT_COLOR"Uptime"ANSI_RESET": %s", time);
    snprintf(lines[4], MAXLEN, TEXT_COLOR"Packages"ANSI_RESET": %s", data->packages);
    snprintf(lines[5], MAXLEN, TEXT_COLOR"Shell"ANSI_RESET": %s", data->shell);
    snprintf(lines[6], MAXLEN, TEXT_COLOR"Terminal"ANSI_RESET": %s", data->term);
    snprintf(lines[7], MAXLEN, TEXT_COLOR"CPU"ANSI_RESET": %s", data->cpu);
    // snprintf(lines[8], MAXLEN, TEXT_COLOR"GPU"ANSI_RESET": %s", data->gpu);
    snprintf(lines[8], MAXLEN, TEXT_COLOR"Memory"ANSI_RESET": %s / %s", occram, totalram);
    snprintf(lines[9], MAXLEN, TEXT_COLOR"Swap"ANSI_RESET": %s / %s", occswap, totalswap);
    snprintf(
            lines[10], MAXLEN, TEXT_COLOR"Disk (/)"ANSI_RESET": %s / %s - %s", occdisk, totaldisk, data->fstype
    );
    snprintf(lines[11], MAXLEN, TEXT_COLOR"Local IP"ANSI_RESET": %s", data->IP);
    snprintf(lines[12], MAXLEN, TEXT_COLOR"Locale"ANSI_RESET": %s", data->locale);

    return lines_count;
}


void formatted_output(double full_size, char *result, size_t result_size) {
    double kb = 1024.0;
    double mb = 1024.0 * 1024.0;
    double gb = 1024.0 * 1024.0 * 1024.0;

    if (full_size >= gb)        snprintf(result, result_size, "%.2f GB", full_size / gb);
    else if (full_size >= mb)   snprintf(result, result_size, "%.2f MB", full_size / mb);
    else if (full_size >= kb)   snprintf(result, result_size, "%.2f KB", full_size / kb);
    else                        snprintf(result, result_size, "%.2f Bytes", full_size);
}


void formatted_time(int time_sec, char *result, size_t result_size) {
    int hours = time_sec / 3600;
    int minutes = (time_sec % 3600) / 60;
    int seconds = time_sec % 60;

    snprintf(result, result_size, "%.2dh: %.2dm: %.2ds", hours, minutes, seconds);
}


void define_system(char *osid, char *filepath) { 
    if (strcmp(osid, "arch") == 0)            snprintf(filepath, MAXLEN, "assets/arch_logo.png");
    else if (strcmp(osid, "debian") == 0)     snprintf(filepath, MAXLEN, "assets/debian_logo.png");
    else if (strcmp(osid, "fedora") == 0)     snprintf(filepath, MAXLEN, "assets/fedora_logo.png");
    else if (strcmp(osid, "ubuntu") == 0)     snprintf(filepath, MAXLEN, "assets/ubuntu_logo.png");
    else if (strcmp(osid, "gentoo") == 0)     snprintf(filepath, MAXLEN, "assets/gentoo_logo.png");
    else if (strcmp(osid, "linuxmint") == 0)  snprintf(filepath, MAXLEN, "assets/mint_logo.png");
    else if (
    (strcmp(osid, "freebsd") == 0) || 
    (strcmp(osid, "openbsd")) == 0)           snprintf(filepath, MAXLEN, "assets/openbsd_logo.png");
    
    else if (strcmp(osid, "manjaro") == 0)    snprintf(filepath, MAXLEN, "assets/manjaro_logo.png");
    else if (strcmp(osid, "centos") == 0)     snprintf(filepath, MAXLEN, "assets/centos_logo.png");
    else if (strcmp(osid, "nixos") == 0)      snprintf(filepath, MAXLEN, "assets/nixos_logo.png");
    else if (strcmp(osid, "unknown") == 0)    snprintf(filepath, 1, "");
}


void trim_string(char *dst, const char *src, size_t max_len) {
    while (*src && isspace((unsigned char)*src)) {
        src++;
    }
    snprintf(dst, max_len, "%s", src);

    size_t len = strlen(dst);
    while (len > 0 && isspace((unsigned char)dst[len - 1])) {
        dst[--len] = '\0';
    }
}


void pack_count(const char *cmd, const char *manager, char *result, size_t result_size) {
    char pack[MAXLEN];
    FILE *fp = popen(cmd, "r");
    
    if (!fp) {
        snprintf(result, result_size, "0 (unknown)");
        return;
    }
    while (fgets(pack, MAXLEN, fp) != NULL) {
        pack[strcspn(pack, "\r\n")] = '\0';
        snprintf(result, result_size, "%s (%s)", pack, manager);
    }

    if (pclose(fp) == -1) return;
}


void get_pack_info(SysInfo *data, char *osid) {
    int status;
    char result[MAXLEN];

    if (strcmp(osid, "arch") == 0) {
        pack_count("pacman -Qq | wc -l", "pacman", result, MAXLEN);
        strcpy(data->packages, result);
    }

    else if ((strcmp(osid, "debian") == 0) || 
            (strcmp(osid, "ubuntu") == 0) || 
            (strcmp(osid, "linuxmint") == 0)) {
        
        pack_count("dpkg-query -f '${binary:Package}\n' -W | wc -l", "pkg or dpkg", result, MAXLEN);
        strcpy(data->packages, result);
    }

    else if ((strcmp(osid, "fedora") == 0) || (strcmp(osid, "centos") == 0)) {
        pack_count("rpm -qa | wc -l", "rpm", result, MAXLEN);
        strcpy(data->packages, result);
    }

    else if (strcmp(osid, "gentoo") == 0) {
        pack_count("qlist -I | wc -l", "portage", result, MAXLEN);
        strcpy(data->packages, result);
    }

    else if ((strcmp(osid, "freebsd") == 0) || (strcmp(osid, "openbsd") == 0)) {
        pack_count("pkg info | wc -l", "pkg", result, MAXLEN);
        strcpy(data->packages, result);
    }

    else if (strcmp(osid, "manjaro") == 0) {
        pack_count("pamac list -- installed | wc -l", "pamac", result, MAXLEN);
        strcpy(data->packages, result);
    }

    else if (strcmp(osid, "nixos") == 0) {
        pack_count("nix-env -q --profile /run/current-system/sw | wc -l", "nixos", result, MAXLEN);
        strcpy(data->packages, result);
    } 
}


void get_IP(SysInfo *data) {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[MAXLEN];

    if (getifaddrs(&ifaddr) == -1) {
        snprintf(data->IP, MAXLEN, "unknown");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)  continue;
        
        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, 
                           (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), 
                            host, MAXLEN, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                snprintf(data->IP, MAXLEN, "unknown");
                continue;
            }
            snprintf(data->IP, MAXLEN, "%s", host);   
        }
    }
    freeifaddrs(ifaddr);
}


void get_hardware(SysInfo *data) {
    FILE *fd = fopen("/proc/cpuinfo", "r");

    if (fd) {
        char temp[MAXLEN];

        while (fgets(temp, sizeof(temp), fd) != NULL) {
            if (strncmp(temp, "model name", 10) == 0) {
                char *colon = strchr(temp, ':');
                
                char *val = colon + 1;
                while (*val == ' ' || *val == '\t') {
                    val++;
                }

                val[strcspn(val, "\r\n")] = '\0';
                
                strncpy(data->cpu, val, MAXLEN - 1);
                data->cpu[MAXLEN - 1] = '\0';
                break;
            }
        }
        fclose(fd);
    }
    else  snprintf(data->cpu, MAXLEN, "unknown");
    
    /* TODO: implement gpu parsing later */ 
}


void get_env(SysInfo *data) {
    char *shell; 
    char *locale;

    shell = getenv("SHELL");
    if (shell == NULL) {
        struct passwd *pw = getpwuid(getuid());
        shell = pw ? pw->pw_shell : "unknown";
    } 
    snprintf(data->shell, MAXLEN, "%s", shell);

    char current[MAXLEN] = "/proc/self/status";
    char name[MAXLEN] = "";
    char ppid[MAXLEN] = "";

    while (1) {
        
        FILE *fd = fopen(current, "r");
        if (!fd) {
            break;
        }
            
        char temp[MAXLEN];
        name[0] = '\0';
        ppid[0] = '\0';

        while (fgets(temp, MAXLEN, fd) != NULL) {
            if (strncmp(temp, "Name:", 5) == 0) {
                trim_string(name, temp + 5, sizeof(name));
            }
            else if (strncmp(temp, "PPid:", 5) == 0) {
                trim_string(ppid, temp + 5, sizeof(ppid));
            }
        }
        fclose(fd);
        
        if (name[0] == '\0' || ppid[0] == '\0' || strcmp(ppid, "0") == 0)   break;

        if (strcmp(name, "bash") != 0 && strcmp(name, "zsh") != 0 &&
            strcmp(name, "tmux") != 0 && strcmp(name, "sysfetch") != 0)   break;
        else {
            snprintf(current, MAXLEN, "/proc/%s/status", ppid);
            continue;
        }
    } 

    if (name[0] != '\0')  snprintf(data->term, MAXLEN, "%s", name);
    else    snprintf(data->term, MAXLEN, "%s", "unknown");
   
    snprintf(data->term, MAXLEN, "%s", name);

    locale = getenv("LANG");
    if (locale == NULL) {
        snprintf(data->locale, MAXLEN, "unknown");
    } else {
        snprintf(data->locale, MAXLEN, "%s", locale);
    }
}


void get_user_info(SysInfo *data) {
    char hostname[MAXLEN];
    gethostname(hostname, MAXLEN);
    
    struct passwd *pw = getpwuid(getuid());
    char *usrname = pw ? pw->pw_name : "unknown";
    
    snprintf(data->host_user, MAXLEN, "%s@%s", usrname, hostname);
}


void get_os_info(struct utsname *kernel, SysInfo *data, char *osid) {
    /* linux kernel info */
    if (uname(kernel) == -1) {
        strncpy(kernel->sysname, "unknown", sizeof(kernel->sysname) - 1);
        kernel->sysname[sizeof(kernel->sysname) - 1] = '\0';

        strncpy(kernel->release, "unknown", sizeof(kernel->release) - 1);
        kernel->release[sizeof(kernel->release) - 1] = '\0';
    }

    snprintf(data->kernel, MAXLEN, "%s %s", kernel->sysname, kernel->release);

    /* parsing the name of distribution 
    ("PRETTY_NAME" for pretty output and "ID" for defining the logo from assets) */
    FILE *fd = fopen("/etc/os-release", "r");
    char result[MAXLEN];

    if (fd) {
        char line[MAXLEN];
        char id_line[MAXLEN];

        while (fgets(line, sizeof(line), fd)) {
            if (sscanf(line, "PRETTY_NAME=\"%255[^\"]\"", result) == 1) {
                strncpy(data->osname, result, sizeof(data->osname));
                break;
            } 
            else if (sscanf(line, "PRETTY_NAME=%255[^\n]", result) == 1) {
                strncpy(data->osname, result, sizeof(data->osname));
                break;
            }
        }
        /* rewind to the top of the file before second loop */
        if (fseek(fd, 0, SEEK_SET) == -1)   return;

        while (fgets(id_line, sizeof(id_line), fd)) {
            if (strncmp(id_line, "ID=", 3) == 0) {
                char *start = id_line + 3;
                start[strcspn(start, "\r\n")] = '\0';

                if (start[0] == '"') {
                    start++;
                    size_t len = strlen(start);
                    
                    if (len > 0 && start[len - 1] == '"') {
                        start[len - 1] = '\0';
                    }
                }

                strncpy(osid, start, MAXLEN - 1);
                osid[MAXLEN - 1] = '\0';
                break;
            }
        }
        fclose(fd);
    } 
    else {
        /* if 'fopen' failed */
        struct utsname buff;
        if (uname(&buff) == 0) {
            for (int i = 0; buff.sysname[i]; i++) {
                buff.sysname[i] = tolower((unsigned char)buff.sysname[i]);
            }

            strncpy(osid, buff.sysname, sizeof(osid) - 1);
            osid[sizeof(osid) - 1] = '\0';
        }
        else {
            /* if nothing worked system's name will be 'unknown' */
            snprintf(osid, MAXLEN, "unknown");
            strncpy(data->osname, "unknown", sizeof(data->osname));
        }
    }
}


int main(void) {
    /* worst case */
    int cell = 23;
    int width, height;
    
    get_size(&width, &height);
    size_t bufsize = (width * height * cell) + (height * 2) + 1;
    
    char lines[MAX_LINES][MAXLEN];
    char filepath[MAXLEN];
    char osid[MAXLEN];
    SysInfo data = {0};

    struct sysinfo info;
    struct utsname kernel_vers;
    struct statvfs vfs;
    struct statfs fs;

    sysinfo(&info);
    get_user_info(&data);
    get_os_info(&kernel_vers, &data, osid);
    define_system(osid, filepath);
    get_pack_info(&data, osid);
    get_hardware(&data);
    get_IP(&data);
    get_env(&data);

    if (statfs("/", &fs) == 0) {
        switch (fs.f_type) {
            case 0xEF53:
                strcpy(data.fstype, "ext4");
                break;
            case 0x01021994:
                strcpy(data.fstype, "tmpfs");
                break;
            case 0x58465342:
                strcpy(data.fstype, "xfs");
                break;
            case 0x9125001D:
                strcpy(data.fstype, "btrfs");
                break;
            default:
                strcpy(data.fstype, "unknown");
                break;
        }
    } else {
        strcpy(data.fstype, "unknown");
    }
    
    if (statvfs("/", &vfs) == 0) {
        unsigned long long total_bytes = (unsigned long long)vfs.f_blocks * vfs.f_frsize;
        unsigned long long occ_bytes = total_bytes - ((unsigned long long)vfs.f_bfree * vfs.f_frsize);
        
        data.totaldisk = total_bytes;
        data.occdisk = occ_bytes;
    } else {
        data.occdisk = 0;
        data.totaldisk = 0;
    }

    data.uptime = info.uptime;

    unsigned long long total_ram = info.totalram * info.mem_unit;
    unsigned long long free_ram = info.freeram * info.mem_unit;

    data.totalram = total_ram;
    data.occram = total_ram - free_ram;
    
    data.totalswap = info.totalswap;
    data.occswap = info.totalswap - info.freeswap;
    
    int lines_count = build_info(&data, lines);

    char *fulldata = malloc(bufsize);

    if (fulldata == NULL) {
        perror(ERROR_COLOR"Error allocating memory"ANSI_RESET);
        return 1;
    }

    if (!proc_img(filepath, width, height, fulldata, bufsize, lines, lines_count, 0)) {
        perror(ERROR_COLOR"Couldn't open the OS logo from 'assets'."ANSI_RESET
                "It might have happened because your system wasn't defined properly");
        return 1;
    } 
    
    printf("%s", fulldata);
    free(fulldata);
    return 0;
}


