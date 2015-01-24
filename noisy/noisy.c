#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ao/ao.h>
#include <sndfile.h>

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <linux/types.h>
#include <linux/netlink.h>

#define syserr(error) _syserr(__FILE__, __LINE__, (error))

void
_syserr(char *file, int line, char *error)
{
	perror(error);
	fprintf(stderr, "at: (%s)%d\n", file, line);
	exit(1);
}

int
play_file(ao_device *dev, char *pathname)
{
	struct SF_INFO info;
	SNDFILE *file = sf_open(pathname, SFM_READ,&info);
	if (!file)
		return -1;
	const int N = 4096;
	short buf[N];
	memset(buf, 0, sizeof(buf));
	for (int i = 0; i < 48000 / N; i++)
		ao_play(dev, (char *) buf, sizeof(buf));
	for (;;) {
		int nsamples = sf_read_short(file, buf, N);
		if (nsamples == 0) break;
		ao_play(dev, (char *) buf, nsamples * sizeof(short));
	}
	sf_close(file);
	return 0;
}

int
main(int argc, char **argv)
{
	if (argc >= 2 && !strcmp(argv[1], "--help")) {
		printf("Usage: %s unplug.wav plugin.wav [devname]\n", argv[0]);
		exit(1);
	}
	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Invalid number of arguments; try --help\n");
		exit(1);
	}
		
	char *unplug_wav = argv[1];
	char *plugin_wav = argv[2];
	char *logical = "/sys/class/power_supply/wireless";
	if (argc == 4)
		logical = argv[3];

	char physical[1024];
	int n = readlink(logical, physical, sizeof(physical) - 1);
	if (n == -1)
		syserr("readlink");
	physical[n] = 0;
	char *devname = strstr(physical, "/devices");
	if (!devname) {
		fprintf(stderr, "Failed to resolve %s, got %s\n",
			logical, physical);
		exit(1);
	}

	/* audio setup */
	ao_initialize();
	int driver = ao_default_driver_id();
	if (driver == -1)
		return 1;
	ao_sample_format fmt;
	fmt.bits = 16;
	fmt.rate = 48000;
	fmt.channels = 2;
	fmt.byte_format = AO_FMT_NATIVE;
	fmt.matrix = 0;
	ao_device *dev = ao_open_live(driver, &fmt, 0);

	/* uevent setup */
	struct sockaddr_nl nls;
	struct pollfd pfd;
	memset(&nls, 0, sizeof(nls));
	nls.nl_family = AF_NETLINK;
	nls.nl_pid = getpid();
	nls.nl_groups = -1;
	int fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (pfd.fd == -1)
		syserr("socket");
	if (bind(fd, (void *) &nls, sizeof(struct sockaddr_nl)))
		syserr("bind");

	/* event loop */
	pfd.events = POLLIN;
	pfd.fd = fd;
	char buf[0x10000];	/* should be enough for everyone */
	for (;;) {
		if (poll(&pfd, 1, -1) == -1)
			syserr("poll");
		int n = recv(pfd.fd, buf, sizeof(buf)-1, MSG_DONTWAIT);
		if (n == -1)
			syserr("recv");
		buf[n] = 0;	/* can just use strcmp below */

		char *ptr = memchr(buf, '@', n);
		if (!ptr)
			continue;
		*ptr++ = 0;
		if (strcmp(buf, "change"))
			continue;

		if (ptr == buf + n)
			continue;

		if (strcmp(ptr, devname))
			continue;

		int online = -1;
		for (;;) {
			ptr += strlen(ptr) + 1;
			if (ptr >= buf + n)
				break;
			const char *key = "POWER_SUPPLY_ONLINE=";
			if (!strncmp(ptr, key, strlen(key))) {
				online = ptr[strlen(key)];
				break;
			}
		}
		switch (online) {
		case -1: case 0:
			break;
		case '0':
			if (dev)
				if (play_file(dev, unplug_wav) == -1)
					fprintf(stderr,
						"libsndfile failed: %s\n",
						unplug_wav);
			break;
		default:
			if (dev)
				if (play_file(dev, plugin_wav) == -1)
					fprintf(stderr,
						"libsndfile failed: %s\n",
						plugin_wav);
			break;
		}
	}

	/* ao_close(dev); */
	/* ao_shutdown(); */
}
