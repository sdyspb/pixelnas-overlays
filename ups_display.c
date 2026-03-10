#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/wait.h>
#include <errno.h>
#include <syslog.h>
#include <math.h>

#define HORIZONTAL_MIRROR

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
    // Degree sign (0x80)
    {'\x80', {0x18, 0x24, 0x24, 0x18, 0x00, 0x00, 0x00, 0x00}},
    // Percent sign
    {'%', {0x66, 0x6C, 0x18, 0x30, 0x66, 0xCC, 0x00, 0x00}},
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
    // Uppercase letters A-Z (some)
    {'A', {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00}},
    {'B', {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}},
    {'C', {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00}},
    {'D', {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00}},
    {'E', {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00}},
    {'F', {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00}},
    {'G', {0x3C, 0x66, 0x60, 0x60, 0x6E, 0x66, 0x3C, 0x00}},
    {'H', {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}},
    {'I', {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}},
    {'J', {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00}},
    {'K', {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00}},
    {'L', {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}},
    {'M', {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00}},
    {'N', {0x66, 0x66, 0x76, 0x7E, 0x6E, 0x66, 0x66, 0x00}},
    {'O', {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}},
    {'P', {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00}},
    {'Q', {0x3C, 0x66, 0x66, 0x66, 0x66, 0x6C, 0x36, 0x00}},
    {'R', {0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x00}},
    {'S', {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00}},
    {'T', {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}},
    {'U', {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}},
    {'V', {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}},
    {'W', {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}},
    {'X', {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00}},
    {'Y', {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00}},
    {'Z', {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00}},
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
    x = screen_width - 1 - x;
#endif
    int byte_index = x / 8;
    int bit_in_byte = x % 8;
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

void clear_top_half(unsigned char *fb, int line_length, int screen_height) {
    if (screen_height < 16) return;
    int top_size = 16 * line_length;
    memset(fb, 0, top_size);
}

// ---------- Battery icon drawing ----------
void draw_battery(unsigned char *fb, int x, int y,
                  int charge_percent, int charging,
                  int line_length, int screen_width, int screen_height) {
    int bat_width = 20;
    int bat_height = 6;
    int bat_x = x;
    int bat_y = y + 1;

    for (int i = 0; i < bat_width; i++) {
        set_pixel(fb, bat_x + i, bat_y, line_length, screen_width, screen_height);
        set_pixel(fb, bat_x + i, bat_y + bat_height - 1, line_length, screen_width, screen_height);
    }
    for (int i = 0; i < bat_height; i++) {
        set_pixel(fb, bat_x, bat_y + i, line_length, screen_width, screen_height);
        set_pixel(fb, bat_x + bat_width - 1, bat_y + i, line_length, screen_width, screen_height);
    }

    int term_x = bat_x + bat_width;
    int term_y = bat_y + 2;
    int term_width = 2;
    int term_height = 2;
    for (int i = 0; i < term_width; i++) {
        for (int j = 0; j < term_height; j++) {
            set_pixel(fb, term_x + i, term_y + j, line_length, screen_width, screen_height);
        }
    }

    int fill_width = (charge_percent * (bat_width - 2)) / 100;
    if (fill_width > 0) {
        for (int i = 0; i < fill_width; i++) {
            for (int j = 1; j < bat_height - 1; j++) {
                set_pixel(fb, bat_x + 1 + i, bat_y + j, line_length, screen_width, screen_height);
            }
        }
    }

    if (charging) {
        int bolt_x = term_x + term_width + 2;
        int bolt_y = y;
        int bolt[6][6] = {
            {0,0,1,0,0,0},
            {0,1,1,0,0,0},
            {1,1,1,1,0,0},
            {0,1,1,1,1,0},
            {0,0,1,1,0,0},
            {0,0,0,1,0,0}
        };
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {
                if (bolt[i][j]) {
                    set_pixel(fb, bolt_x + j, bolt_y + i, line_length, screen_width, screen_height);
                }
            }
        }
    }
}

// ---------- UPS data fetching and parsing ----------
typedef struct {
    float battery_voltage;
    int load;
    float temperature;
    int charge;
    float input_voltage;
    char raw_status[32];
    int valid;
} ups_data_t;

