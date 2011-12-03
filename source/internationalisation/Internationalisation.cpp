
#ifdef ENABLE_NLS
#include <libintl.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TO_STRING(x) #x
#define T_(x) TO_STRING(x)

void initialize_internationalisation()
{
#ifdef ENABLE_NLS
	bindtextdomain("gpick", T_(LOCALEDIR));

	char *td = textdomain("gpick");
	if (!(td && (strcmp(td, "gpick") == 0))){
		fprintf(stderr, "failed to load textdomain \"gpick\"\n");
	}
#endif
}

