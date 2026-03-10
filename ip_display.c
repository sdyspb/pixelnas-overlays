#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <errno.h>
#include <syslog.h>

#define HORIZONTAL_MIRROR

// ---------- Font table (full lowercase, uppercase, digits, punctuation) ----------
struct glyph {
    char c;
    unsigned char data[8];
};

static struct glyph font[] = {
    // Digits 0-9
    {'0', {0x3C, 0x66, 0x6E, 0x7E, 0x76, 0x66, 0x3C, 0x00}},
    {'1', {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}},
    {'2', {0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00}},
    {'3', {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00}},
    {'4', {0x0C, 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x00}},
    {'5', {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00}},
    {'6', {0x3C, 0x66, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00}},
    {'7', {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00}},
    {'8', {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00}},
    {'9', {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x66, 0x3C, 0x00}},
    // Space and punctuation
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00}},
    {':', {0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00}},
    // Lowercase letters a-z
    {'a', {0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00}},
    {'b', {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00}},
    {'c', {0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x00}},
    {'d', {0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00}},
    {'e', {0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00}},
    {'f', {0x1C, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x30, 0x00}},
    {'g', {0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x7C}},
    {'h', {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00}},
    {'i', {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00}},
    {'j', {0x0C, 0x00, 0x1C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38}},
    {'k', {0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00}},
    {'l', {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}},
    {'m', {0x00, 0x00, 0x6C, 0xFE, 0xFE, 0xD6, 0xC6, 0x00}},
    {'n', {0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00}},
    {'o', {0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00}},
    {'p', {0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60}},
    {'q', {0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x06}},
    {'r', {0x00, 0x00, 0x7C, 0x66, 0x60, 0x60, 0x60, 0x00}},
    {'s', {0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00}},
    {'t', {0x30, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x1C, 0x00}},
    {'u', {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00}},
    {'v', {0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}},
    {'w', {0x00, 0x00, 0xC6, 0xD6, 0xFE, 0x7C, 0x6C, 0x00}},
    {'x', {0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00}},
    {'y', {0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C}},
    {'z', {0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00}},
    // Uppercase letters (some)
    {'P', {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00}},
    {'F', {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00}},
    {'L', {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}},
    {'O', {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}},
    {'K', {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00}},
    {'B', {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}},
    {'A', {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00}},
    {'T', {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}},
    {'R', {0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x00}},
    {'Y', {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00}},
};

#define FONT_SIZE (sizeof(font)/sizeof(font[0]))

static volatile int keep_running = 1;

void handle_signal(int sig) {
    (void)sig;
    keep_running = 0;
}

// ---------- Framebuffer drawing functions ----------
static inline void set_pixel(unsigned char *fb, int x, int y,
                             int line_length, int screen_width, int screen_height) {
    if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) return;
#ifdef HORIZONTAL_MIRROR
    x = screen_width - 1 - x;   // mirror horizontally
#endif
    int byte_index = x / 8;
    int bit_in_byte = x % 8;     // 0 = leftmost, 7 = rightmost
    long offset = y * line_length + byte_index;
    fb[offset] |= (1 << bit_in_byte);
}

void draw_char(unsigned char *fb, int x, int y, char c,
               int line_length, int screen_width, int screen_height) {
    for (int i = 0; i < FONT_SIZE; i++) {
        if (font[i].c == c) {
            unsigned char *glyph = font[i].data;
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    if (glyph[row] & (1 << (7 - col))) {
                        int px = x + col;
                        int py = y + row;
                        set_pixel(fb, px, py, line_length, screen_width, screen_height);
                    }
                }
            }
            return;
        }
    }
    // Character not found – silently ignore
}

void draw_string(unsigned char *fb, int x, int y, const char *str,
                 int line_length, int screen_width, int screen_height) {
    while (*str) {
        draw_char(fb, x, y, *str, line_length, screen_width, screen_height);
        x += 8;
        if (x + 8 > screen_width) break;
        str++;
    }
}

void clear_line(unsigned char *fb, int y, int line_length) {
    memset(fb + y * line_length, 0, line_length);
}

void clear_character_row(unsigned char *fb, int y_start, int line_length) {
    for (int row = 0; row < 8; row++) {
        clear_line(fb, y_start + row, line_length);
    }
}

void clear_bottom_half(unsigned char *fb, int line_length, int screen_height) {
    if (screen_height <= 16) return;
    int bottom_start = 16 * line_length;
    int bottom_size = (screen_height - 16) * line_length;
    memset(fb + bottom_start, 0, bottom_size);
}

// ---------- Network interface handling ----------
#define MAX_IFACES 16
struct iface_info {
    char name[IFNAMSIZ];
};

static struct iface_info ifaces[MAX_IFACES];
static int iface_count = 0;
static int current_iface = 0;

