/*  spacescope.c
 *  Copyright (C) 1998-2002 Andy Lo A Foe <andy@loafoe.com>
 *  Original code by Tinic Uro
 *
 *  This file is part of AlsaPlayer.
 *
 *  AlsaPlayer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  AlsaPlayer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <dirent.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "scope_config.h"
#include "prefs.h"
#include "ap_unused.h"
#include "../scope_view.h"

#define SPACE_WH	128	/* Leave this at 128 for now */

static char actEq[257];
static char oldEq[257];

static char scX[257];
static char scY[257];

static scope_view view = SCOPE_VIEW_INIT("Spacescope", SPACE_WH, SPACE_WH);
static guint32 palette[256];
static pthread_t spacescope_thread;
static gint running = 0;


static int spacescope_running(void);

static void
spacescope_set_data(void *audio_buffer, int size)
{
	int i;
	short *sound = (short *)audio_buffer;

	if (!sound) {
		memset(&actEq, 0, sizeof(actEq));
		return;
	}
	if (running && sound) {
		char *newset = actEq;
		int bufsize = size / (size >= 512 ? 512 : size);
		for (i=0; i < 256; i++) {
			*newset++=(char)(((int)(*sound)+(int)(*(sound+1)))>>10);
			sound += bufsize;
		}
	}
}


static void
the_spacescope(void)
{
	gint foo, bar;
	char *oldset = oldEq;
	char *newset = actEq;
	guchar bits[(SPACE_WH+1) * (SPACE_WH+1)], *loc;
	int i;

	while (running) {
		memset(bits, 0, SPACE_WH * SPACE_WH);
		memcpy(oldset,newset,256);

		for (i=0; i < 256; i++) {
			foo = (1+(oldset[i]+64))>>1;
			bar = ((( scX[i]*foo)>>7 )+(64)) +
				((((scY[i]*foo) ) + (64 * SPACE_WH)) & 0xffffff80);
			if ((bar > 0) && (bar < (SPACE_WH * SPACE_WH))) {
				loc = bits + bar;
				*loc = foo;
			}
		}
		scope_view_put_indexed(&view, bits, SPACE_WH, palette);
		dosleep(SCOPE_SLEEP);
	}
}

static void stop_spacescope(void);

static gboolean
close_spacescope_window(GtkWidget * UNUSED (widget), GdkEvent * UNUSED (event), gpointer UNUSED (data))
{
	stop_spacescope();
	return TRUE;
}

static void
stop_spacescope(void)
{
	if (running) {
		running = 0;
		pthread_join(spacescope_thread, NULL);
	}
	scope_view_hide(&view);
}

static void *
run_spacescope(void * UNUSED (data))
{
	scope_thread_nice();
	the_spacescope();
	return NULL;
}


static void
start_spacescope(void)
{
	if (running) {
		printf("spacescope already running\n");
		return;
	}
	scope_view_show(&view, G_CALLBACK(close_spacescope_window));
	running = 1;
	if (pthread_create(&spacescope_thread, NULL, run_spacescope, NULL) != 0)
		running = 0;
}


static int init_spacescope(void * UNUSED (arg))
{
	int i;

	/* The GTK2 colour table overflowed its channels (255*8 << 16);
	 * keeping only the low 24 bits reproduces what GdkRgb displayed. */
	for (i = 1; i < 32; i++) {
		palette[i] = ((i*8 << 16) + (255*8 << 8)) & 0xffffff;
		palette[i+31] = ((255*8 << 16) + ((31 - i)*8 << 8)) & 0xffffff;
	}
	palette[63] = (255*8 << 16) & 0xffffff;

	/* Create sin/cos tables */
	for (i = 0; i < 256; i++) {
		scX[i] = (char) (sin(((2*M_PI)/255)*i)*128);
		scY[i] = (char) (-cos(((2*M_PI)/255)*i)*128);
	}

	if (prefs_get_bool(ap_prefs, "spacescope", "active", 0))
		start_spacescope();
	return 1;
}

static void shutdown_spacescope(void)
{
	prefs_set_bool(ap_prefs, "spacescope", "active", spacescope_running());

	stop_spacescope();
	scope_view_destroy(&view);
}


static int spacescope_running(void)
{
	return running;
}


scope_plugin spacescope_plugin = {
	SCOPE_PLUGIN_VERSION,
	"Spacescope",
	"Andy Lo A Foe",
	NULL,
	init_spacescope,
	start_spacescope,
	spacescope_running,
	stop_spacescope,
	shutdown_spacescope,
	spacescope_set_data,
	NULL
};


scope_plugin *scope_plugin_info(void)
{
	return &spacescope_plugin;
}
