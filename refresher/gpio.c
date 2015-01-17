/* -*- indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/types.h>
#include <dirent.h>
#include <poll.h>

static int
find_input_event(int bustype, int vendor, int product, int version)
{
        char dir[] = "/dev/input/";
        DIR *input = opendir(dir);
        if (!input)
                return -1;
        int res = -1;
        while (res == -1) {
                struct dirent ent;
                struct dirent *ptr;
                readdir_r(input, &ent, &ptr);
                if (!ptr)
                        break;
                char *name = malloc(sizeof(dir) + strlen(ent.d_name));
                if (!name) exit(1);
                strcpy(name, dir);
                strcpy(name + sizeof(dir) - 1, ent.d_name);
                int fd = open(name, O_RDONLY | O_NONBLOCK);
                free(name);
                if (fd == -1)
                        continue;
                struct input_id id;
                if (ioctl(fd, EVIOCGID, &id) != -1)
                        if (id.bustype == bustype
                            && id.vendor == vendor
                            && id.product == product
                            && id.version == version)
                        {
                                res = fd;
                                break;
                        } else
                                close(fd);
        }
        closedir(input);
        return res;
}

int
open_gpio_keys()
{
        int fd = find_input_event(BUS_HOST, 1, 1, 256);
        if (fd != -1)
                if (ioctl(fd, EVIOCGRAB, 1) == -1)
                        perror("EVIOCGRAB");
        return fd;
}

int
read_gpio(int fd)
{
        struct input_event ev;
        int n = read(fd, &ev, sizeof(ev));
        if (n == -1 && errno == EAGAIN)
                return 0;
        if (n != sizeof(ev)) {
                fprintf(stderr, "failed to read intact struct input: %d %d",
                        n, sizeof(ev));
                exit(1);
        }
        /* we don't bother to look for EV_SYN, and just act on EV_KEY
         * immediately */
        if (ev.type == EV_KEY && ev.code == KEY_POWER)
                return ev.value ? 1 : -1;
        return fd;
}

int
poll_gpio(int fd, long us_timeout)
{
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = us_timeout * 1000;

        struct pollfd pf;
        pf.fd = fd;
        pf.events = POLLIN;

        int nready = ppoll(&pf, 1, us_timeout > 0 ? &ts : 0, 0);
        if (nready == -1)
                return -1;
        if (pf.revents & POLLIN)
                return read_gpio(fd);
        return 0;
}

int
set_backlight(int brightness)
{
        int fd = open("/sys/class/leds/lcd-backlight/brightness", O_WRONLY);
        if (fd == -1)
                return -1;
        char decimal[4];
        int n = snprintf(decimal, sizeof(decimal), "%d", brightness);
        if (n < sizeof(decimal))
                if (write(fd, decimal, n) == -1) {
                        close(fd);
                        return -1;
                }
        close(fd);
        return 0;
}
