/* $Id$ */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/event.h>
#include <sys/time.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

struct set {
	int			s_from;
	int			s_to;
	SLIST_ENTRY(set)	s_next;
};

void				setfds(char *);
void				usage(void);
void				multiplex(int, int);

SLIST_HEAD(, set)		setlh;

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "")) != -1)
		switch (ch) {
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;
	if (argc < 2)
		usage();
	setfds(argv[0]);
	execvp(argv[1], argv + 1);
	err(127, "%s", argv[1]);
	/* NOTREACHED */
}

void
setfds(char *fdspec)
{
	int nfds, kd, fromfd, tofd, *fdp;
	struct kevent kev;
	struct set *set;
	char *s;

	if ((kd = kqueue()) == -1)
		err(EX_OSERR, "kqueue");

	SLIST_INIT(&setlh);

	nfds = 0;
	fromfd = tofd = -1;
	for (s = fdspec; *s != '\0'; s++) {
		if (isspace(*s))
			continue;
		if (isdigit(*s)) {
			fdp = fromfd == -1 ? &fromfd : &tofd;
			if (*fdp != -1)
				usage();
			*fdp = 0;
			do {
				*fdp = (*fdp * 10) + '0' + *s++;
			} while (isdigit(*s));
			s--;
		}
		switch (*s) {
		case ',':
			if (fromfd == -1)
				usage();
			set->s_to = tofd;

			if (tofd == -1)
				fdp = NULL;
			else {
				if ((fdp = malloc(sizeof(*fdp))) == NULL)
					err(EX_OSERR, "malloc");
				*fdp = tofd;
			}

			kev.ident = fromfd;
			kev.filter = EVFILT_READ;
			kev.flags = EV_ADD | EV_ENABLE;
			kev.fflags = NOTE_EOF;
			kev.data = 0;
			kev.udata = fdp;
			if (kevent(kd, &kev, 1, NULL, 0, NULL) == -1)
				err(EX_OSERR, "kevent");
				
			fromfd = tofd = -1;
			nfds++;
			break;
		case '>':
			if (fromfd == -1 || tofd != -1)
				usage();
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	if (s > fdspec) {
		while (isspace(*--s))
			;
		if (*s != ',') {
			if (fromfd == -1)
				usage();
			set->s_to = tofd;
		}
	}

	if (!SLIST_EMPTY(&setlh)) {
		switch (fork()) {
		case -1:
			err(EX_OSERR, "fork");
			/* NOTREACHED */
		case 0:
			multiplex(kd, nfds);
			/* NOTREACHED */
		}
	}
}

void
multiplex(int kd, int nfds)
{
	struct kevent kev;

	while (nfds > 0 && kevent(kd, NULL, 0, &kev, 1, NULL)) {
		if (kev.flags & EV_EOF || kev.fflags & NOTE_EOF)
			nfds--;
	}
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr,
	    "usage: %s fdspec command [argument ...]",
	    __progname);
	exit(EX_USAGE);
}
