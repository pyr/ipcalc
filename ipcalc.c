/*
 * $Id: ipcalc.c,v 1.2 2006/12/04 17:06:06 pyr Exp $
 *
 * Copyright (c) 2006 Pierre-Yves Ritschard <pyr@spootnik.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <err.h>

extern const char	*__progname;
static void		 usage(void);
static int		 compar(const void *, const void *);
static u_char		 parse_pfxlen(const char *, in_addr_t *);
static in_addr_t	 parse_net(const char *);

static void		 describe_address(in_addr_t, u_char, int);
static void		 find(in_addr_t, in_addr_t, int, int);
static void		 split(in_addr_t, u_char, char *, int, int);
static void		 describe_mask(u_char, in_addr_t, int);

#define NETM(m)		(htonl(0xffffffff << (32 - (m))))
#define NETW(n, m)	((n) & NETM(m))
#define BCAST(n, m)	((m == 32)?(n):((n) | (htonl(0xffffffff >> (m)))))

void
usage(void)
{
	fprintf(stderr,
		"usage:\n"
		" %s [-c]           addr / mask    (describe network)\n"
		" %s [-c]  -n       mask           (describe mask)\n"
		" %s [-vc] -s list  addr / mask    (split network)\n"
		" %s [-vc] -r       addr : addr    (find networks)\n",
		__progname, __progname, __progname, __progname);
	
	exit(1);
}

int
compar(const void *v1, const void *v2)
{
	const int	 n1 = *((const int *)v1);
	const int	 n2 = *((const int *)v2);

	return (n2 - n1);
}

u_char
parse_pfxlen(const char *s, in_addr_t *padd)
{
        u_int		 i;
        u_char		 pfxlen;
        in_addr_t	 add;
        in_addr_t	 sadd;
        const char	*estr;

        pfxlen = (u_char) strtonum(s, 1, 32, &estr);
        if (!estr) {
                add = 0xffffffff;
                for (i = 0; i < 32 - pfxlen; i++)
                        add &= ~(1 << i);        
                add = htonl(add);
        } else {
                if (!strncmp(s, "0x", 2)) {
                        add = inet_addr(s);
                        if (add == 0xffffffff && strcmp(s, "0xffffffff"))
                                errx(1, "invalid mask definition \"%s\"", s);
                } else {
                        i = inet_pton(AF_INET, s, &add);
                        if (i == -1)
                                err(1, "inet_pton");
                        else if (i == 0)
                                errx(1, "invalid mask definition \"%s\"", s);
                }
                for (sadd = add, pfxlen = 0; sadd; pfxlen++)
                        sadd &= sadd - 1;

                /*
                 * for sanity,
                 * check that bits in the netmask are contiguous.
                 */
                for (sadd = 0xffffffff, i = 0; i < 32 - pfxlen; i++)
                        sadd &= ~(1 << i);
                sadd = htonl(sadd);
                if (sadd != add)
                        errx(1, "invalid mask definition \"%s\"", s);
        }
         
        if (padd)
                *padd = add;
        return (pfxlen);
}

in_addr_t
parse_net(const char *net)
{
	in_addr_t	 add;

	switch (inet_pton(AF_INET, net, &add)) {
	case -1:
		err(1, "inet_pton");
	case 0:
		errx(1, "could not parse address \"%s\"", net);
	default:
		return (add);
	}
}

void
describe_address(in_addr_t add, u_char prefixlen, int cflag)
{
	int			 len;
	int			 af;
	int			 hcount;
	in_addr_t		 netm;
	in_addr_t		 wildc;
	in_addr_t		 netw;
	in_addr_t		 bcast;
	in_addr_t		 hmin;
	in_addr_t		 hmax;
	char			 pbuf[sizeof("255.255.255.255")];

	len = sizeof(pbuf);
	af = AF_INET;

	netm = NETM(prefixlen);
	wildc = ~netm;
	netw = NETW(add, prefixlen);
	bcast = BCAST(add, prefixlen);

	printf("address   : %-16s\n", inet_ntop(af, &add, pbuf, len));
	printf("netmask   : %-16s(0x%08x)\n",
			inet_ntop(af, &netm, pbuf, len), ntohl(netm));
	if (cflag)
		printf("wildcard  : %-16s\n", inet_ntop(af, &wildc, pbuf, len));
	printf("network   : %-16s/%d\n",
			inet_ntop(af, &netw, pbuf, len), prefixlen);

	hcount = 1 << (32 - prefixlen);
	if (prefixlen < 31) {
		hmin = netw | htonl(0x00000001);
		hmax = bcast & ~(htonl(0x00000001));
		hcount -= 2;

		printf("broadcast : %-16s\n", inet_ntop(af, &bcast, pbuf, len));
		printf("host min  : %-16s\n", inet_ntop(af, &hmin, pbuf, len));
		printf("host max  : %-16s\n", inet_ntop(af, &hmax, pbuf, len));
	} else if (prefixlen == 31) {
		printf("host min  : %-16s\n", inet_ntop(af, &netw, pbuf, len));
		printf("host max  : %-16s\n", inet_ntop(af, &bcast, pbuf, len));

	} else /* prefixlen == 32 */
		printf("host route: %-16s\n", inet_ntop(af, &add, pbuf, len));
	printf("hosts/net : %d\n", hcount);
}

