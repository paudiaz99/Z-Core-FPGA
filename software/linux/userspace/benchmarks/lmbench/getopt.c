/*
 * Copyright (c) 1997 L.W.McVoy 
 *
 * SGI's fucking getopt doesn't follow GNU's reset policy.  Isn't having
 * N versions of Unix a great thing for the world?  I'm gonna move to NT
 * if these assholes don't get their act together.
 *
 * This version handles
 *
 *	-	(leaves it and returns)
 *	-a
 *	-abcd
 *	-r <arg>
 *	-r<arg>
 *	-abcr <arg>
 *	-abcr<arg>
 *	-r<arg> -R<arg>, etc.
 *
 * A special form is "d|" instead of "d:".  This means the arg has to be
 * right next to the option.  
 * Another special form is "d;".  This means the option must be right next
 * to the option letter and can not be blank.
 */
#include "bench.h"
static char *id = "%@%";

int	optopt;
int     optind;
char   *optarg;
static int n;

int
getopt(int ac, char **av, char *opts)
{
	char	*t;

	if (!optind) {
		optind = 1;
		n = 1;
	}
	debug((stderr, "GETOPT ind=%d n=%d arg=%s av[%d]='%s'\n",
	    optind, n, optarg ? optarg : "", optind, av[optind]));

	if ((optind >= ac) || (av[optind][0] != '-') || !av[optind][1]) {
		return (EOF);
	}

	assert(av[optind][n]);
	for (t = (char *)opts; *t; t++) {
		if (*t == av[optind][n]) {
			break;
		}
	}
	if (!*t) { 
		optopt = av[optind][n];
		debug((stderr, "\tran out of option letters\n"));
		return ('?');
	}

	if (t[1] != ':' && t[1] != '|' && t[1] != ';') {
		if (!av[optind][n+1]) {
			optind++;
			n = 1;
		} else {
			n++;
		}
		debug((stderr, "\tLegit singleton %c\n", *t));
		return (*t);
	}

	if (av[optind][n+1]) {
		if (av[optind][n+1]) {
			optarg = &av[optind][n+1];
		} else {
			optarg = 0;
		}
		optind++;
		n = 1;
		debug((stderr, "\t%c with %s\n", *t, optarg));
		return (*t);
	} 
	
	if (t[1] == '|') {
		optarg = 0;
		optind++;
		n = 1;
		debug((stderr, "\t%c without arg\n", *t));
		return (*t);
	}

	if (t[1] == ';') {
		optarg = 0;
		optind++;
		optopt = *t;
		debug((stderr, "\twanted another word\n"));
		return ('?');
	}

	if ((optind + 1 == ac) || (av[optind+1][0] == '-')) {
		optopt = av[optind][n];
		debug((stderr, "\twanted another word\n"));
		return ('?');
	}
	optarg = av[optind+1];
	optind += 2;
	n = 1;
	debug((stderr, "\t%c with arg %s\n", *t, optarg));
	return (*t);
}

#ifdef	TEST

main(int ac, char **av)
{
	extern	char *optarg;
	extern	int optind;
	char	*comment = 0;
	int	c;

	while ((c = getopt(ac, av, "fnpsx:y|")) != -1) {
		switch (c) {
		    case 'f': 
		    case 'n': 
		    case 'p': 
		    case 's': 
		    	printf("Got option %c\n", c);
			break;
		    case 'x':
		    case 'y':
		   	 comment = optarg; 
			 printf("Got optarg %s with -%c\n", comment, c);
			 break;
		    case '?':
			fprintf(stderr, "bad option %c\n", optopt);
			break;
		    default:
			fprintf(stderr, "unknown ret %c\n", c);
			break;
	    	}
	}
	while (av[optind]) {
		printf("av[%d] = %s\n", optind, av[optind++]);
	}
	exit(0);
}
#endif