void update_iface_list() {
    struct ifaddrs *ifaddr, *ifa;
    iface_count = 0;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;

        int found = 0;
        for (int i = 0; i < iface_count; i++) {
            if (strcmp(ifaces[i].name, ifa->ifa_name) == 0) {
                found = 1;
                break;
            }
        }
        if (!found && iface_count < MAX_IFACES) {
            strncpy(ifaces[iface_count].name, ifa->ifa_name, IFNAMSIZ-1);
            ifaces[iface_count].name[IFNAMSIZ-1] = 0;
            iface_count++;
        }
    }
    freeifaddrs(ifaddr);
}

void fix_current_iface() {
    if (iface_count == 0) {
        current_iface = 0;
    } else if (current_iface >= iface_count) {
        current_iface = 0;
    }
}

int get_ip(const char *iface, char *ip_buf, size_t buf_len) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return -1;

    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        close(fd);
        return -1;
    }

    struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
    strncpy(ip_buf, inet_ntoa(sin->sin_addr), buf_len);
    ip_buf[buf_len - 1] = 0;
    close(fd);
    return 0;
}

// ---------- Main ----------
int main(int argc, char **argv) {
    const char *fbdev = "/dev/fb0";
    const char *inputdev = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "d:e:h")) != -1) {
        switch (opt) {
            case 'd':
                fbdev = optarg;
                break;
            case 'e':
                inputdev = optarg;
                break;
            case 'h':
                fprintf(stderr, "Usage: %s [-d fbdev] [-e input_event_device]\n", argv[0]);
                return 0;
            default:
                fprintf(stderr, "Usage: %s [-d fbdev] [-e input_event_device]\n", argv[0]);
                return 1;
        }
    }

    openlog("ip_display", LOG_PID | LOG_CONS, LOG_USER);

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    int fbfd = open(fbdev, O_RDWR);
    if (fbfd < 0) {
        syslog(LOG_ERR, "Failed to open framebuffer %s: %m", fbdev);
        closelog();
        return 1;
    }

    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0 ||
        ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        syslog(LOG_ERR, "Failed to get framebuffer info: %m");
        close(fbfd);
        closelog();
        return 1;
    }

    syslog(LOG_INFO, "Using %s: %dx%d, %d bpp, line length %d",
           fbdev, vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, finfo.line_length);

    if (vinfo.xres != 128 || vinfo.yres != 32) {
        syslog(LOG_WARNING, "Unexpected resolution %dx%d", vinfo.xres, vinfo.yres);
    }

    long screensize = finfo.smem_len;
    unsigned char *fbp = mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fbp == MAP_FAILED) {
        syslog(LOG_ERR, "Failed to mmap framebuffer: %m");
        close(fbfd);
        closelog();
        return 1;
    }

    int input_fd = -1;
    if (inputdev) {
        input_fd = open(inputdev, O_RDONLY | O_NONBLOCK);
        if (input_fd < 0) {
            syslog(LOG_WARNING, "Failed to open input device %s: %m, continuing without button", inputdev);
        }
    } else {
        syslog(LOG_INFO, "No input device specified, button disabled");
    }

    // Clear bottom half (lines 16-31)
    clear_bottom_half(fbp, finfo.line_length, vinfo.yres);

    update_iface_list();
    fix_current_iface();

    while (keep_running) {
        char first_line[IFNAMSIZ + 2] = "no iface";
        char ip_str[16] = "...";
        const char *display_ip = ip_str;

        if (iface_count > 0 && current_iface < iface_count) {
            snprintf(first_line, sizeof(first_line), "%s:", ifaces[current_iface].name);
            if (get_ip(ifaces[current_iface].name, ip_str, sizeof(ip_str)) == 0) {
                display_ip = ip_str;
            }
        }

        // Draw network info in bottom half (lines 16 and 24)
        clear_character_row(fbp, 16, finfo.line_length);
        clear_character_row(fbp, 24, finfo.line_length);

        draw_string(fbp, 0, 16, first_line, finfo.line_length, vinfo.xres, vinfo.yres);
        draw_string(fbp, 0, 24, display_ip, finfo.line_length, vinfo.xres, vinfo.yres);

        if (input_fd >= 0) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(input_fd, &fds);
            struct timeval tv;
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            int ret = select(input_fd + 1, &fds, NULL, NULL, &tv);
            if (ret < 0) {
                if (errno == EINTR) continue;
                syslog(LOG_ERR, "select error: %m");
                break;
            }
            if (ret > 0 && FD_ISSET(input_fd, &fds)) {
                struct input_event ev;
                while (read(input_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                    if (ev.type == EV_KEY && ev.code == 28 && ev.value == 1) {
                        syslog(LOG_INFO, "Button pressed, switching interface");
                        update_iface_list();
                        if (iface_count > 0) {
                            current_iface = (current_iface + 1) % iface_count;
                        }
                        fix_current_iface();
                        break;
                    }
                }
                continue;
            }
        } else {
            for (int i = 0; i < 2 && keep_running; i++) {
                sleep(1);
            }
        }
    }

    clear_bottom_half(fbp, finfo.line_length, vinfo.yres);
    munmap(fbp, screensize);
    close(fbfd);
    if (input_fd >= 0) close(input_fd);
    closelog();
    return 0;
}