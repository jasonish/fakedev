#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <err.h>

const char default_ifname[] = "fake0";

static void
up(const char *ifname)
{
    int sockfd = socket(AF_PACKET, SOCK_RAW, 0);
    if (sockfd == -1) {
        err(1, "socket");
    }

    struct ifreq ifreq;
    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifreq) == -1) {
        err(1, "ioctl");
    }
    ifreq.ifr_flags = IFF_UP;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifreq) == -1) {
        err(1, "ioctl");
    }

    close(sockfd);
}

int
main(int argc, char **argv)
{
    struct ifreq ifr;
    int tapfd;
    int rc;
    const char *ifname = default_ifname;

    if (getenv("FAKEDEV_IFNAME") != NULL) {
        ifname = getenv("FAKEDEV_IFNAME");
    }

    printf("Creating interface %s.\n", ifname);
    tapfd = open("/dev/net/tun", O_RDWR);
    if (tapfd == -1) {
        err(1, "Failed to open /dev/net/tun");
    }
    
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    
    rc = ioctl(tapfd, TUNSETIFF, (void *)&ifr);
    if (rc == -1) {
        err(1, "ioctl failed");
    }

    printf("Bring up interface %s.\n", ifname);
    up(ifname);

    printf("Entering infinite read loop...\n");

    /* Must read packets off the interface - just read the minimum
     * required. */
    uint8_t buf[4];
    ssize_t len;
    for (;;) {
        len = read(tapfd, buf, sizeof(buf));
        if (len == -1) {
            err(1, "read: %s", ifname);
        }
    }
}
