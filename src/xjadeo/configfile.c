#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define XJADEORC "xjadeorc"
#define MAX_LINE_LEN 256 
#define PATH_MAX 1024

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <xjadeo.h>
#include "paths.h"

#ifdef SYSCONFDIR
# define SYSCFGDIR SYSCONFDIR
#else
# define SYSCFGDIR "/etc/"
#endif

/* test if file exists and is a regular file - returns 1 if ok */
int testfile (char *filename) {
  struct stat s;
  int result= stat(filename, &s);
  if (result != 0) return 0; /* stat() failed */
  if (S_ISREG(s.st_mode)) return 1; /* is a regular file - ok */
        return(0); 
}


#define OPTSTR(arg) if (arg) free(arg); arg=strdup(value); rv=1;

extern char OSD_fontfile[1024]; 
extern double     delay;
extern int    videomode;
extern int     seekflags;
extern int want_quiet;
extern int want_verbose;
extern int want_letterbox;
extern int want_nosplash;
extern int mq_en;
extern char *ipc_queue;
extern int remote_en;
extern int avoid_lash;
extern char *midi_driver;
extern int use_jack;
extern int interaction_override;
#if defined (HAVE_LTCSMPTE) || defined (HAVE_LTC)
extern int use_ltc;
#endif 

#ifdef HAVE_MIDI
extern char midiid[128];
extern int midi_clkconvert;  /* --midifps [0:MTC|1:VIDEO|2:RESAMPLE] */
extern int midi_clkadj;    /* 0|1 */
#endif

extern long	userFrame;
extern char *smpte_offset;
extern char *load_movie;
extern double filefps;
extern int osc_port;
extern int want_dropframes;
extern int want_autodrop;
extern int want_genpts;
extern int want_ignstart;
extern int OSD_mode;
extern char OSD_text[128];
extern int OSD_fx, OSD_tx, OSD_sx, OSD_fy, OSD_sy, OSD_ty;
int start_ontop;
int start_fullscreen;

#if (HAVE_LIBXV || HAVE_IMLIB || HAVE_IMLIB2)
 extern int xj_ontop;
 extern int xj_fullscreen;
#endif

extern char	*current_file;
#ifdef JACK_SESSION
extern int jack_session_restore;
extern int js_winx;
extern int js_winy;
extern int js_winw;
extern int js_winh;
#endif

#define YES_OK(VAR) \
    if (!strncasecmp(value,"yes",3)){ \
      VAR = 1; rv=1; \
    } else if (!strncasecmp(value,"no",3)) {\
      rv=1; \
    }


#define YES_NO(VAR) \
    if (!strncasecmp(value,"yes",3)){ \
      VAR = 1; rv=1; \
    } else if (!strncasecmp(value,"no",3)) {\
      VAR = 0; rv=1; \
    }

