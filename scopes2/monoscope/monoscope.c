/*  monoscope.h
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
#include <pthread.h>
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

#include "scope_config.h"
#include "prefs.h"
#include "alsaplayer_convolve.h"
#include "alsaplayer_error.h"
#include "ap_unused.h"
#include "../scope_view.h"

short newEq[CONVOLVE_BIG];	/* latest block of 512 samples. */
static short copyEq[CONVOLVE_BIG];
int avgEq[CONVOLVE_SMALL];	/* a running average of the last few. */
int avgMax;			/* running average of max sample. */

static scope_view view = SCOPE_VIEW_INIT("Monoscope", 256, 128);
static guint32 palette[256];
static pthread_t monoscope_thread;
static pthread_mutex_t update_mutex = PTHREAD_MUTEX_INITIALIZER;
static int running = 0;
static convolve_state *state = NULL;

static int monoscope_running(void);

static void
monoscope_set_data(void *audio_buffer, int size)
{
	int i;
	short *sound = (short *)audio_buffer;

	if (pthread_mutex_trylock(&update_mutex) != 0) {
		/* alsaplayer_error("missing an update"); */
		return;
	}
	if (!sound) {
		memset(&newEq, 0, sizeof(newEq));
		pthread_mutex_unlock(&update_mutex);
		return;
	}
	if (running && size >= CONVOLVE_BIG) {
		short * newset = newEq;
		int skip = (size / (CONVOLVE_BIG * 2)) * 2;
		for (i = 0; i < CONVOLVE_BIG; i++) {
			*newset++ = (((int) sound[0]) + (int) sound[1]) >> 1;
			sound += skip;
		}
	}
	pthread_mutex_unlock(&update_mutex);
}


static void
the_monoscope(void)
{
	int foo;
	int bar;
	int i, h;
	guchar bits[ 257 * 129];
	guchar *loc;

	while (running) {
		int factor;
		int val;
		int max = 1;
		short * thisEq;
		pthread_mutex_lock(&update_mutex);
		memcpy (copyEq, newEq, sizeof (short) * CONVOLVE_BIG);
		thisEq = copyEq;
#if 1
		val = convolve_match (avgEq, copyEq, state);
		thisEq += val;
#endif
		pthread_mutex_unlock(&update_mutex);
		memset(bits, 0, 256 * 128);
		for (i=0; i < 256; i++) {
			foo = thisEq[i] + (avgEq[i] >> 1);
			avgEq[i] = foo;
			if (foo < 0)
				foo = -foo;
			if (foo > max)
				max = foo;
		}
		avgMax += max - (avgMax >> 8);
		if (avgMax < max)
			avgMax = max; /* Avoid overflow */
		factor = 0x7fffffff / avgMax;
		/* Keep the scaling sensible. */
		if (factor > (1 << 18))
			factor = 1 << 18;
		if (factor < (1 << 8))
			factor = 1 << 8;
		for (i=0; i < 256; i++) {
			foo = avgEq[i] * factor;
			foo >>= 18;
			if (foo > 63)
				foo = 63;
			if (foo < -64)
				foo = -64;
			val = (i + ((foo+64) << 8));
			bar = val;
			if ((bar > 0) && (bar < (256 * 128))) {
				loc = bits + bar;
				if (foo < 0) {
					for (h = 0; h <= (-foo); h++) {
						*loc = h;
						loc+=256;
					}
				} else {
					for (h = 0; h <= foo; h++) {
						*loc = h;
						loc-=256;
					}
				}
			}
		}
		for (i=16;i < 128; i+=16) {
			for (h = 0; h < 256; h+=2) {
				bits[(i << 8) + h] = 63;
				if (i == 64)
					bits[(i << 8) + h + 1] = 63;
			}
		}
		for (i = 16; i < 256; i+=16) {
			for (h = 0; h < 128; h+=2) {
				bits[i + (h << 8)] = 63;
			}
		}
		scope_view_put_indexed(&view, bits, 256, palette);
		dosleep(SCOPE_SLEEP);
	}
}

static void stop_monoscope(void);

static gboolean
close_monoscope_window(GtkWidget * UNUSED (widget), GdkEvent * UNUSED (event), gpointer UNUSED (data))
{
	stop_monoscope();
	return TRUE;
}


static void
init_palette(void)
{
	int i;

	for (i = 1; i < 32; i++) {
		palette[i] = scope_rgb(i * 8, 255, 0);
		palette[i + 31] = scope_rgb(255, (31 - i) * 8, 0);
	}
	palette[63] = scope_rgb(40, 75, 0);
}


static void
stop_monoscope(void)
{
	if (running) {
		running = 0;
		pthread_join(monoscope_thread, NULL);
	}
	scope_view_hide(&view);
}


static void *
run_monoscope(void * UNUSED (data))
{
	scope_thread_nice(); /* Be nice to most processes */
	the_monoscope();
	return NULL;
}


static void
start_monoscope(void)
{
	if (running) {
		printf("monoscope already running\n");
		return;
	}
	scope_view_show(&view, G_CALLBACK(close_monoscope_window));
	running = 1;
	if (pthread_create(&monoscope_thread, NULL, run_monoscope, NULL) != 0)
		running = 0;
}


static int
init_monoscope(void * UNUSED (arg))
{
	state = convolve_init();
	if(!state) return 0;

	init_palette();

	/* FIXME - Need to call convolve_close(state); at some point
	 * if this is going to become a proper plugin. */

	if (prefs_get_bool(ap_prefs, "monoscope", "active", 0))
		start_monoscope();

	return 1;
}

static void shutdown_monoscope(void)
{
	prefs_set_bool(ap_prefs, "monoscope", "active", monoscope_running());

	stop_monoscope();
	scope_view_destroy(&view);
}

static int monoscope_running(void)
{
	return running;
}

scope_plugin monoscope_plugin = {
	SCOPE_PLUGIN_VERSION,
	"Monoscope",
	"Andy Lo A Foe",
	NULL,
	init_monoscope,
	start_monoscope,
	monoscope_running,
	stop_monoscope,
	shutdown_monoscope,
	monoscope_set_data,
	NULL
};


scope_plugin *scope_plugin_info(void)
{
	return &monoscope_plugin;
}
