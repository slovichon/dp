/* $Id$ */

int
main(int argc, char *argv[])
{
	int ch;

	while ((i = getopt(argc, argv, "")) != -1)
		switch (i) {
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

struct set {
	int			s_from;
	int			s_to;
	SLIST_ENTRY(set)	s_next;
};

SLIST_HEAD(, setlh);

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
				if (fdp = malloc(sizeof(*fdp)) == NULL)
					err(EX_OSERR, "malloc");
				*fdp = tofd;
			}

			kev.ident = fromfd;
			kev.filter = EVFILT_READ;
			kev.flags = EV_ADD | EV_ENABLE;
			kev.fflags = NOTE_EOF;
			kev.data = 0;
			kev.udata = fdp;
			if (kevent(kq, kev, 1, NULL, 0, NULL) == -1)
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

	if (!SLIST_EMPTY(&set)) {
		switch (fork()) {
		case -1:
			err(EX_OSERR, "fork");
			/* NOTREACHED */
		case 0:
			mutiplex(kq, nfds);
			/* NOTREACHED */
		}
	}
}

void
multiplex(int kq, int nfds)
{
	struct kevent kev;

	while (nfds > 0 && kevent(kq, NULL, 0, &kev, 1, NULL)) {
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