int parseoption (char *item, char *value) {
  int rv =0;
  if (!strncasecmp(item,"VIDEOMODE",9)) {
    int vmode; 
    vmode=parsevidoutname(value);
    if (vmode==0 ) vmode = atoi(value);
    if (vmode >=0) {
      videomode = vmode; rv=1;
    }
  } else if (!strncasecmp(item,"FPS",3)) {
    if (atof(value) > 0) {
      delay = 1.0 / atof(value);
    } else  delay = -1 ; // use file-framerate
    rv=1;
  } else if (!strncasecmp(item,"MIDICLK",7)) {
    rv=1;
  #ifdef HAVE_MIDI
    YES_NO(midi_clkadj)
  #endif
  } else if (!strncasecmp(item,"MIDIID",6)) {
    rv=1;
  #ifdef HAVE_MIDI
    strncpy(midiid,value,32);
    midiid[31]=0;
  #endif
  } else if (!strncasecmp(item,"MIDISMPTE",9)) {
    rv=1;
  #ifdef HAVE_MIDI
    midi_clkconvert=atoi(value);
  #endif
  } else if (!strncasecmp(item,"MIDIDRIVER",10)) {
  #ifdef HAVE_MIDI
    if (midi_driver) free(midi_driver);
		if (!strcmp(value, "(null)"))
			midi_driver = NULL;
		else
			midi_driver = strdup(value);
  #endif
    rv=1;
  } else if (!strncasecmp(item,"SYNCSOURCE",10)) {
		use_jack=1;
#if defined (HAVE_LTCSMPTE) || defined (HAVE_LTC)
		use_ltc=0;
#endif
		switch (atoi(value)) {
			case 3:
#if defined (HAVE_LTCSMPTE) || defined (HAVE_LTC)
				use_ltc=1;
#endif
#ifdef HAVE_MIDI
				snprintf(midiid,128,"-2"); // no-MTC
#endif
				rv=1;
				break;
			case 2:
				// assert (atoi(midiid >= -1);
				rv=1;
				break;
			case 1:
#ifdef HAVE_MIDI
				snprintf(midiid,128,"-2"); // no-MTC
#endif
				rv=1;
				break;
			case 0:
#ifdef HAVE_MIDI
				snprintf(midiid,128,"-2"); // no-MTC
#endif
				use_jack=0; // // none/userFrame
				rv=1;
				break;
			default:
				break;
		}
  } else if (!strncasecmp(item,"REMOTECTL",9)) {
    YES_NO(remote_en)
  } else if (!strncasecmp(item,"MQ",2)) {
    YES_NO(mq_en)
  } else if (!strncasecmp(item,"IPC",3)) {
		rv=1;
    if (ipc_queue) free(ipc_queue);
		if (!strcmp(value, "(null)"))
			ipc_queue = NULL;
		else
			ipc_queue = strdup(value);
  } else if (!strncasecmp(item,"QUIET",5)) {
    YES_OK (want_quiet);
  } else if (!strncasecmp(item,"VERBOSE",7)) {
    YES_OK (want_verbose);
  } else if (!strncasecmp(item,"NOSPLASH",8)) {
    YES_OK (want_nosplash);
  } else if (!strncasecmp(item,"SEEK",4)) {
    if (!strncasecmp(value,"any",3)){
      seekflags=SEEK_ANY; rv=1;
    } else if (!strncasecmp(value,"cont",4)){
      seekflags=SEEK_CONTINUOUS; rv=1;
    } else if (!strncasecmp(value,"key",3)){
      seekflags=SEEK_KEY; rv=1;
    }
  } else if (!strncasecmp(item,"LETTERBOX",9)) {
    YES_OK(want_letterbox)
  } else if (!strncasecmp(item,"LASH",4)) {
    if (!strncasecmp(value,"no",2)) {
      avoid_lash = 1; rv=1;
    } else if (!strncasecmp(value,"yes",3))
      rv=1;
  } else if (!strncasecmp(item,"MQ",2)) {
    YES_OK(mq_en);
  } else if (!strncasecmp(item,"FONTFILE",8)) {
    strncpy(OSD_fontfile,value,1023);rv=1;
    OSD_fontfile[1023]=0; // just to be sure.
#ifdef JACK_SESSION
  } else if (!strncasecmp(item,"MOVIEFILE",9)) {
		if (load_movie) free(load_movie);
		load_movie = strdup(value); rv=1;
  } else if (!strncasecmp(item,"WINPOS",6)) {
		jack_session_restore=1; rv=1;
		js_winx=atoi(value)>>16;
		js_winy=atoi(value)&0xffff;
  } else if (!strncasecmp(item,"WINPOSX",7)) {
		jack_session_restore=1; rv=1;
		js_winx=atoi(value);
  } else if (!strncasecmp(item,"WINPOSY",7)) {
		jack_session_restore=1; rv=1;
		js_winy=atoi(value);
  } else if (!strncasecmp(item,"WINSIZE",7)) {
		jack_session_restore=1; rv=1;
		js_winw=atoi(value)>>16;
		js_winh=atoi(value)&0xffff;
  } else if (!strncasecmp(item,"WINSIZEW",8)) {
		jack_session_restore=1; rv=1;
		js_winw=atoi(value);
  } else if (!strncasecmp(item,"WINSIZEH",8)) {
		jack_session_restore=1; rv=1;
		js_winh=atoi(value);
  } else if (!strncasecmp(item,"WINONTOP",8)) {
    YES_OK(start_ontop)
  } else if (!strncasecmp(item,"WINFULLSCREEN",13)) {
    YES_OK(start_fullscreen)
  } else if (!strncasecmp(item,"GENPTS",6)) {
    YES_OK(want_genpts)
  } else if (!strncasecmp(item,"IGNORESTART",11)) {
    YES_OK(want_ignstart)
  } else if (!strncasecmp(item,"DROPFRAMES",10)) {
    YES_OK(want_dropframes)
  } else if (!strncasecmp(item,"AUTODF",6)) {
    YES_OK(want_autodrop)
  } else if (!strncasecmp(item,"IAOVERRIDE",6)) {
		interaction_override=atoi(value); rv=1;
  } else if (!strncasecmp(item,"SMPTEOFFSET",11)) {
	  smpte_offset=strdup(value); rv=1;
		/*ts_offset is set from smpte_offset */
  } else if (!strncasecmp(item,"USERFRAME",9)) {
		userFrame=atol(value); rv=1;
  } else if (!strncasecmp(item,"FILEFPS",7)) {
		filefps = atof(value); rv=1;
  } else if (!strncasecmp(item,"OSCPORT",7)) {
		osc_port=atoi(value); rv=1;
  } else if (!strncasecmp(item,"OSDMODE",7)) {
		OSD_mode=atoi(value); rv=1;
  } else if (!strncasecmp(item,"OSDSX",5)) {
		OSD_sx=atoi(value); rv=1;
  } else if (!strncasecmp(item,"OSDSY",5)) {
		OSD_sy=atoi(value); rv=1;
  } else if (!strncasecmp(item,"OSDFX",5)) {
		OSD_fx=atoi(value); rv=1;
  } else if (!strncasecmp(item,"OSDFY",5)) {
		OSD_fy=atoi(value); rv=1;
  } else if (!strncasecmp(item,"OSDTX",5)) {
		OSD_tx=atoi(value); rv=1;
  } else if (!strncasecmp(item,"OSDTY",5)) {
		OSD_ty=atoi(value); rv=1;
  } else if (!strncasecmp(item,"OSDTEXT",7)) {
		snprintf(OSD_text,128,"%s",value); rv=1;
#endif
  }
  return (rv);
}

