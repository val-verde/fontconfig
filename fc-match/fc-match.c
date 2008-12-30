/*
 * fontconfig/fc-match/fc-match.c
 *
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#ifdef linux
#define HAVE_GETOPT_LONG 1
#endif
#define HAVE_GETOPT 1
#endif

#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_GETOPT
#define HAVE_GETOPT 0
#endif
#ifndef HAVE_GETOPT_LONG
#define HAVE_GETOPT_LONG 0
#endif

#if HAVE_GETOPT_LONG
#undef  _GNU_SOURCE
#define _GNU_SOURCE
#include <getopt.h>
static const struct option longopts[] = {
    {"sort", 0, 0, 's'},
    {"all", 0, 0, 'a'},
    {"verbose", 0, 0, 'v'},
    {"format", 1, 0, 'f'},
    {"version", 0, 0, 'V'},
    {"help", 0, 0, 'h'},
    {NULL,0,0,0},
};
#else
#if HAVE_GETOPT
extern char *optarg;
extern int optind, opterr, optopt;
#endif
#endif

static void
usage (char *program, int error)
{
    FILE *file = error ? stderr : stdout;
#if HAVE_GETOPT_LONG
    fprintf (file, "usage: %s [-savVh] [-f FORMAT] [--sort] [--all] [--verbose] [--format=FORMAT] [--version] [--help] [pattern]\n",
	     program);
#else
    fprintf (file, "usage: %s [-savVh] [-f FORMAT] [pattern]\n",
	     program);
#endif
    fprintf (file, "List fonts matching [pattern]\n");
    fprintf (file, "\n");
#if HAVE_GETOPT_LONG
    fprintf (file, "  -s, --sort           display sorted list of matches\n");
    fprintf (file, "  -a, --all            display unpruned sorted list of matches\n");
    fprintf (file, "  -v, --verbose        display entire font pattern\n");
    fprintf (file, "  -f, --format=FORMAT  use the given output format\n");
    fprintf (file, "  -V, --version        display font config version and exit\n");
    fprintf (file, "  -h, --help           display this help and exit\n");
#else
    fprintf (file, "  -s,        (sort)    display sorted list of matches\n");
    fprintf (file, "  -a         (all)     display unpruned sorted list of matches\n");
    fprintf (file, "  -v         (verbose) display entire font pattern\n");
    fprintf (file, "  -f FORMAT  (format)  use the given output format\n");
    fprintf (file, "  -V         (version) display font config version and exit\n");
    fprintf (file, "  -h         (help)    display this help and exit\n");
#endif
    exit (error);
}

int
main (int argc, char **argv)
{
    int		verbose = 0;
    int		sort = 0, all = 0;
    FcChar8     *format = NULL;
    int		i;
    FcFontSet	*fs;
    FcPattern   *pat;
    FcResult	result;
#if HAVE_GETOPT_LONG || HAVE_GETOPT
    int		c;

#if HAVE_GETOPT_LONG
    while ((c = getopt_long (argc, argv, "asvf:Vh", longopts, NULL)) != -1)
#else
    while ((c = getopt (argc, argv, "asvf:Vh")) != -1)
#endif
    {
	switch (c) {
	case 'a':
	    all = 1;
	    break;
	case 's':
	    sort = 1;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'f':
	    format = (FcChar8 *) strdup (optarg);
	    break;
	case 'V':
	    fprintf (stderr, "fontconfig version %d.%d.%d\n", 
		     FC_MAJOR, FC_MINOR, FC_REVISION);
	    exit (0);
	case 'h':
	    usage (argv[0], 0);
	default:
	    usage (argv[0], 1);
	}
    }
    i = optind;
#else
    i = 1;
#endif

    if (!FcInit ())
    {
	fprintf (stderr, "Can't init font config library\n");
	return 1;
    }
    if (argv[i])
	pat = FcNameParse ((FcChar8 *) argv[i]);
    else
	pat = FcPatternCreate ();

    if (!pat)
	return 1;

    FcConfigSubstitute (0, pat, FcMatchPattern);
    FcDefaultSubstitute (pat);
    
    fs = FcFontSetCreate ();

    if (sort || all)
    {
	FcFontSet	*font_patterns;
	int	j;
	font_patterns = FcFontSort (0, pat, all ? FcFalse : FcTrue, 0, &result);

	for (j = 0; j < font_patterns->nfont; j++)
	{
	    FcPattern  *font_pattern;

	    font_pattern = FcFontRenderPrepare (NULL, pat, font_patterns->fonts[j]);
	    if (font_pattern)
		FcFontSetAdd (fs, font_pattern);
	}

	FcFontSetSortDestroy (font_patterns);
    }
    else
    {
	FcPattern   *match;
	match = FcFontMatch (0, pat, &result);
	if (match)
	    FcFontSetAdd (fs, match);
    }
    FcPatternDestroy (pat);

    if (fs)
    {
	int	j;

	for (j = 0; j < fs->nfont; j++)
	{
	    if (verbose)
	    {
		FcPatternPrint (fs->fonts[j]);
	    }
	    else if (format)
	    {
	        FcChar8 *s;

		s = FcPatternFormat (fs->fonts[j], format);
		printf ("%s", s);
		free (s);
	    }
	    else
	    {
		FcChar8	*family;
		FcChar8	*style;
		FcChar8	*file;

		if (FcPatternGetString (fs->fonts[j], FC_FILE, 0, &file) != FcResultMatch)
		    file = (FcChar8 *) "<unknown filename>";
		else
		{
		    FcChar8 *slash = (FcChar8 *) strrchr ((char *) file, '/');
		    if (slash)
			file = slash+1;
		}
		if (FcPatternGetString (fs->fonts[j], FC_FAMILY, 0, &family) != FcResultMatch)
		    family = (FcChar8 *) "<unknown family>";
		if (FcPatternGetString (fs->fonts[j], FC_STYLE, 0, &style) != FcResultMatch)
		    style = (FcChar8 *) "<unknown style>";

		printf ("%s: \"%s\" \"%s\"\n", file, family, style);
	    }
	}
	FcFontSetDestroy (fs);
    }
    FcFini ();
    return 0;
}
