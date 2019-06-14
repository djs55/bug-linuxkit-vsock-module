#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/uio.h>
#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <net/if_arp.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <ifaddrs.h>
#include <linux/vm_sockets.h>

#include "protocol.h"
#include "log.h"

/* Support big frames if the server requests it */
const int max_packet_size = 16384;

/* Negotiate a vmnet connection, returns 0 on success and 1 on error. */
int negotiate(int fd, struct vif_info *vif)
{
	enum command command = ethernet;
	struct init_message *me;
	struct ethernet_args args;
	struct init_message you;
	char *txt;

	me = create_init_message();
	if (!me)
		goto err;

	if (write_init_message(fd, me) == -1)
		goto err;

	if (read_init_message(fd, &you) == -1)
		goto err;

	if (me->version != you.version) {
		ERROR("Server did not accept our protocol version (client: %d, server: %d)", me->version, you.version);
		goto err;
	}

	txt = print_init_message(&you);
	if (!txt)
		goto err;

	INFO("Server reports %s", txt);
	free(txt);

	if (write_command(fd, &command) == -1)
		goto err;

	/* We don't need a uuid */
	memset(&args.uuid_string[0], 0, sizeof(args.uuid_string));
	if (write_ethernet_args(fd, &args) == -1)
		goto err;

	if (read_vif_response(fd, vif) == -1)
		goto err;

	return 0;
err:
	ERROR("Failed to negotiate vmnet connection");
	return 1;
}

static int connect_vsocket(long port)
{
	struct sockaddr_vm sa;
	int sock;
	int res;

	sock = socket(AF_VSOCK, SOCK_STREAM, 0);
	if (sock == -1)
		return -1;

	bzero(&sa, sizeof(sa));
	sa.svm_family = AF_VSOCK;
	sa.svm_reserved1 = 0;
	sa.svm_port = port;
	sa.svm_cid = VMADDR_CID_HOST;

	INFO("connecting to port %lx", port);
	res = connect(sock, (const struct sockaddr *)&sa, sizeof(sa));
	if (res == -1){
		if (errno == ENODEV){
			ERROR("connect failed with ENODEV: this kernel is broken");
			/* On some kernels this process cannot be killed because of another AF_VSOCK bug */
			system("/build/result-bad.sh");
			exit(-1);
		}
		INFO("connect failed with %s: is vpnkit running on Windows?", strerror(errno));
		INFO("I'm not sure if this kernel is broken or not.");
		exit(0);
	}

	return sock;
}

void usage(char *name)
{
	printf("%s usage:\n", name);
	printf("\t[--vsock <port>]\n");
	printf("where\n");
	printf("\t--vsock <port>: use <port> as the well-known AF_VSOCK port (defaults to vpnkit ethernet port)\n");
}

int main(int argc, char **argv)
{
	struct vif_info vif;
	unsigned int port = 0;
	int sock = -1;
	int c;

	int option_index;
	static struct option long_options[] = {
		/* These options set a flag. */
		{"verbose", no_argument, NULL, 'v'},
		{0, 0, 0, 0}
	};

	opterr = 0;
	while (1) {
		option_index = 0;

		c = getopt_long(argc, argv, "w:v",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'w':
			port = (unsigned int) strtol(optarg, NULL, 0);
			break;
		case 'v':
			verbose ++;
			break;
		case 0:
			break;
		default:
			usage(argv[0]);
			exit(1);
		}
	}
	if (port == 0) {
		port = 0x30D48B34; /* vpnkit ethernet */
	}
	sock = connect_vsocket(port);
	INFO("connected AF_VSOCK successfully");

	if (negotiate(sock, &vif) != 0) {
		ERROR("failed to negotiate a vpnkit ethernet connection");
		INFO("I'm not sure if this kernel is broken or not.");
		exit(1);
	}

	INFO("VMNET VIF has MAC %02x:%02x:%02x:%02x:%02x:%02x",
		vif.mac[0], vif.mac[1], vif.mac[2],
		vif.mac[3], vif.mac[4], vif.mac[5]
	);
	/* On some kernels this process cannot exit due to another AF_VSOCK bug. */
	system("/build/result-good.sh");
	exit(0);
}