int readconfig (char *fn) {
         FILE* config_fp;
         char line[MAX_LINE_LEN];
         char* token, *item,*value;
  int lineno=0;
  
         if (!(config_fp = fopen(fn, "r"))) {
    fprintf(stderr,"configfile failed: %s (%s)\n",fn,strerror(errno));
    return (-1);
  }
#if 0
  fprintf(stdout,"INFO: parsing configfile: %s\n",fn);
#endif
         while( fgets( line, MAX_LINE_LEN-1, config_fp ) != NULL ) {
    lineno++;
    line[MAX_LINE_LEN-1]=0;
           token = strtok( line, "\t =\n\r" ) ; 
    if( token != NULL && token[0] != '#' && token[0] != ';') {
      item=strdup(token);
      token = strtok( NULL, "\t =\n\r" ) ; 
      if (!token) {
        free(item);
  #ifdef CFG_WARN_ONLY
        printf("WARNING: ignored line in config file. %s:%d\n",fn,lineno);
        continue;
  #else
        printf("ERROR parsing config file. %s:%d\n",fn,lineno);
        exit(1);
  #endif
      }
      value=strdup(token);
      if (!parseoption(item,value)) {
  #ifdef CFG_WARN_ONLY
        printf("WARNING: ignored error in config file. %s:%d\n",fn,lineno);
  #else
        printf("ERROR parsing config file. %s:%d\n",fn,lineno);
        exit(1);
  #endif
      }
      free(item); free(value);
           }
         }
  fclose(config_fp);
         return 0;
}

void xjadeorc (void) {
        char *home;
        char filename[PATH_MAX];
  if ((strlen(SYSCFGDIR) + strlen(XJADEORC) + 2) < PATH_MAX) {
    sprintf(filename, "%s/%s", SYSCFGDIR, XJADEORC);
    if (testfile(filename)) readconfig(filename);
  }
  home = getenv("HOME");
  if (home && (strlen(home) + strlen(XJADEORC) + 2) < PATH_MAX) {
    sprintf(filename, "%s/.%s", home, XJADEORC);
    if (testfile(filename)) readconfig(filename);
  }
  if (strlen(XJADEORC) + 2 < PATH_MAX) {
    sprintf(filename, "./%s", XJADEORC);
    if (testfile(filename)) readconfig(filename);
  }
}

#define BOOL(VAR) ((VAR)?"yes":"no")