int get_ups_data(ups_data_t *data) {
    FILE *fp;
    char line[256];
    data->valid = 0;
    data->battery_voltage = 0;
    data->load = 0;
    data->temperature = 0;
    data->charge = 0;
    data->input_voltage = 0;
    data->raw_status[0] = '\0';

    fp = popen("upsc pixelups@localhost 2>/dev/null", "r");
    if (!fp) return -1;

    while (fgets(line, sizeof(line), fp)) {
        char key[64], value[64];
        if (sscanf(line, "%63[^:]: %63[^\n]", key, value) == 2) {
            if (strcmp(key, "battery.voltage") == 0)
                data->battery_voltage = atof(value);
            else if (strcmp(key, "ups.load") == 0)
                data->load = atoi(value);
            else if (strcmp(key, "ups.temperature") == 0)
                data->temperature = atof(value);
            else if (strcmp(key, "battery.charge") == 0)
                data->charge = atoi(value);
            else if (strcmp(key, "input.voltage") == 0)
                data->input_voltage = atof(value);
            else if (strcmp(key, "ups.status") == 0) {
                strncpy(data->raw_status, value, sizeof(data->raw_status)-1);
                data->raw_status[sizeof(data->raw_status)-1] = '\0';
            }
        }
    }
    pclose(fp);
    if (data->raw_status[0] != '\0') data->valid = 1;
    return 0;
}

void interpret_status(const char *raw, char *out, int out_len) {
    if (strstr(raw, "ALARM") || strstr(raw, "FAIL")) {
        snprintf(out, out_len, "FAIL");
    } else if (strstr(raw, "LB")) {
        snprintf(out, out_len, "LOW");
    } else {
        snprintf(out, out_len, "OK");
    }
}

// ---------- Main ----------
int main(int argc, char **argv) {
    const char *fbdev = "/dev/fb0";

    int opt;
    while ((opt = getopt(argc, argv, "d:h")) != -1) {
        switch (opt) {
            case 'd':
                fbdev = optarg;
                break;
            case 'h':
                fprintf(stderr, "Usage: %s [-d fbdev]\n", argv[0]);
                return 0;
            default:
                fprintf(stderr, "Usage: %s [-d fbdev]\n", argv[0]);
                return 1;
        }
    }

    openlog("ups_display", LOG_PID | LOG_CONS, LOG_USER);

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

    // Clear top half (lines 0-15)
    clear_top_half(fbp, finfo.line_length, vinfo.yres);

    int value_index = 0;
    int cycle_counter = 0;
    ups_data_t ups;
    char status_word[8] = "OK";
    char value_line[32];

    while (keep_running) {
        if (get_ups_data(&ups) == 0 && ups.valid) {
            interpret_status(ups.raw_status, status_word, sizeof(status_word));

            clear_character_row(fbp, 0, finfo.line_length);
            char status_left[32];
            snprintf(status_left, sizeof(status_left), "UPS: %s", status_word);
            draw_string(fbp, 0, 0, status_left, finfo.line_length, vinfo.xres, vinfo.yres);

            int charging = (ups.input_voltage > 0.0);
            char percent_str[8];
            snprintf(percent_str, sizeof(percent_str), " %d%%", ups.charge);
            int percent_width = strlen(percent_str) * 8;

            int battery_width = 20;
            if (charging) battery_width += 2 + 6;

            int right_part_width = battery_width + percent_width;
            int bat_x = vinfo.xres - right_part_width;
            draw_battery(fbp, bat_x, 0, ups.charge, charging,
                         finfo.line_length, vinfo.xres, vinfo.yres);

            int perc_x = bat_x + battery_width;
            draw_string(fbp, perc_x, 0, percent_str,
                        finfo.line_length, vinfo.xres, vinfo.yres);

            switch (value_index) {
                case 0:
                    snprintf(value_line, sizeof(value_line), "Batt V: %.1f", ups.battery_voltage);
                    break;
                case 1:
                    snprintf(value_line, sizeof(value_line), "Load W: %d", ups.load);
                    break;
                case 2:
                    snprintf(value_line, sizeof(value_line), "Temp %cC: %.1f", 0x80, ups.temperature);
                    break;
            }
        } else {
            clear_character_row(fbp, 0, finfo.line_length);
            draw_string(fbp, 0, 0, "UPS: NO DATA", finfo.line_length, vinfo.xres, vinfo.yres);
            snprintf(value_line, sizeof(value_line), "No data");
        }

        clear_character_row(fbp, 8, finfo.line_length);
        draw_string(fbp, 0, 8, value_line, finfo.line_length, vinfo.xres, vinfo.yres);

        for (int i = 0; i < 1 && keep_running; i++) {
            sleep(1);
        }

        cycle_counter++;
        if (cycle_counter >= 2) {
            cycle_counter = 0;
            value_index = (value_index + 1) % 3;
        }
    }

    clear_top_half(fbp, finfo.line_length, vinfo.yres);
    munmap(fbp, screensize);
    close(fbfd);
    closelog();
    return 0;
}