void
find(in_addr_t start, in_addr_t end, int verbose, int cflag)
{
	in_addr_t	 net;
	int	 	 i;
	int		 af;
	int		 len;
	char		 pbuf[sizeof("255.255.255.255")];

	af = AF_INET;
	len = sizeof(pbuf);
	for (net = start; ntohl(net) <= ntohl(end); ) {

		for (i = 1;
		     net != NETW(net, i) || ntohl(BCAST(net, i)) > ntohl(end);
		     i++) 
			;

		if (verbose) {
			if (net != start)
				printf("\n");
			describe_address(net, i, cflag);
		} else 
			printf("%s/%d\n", inet_ntop(af, &net, pbuf, len), i);

		net = htonl(ntohl(BCAST(net, i)) + 1);
	}
}

void
split(in_addr_t add, u_char pfxlen, char *masktab, int verbose, int cflag)
{
	int		*tab;
	int		*ptab;
	int		 i;
	int		 e;
	in_addr_t	 net;
	in_addr_t	 max;
	size_t		 sz;
	size_t		 len;
	const char	*estr;
	char		*m;
	char		 pbuf[sizeof("255.255.255.255")];

	len = sizeof(pbuf);
	tab = NULL;
	max = htonl(ntohl(add) + (1 << (32 - pfxlen)));

	for (sz = 0; (m = strsep(&masktab, ",")) != NULL; sz++) {
		i = strtonum(m, 1, INT_MAX, &estr);
		if (estr)
			errx(1, "invalid split element: %s: %s", m, estr);
		if (tab == NULL) {
			if ((tab = calloc(1, sizeof(int))) == NULL)
				err(1, "out of memory");
			tab[0] = i;
		} else {
			if ((tab = realloc(tab, sz + 1)) == NULL)
				err(1, "out of memory");
			tab[sz] = i;
		}
	}
	qsort(tab, sz, sizeof(int), compar);
	if ((ptab = calloc(sz, sizeof(int))) == NULL)
		err(1, "out of memory");

	/*
	 * find adjacent power of 2 for each network size requested,
	 * then infer prefix length needed
	 */

	for (i = 0, net = add; i < sz; i++) {
		for (e = 0; tab[i] > (1 << e) - ((e > 1) ? 2 : 0); e++)
			;
		ptab[i] = 32 - e;
		net = htonl(ntohl(net) + (1 << (32 - ptab[i])));
		if (ntohl(net) > ntohl(max))
			errx(1, "network too small, cannot split");
	}

	net = NETW(add, *ptab);
	for (i = 0; i < sz; i++) {
		if (verbose) {
			if (i)
				printf("\n");
			printf("you want a /%d to store %d IPs\n",
				ptab[i], tab[i]); 
			describe_address(net, ptab[i], cflag);
		} else
			printf("%s/%d\n",
				inet_ntop(AF_INET, &net, pbuf, len), ptab[i]);
		net = htonl(ntohl(net) + (1 << (32 - ptab[i])));
	}

	if (verbose && ntohl(net) < ntohl(BCAST(add, pfxlen))) {
		printf("\nremaining:\n");
		find(net, BCAST(add, pfxlen), 0, 0);
	}
}

void
describe_mask(u_char pfxlen, in_addr_t add, int cflag)
{
	size_t			 len;
	char			 pbuf[sizeof("255.255.255.255")];
	
	len = sizeof("255.255.255.255");

	printf("netmask   : %s\n", inet_ntop(AF_INET, &add, pbuf, len));
	if (cflag) {
		add = ~add;
		printf("wildcard  : %s\n", inet_ntop(AF_INET, &add, pbuf, len));
		add = ~add;
	}
	printf("hex mask  : 0x%08x\n", ntohl(add)); 
	printf("prefixlen : %d\n", pfxlen);
}

int
main(int argc, char *argv[])
{
	int			 c;
	int			 cflag;
	int			 nflag;
	int			 rflag;
	int			 vflag;
	u_char			 pfxlen;
	in_addr_t		 pfx;
	char			*sflag;
	char			*s;
	char			 sepc;
	
	cflag = nflag = rflag = vflag = 0;
	sflag = NULL;
	while ((c = getopt(argc, argv, "cnrs:v")) != -1) {
		switch (c) {
		case 'c':
			cflag = 1;
			break;
		case 'n':
			if (nflag || rflag || sflag != NULL)
				usage();
			nflag = 1;
			break;
		case 'r':
			if (nflag || rflag || sflag != NULL)
				usage();
			rflag = 1;
			break;
		case 's':
			if (nflag || rflag || sflag != NULL)
				usage();
			sflag = strdup(optarg);
			if (sflag == NULL)
				err(1, "strdup");
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1 && argc != 2 && argc != 3)
		usage();

	s = NULL;
	if (!nflag) {
		sepc = rflag ? ':' : '/' ;
		if ((s = strchr(argv[0], sepc)) == NULL) {
			if (argc != 3)
				usage();
			s = argv[2];
		} else {
			if (argc != 1)
				usage();
			*s++ = '\0';
		}
	} else if (argc != 1)
		usage();

	if (rflag)
		find(parse_net(argv[0]), parse_net(s), vflag, cflag);
	else if (sflag)
		split(parse_net(argv[0]), parse_pfxlen(s, NULL),
				sflag, vflag, cflag);
	else if (nflag) {
		pfxlen = parse_pfxlen(argv[0], &pfx);
		describe_mask(pfxlen, pfx, cflag);
	} else
		describe_address(parse_net(argv[0]), parse_pfxlen(s, NULL),
				cflag);
	return (0);
}