int saveconfig (const char *fn) {
	FILE* fp;
	if (!(fp = fopen(fn, "w"))) {
	if (!want_quiet)
		fprintf(stderr,"writing configfile failed: %s (%s)\n",fn,strerror(errno));
		return -1;
	}
	fprintf(fp, "# config file for xjadeo\n#\n# lines beginning with '#' or ';' are ignored.\n#\n");

	fprintf(fp, "\n## Settings ##\n");
	fprintf(fp, "MOVIEFILE=%s\n", current_file);
	fprintf(fp, "LETTERBOX=%s\n", BOOL(want_letterbox));
	fprintf(fp, "VIDEOMODE=%i\n", videomode); // XXX
	fprintf(fp, "FPS=%f\n", delay<1?-1:1.0/delay);
	fprintf(fp, "SEEK=%s\n", (seekflags==SEEK_ANY)?"any":(seekflags==SEEK_KEY?"key":"cont"));
	fprintf(fp, "OSCPORT=%i\n", osc_port);
	fprintf(fp, "MQ=%s\n", BOOL(mq_en));
	fprintf(fp, "IPC=%s\n", ipc_queue?ipc_queue:"(null)");
	fprintf(fp, "REMOTECTL=%s\n", BOOL(remote_en));
	fprintf(fp, "NOSPLASH=%s\n", BOOL(want_nosplash));
	fprintf(fp, "VERBOSE=%s\n", BOOL(want_verbose));
	fprintf(fp, "QUIET=%s\n", BOOL(want_quiet));
	fprintf(fp, "IAOVERRIDE=%i\n", interaction_override);

	fprintf(fp, "\n## Sync settings ##\n");
#ifdef HAVE_MIDI
	fprintf(fp, "MIDISMPTE=%i\n", midi_clkconvert);
	fprintf(fp, "MIDIDRIVER=%s\n", midi_driver?midi_driver:"(null)");
	fprintf(fp, "MIDIID=%s\n", midiid);
	fprintf(fp, "MIDICLK=%s\n", BOOL(midi_clkadj));
#endif
	fprintf(fp, "USERFRAME=%li\n", userFrame);
	fprintf(fp, "FILEFPS=%f\n", filefps);
	fprintf(fp, "SMPTEOFFSET=%s\n", smpte_offset?smpte_offset:"0");
// use current setting and connection -- not commandline
	int ss =0;
#if defined (HAVE_LTCSMPTE) || defined (HAVE_LTC)
	if (ltcjack_connected()) ss=3;
	else
#endif
#ifdef HAVE_MIDI
	if (midi_connected()) ss=2;
	else
#endif
	if (jack_connected()) ss=1;
	fprintf(fp, "SYNCSOURCE=%i\n", ss);

	fprintf(fp, "\n## Decoder settings ##\n");
	fprintf(fp, "GENPTS=%s\n", BOOL(want_genpts));
	fprintf(fp, "IGNORESTART=%s\n", BOOL(want_ignstart));
	fprintf(fp, "DROPFRAMES=%s\n", BOOL(want_dropframes));
	fprintf(fp, "AUTODF=%s\n", BOOL(want_autodrop));

	fprintf(fp, "\n## OSD ##\n");
	fprintf(fp, "OSDMODE=%i\n", OSD_mode);
	fprintf(fp, "OSDSX=%i\n", OSD_sx);
	fprintf(fp, "OSDSY=%i\n", OSD_sy);
	fprintf(fp, "OSDFX=%i\n", OSD_fx);
	fprintf(fp, "OSDFY=%i\n", OSD_fy);
	fprintf(fp, "OSDTX=%i\n", OSD_tx);
	fprintf(fp, "OSDTY=%i\n", OSD_ty);
	fprintf(fp, "OSDTEXT=%s\n", OSD_text);
	fprintf(fp, "FONTFILE=%s\n", OSD_fontfile);

	fprintf(fp, "\n## WINDOW POSITION/SIZE ##\n");
	unsigned int w,h;
	int x,y;
	Xgetpos(&x,&y); 
	Xgetsize(&w,&h); 

	fprintf(fp, "WINPOSX=%i\n", x);
	fprintf(fp, "WINPOSY=%i\n", y);
	fprintf(fp, "WINSIZEW=%i\n", w);
	fprintf(fp, "WINSIZEH=%i\n", h);

#if (HAVE_LIBXV || HAVE_IMLIB || HAVE_IMLIB2)
	fprintf(fp, "WINONTOP=%s\n", BOOL(xj_ontop));
	fprintf(fp, "WINFULLSCREEN=%s\n", BOOL(xj_fullscreen));
#endif

	fclose(fp);
	if (!want_quiet)
		fprintf(stderr,"written config: %s\n",fn);

  return 0;
}
