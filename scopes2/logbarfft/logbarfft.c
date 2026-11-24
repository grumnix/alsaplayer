/*  barfft.cpp
 *  Copyright (C) 1999-2002 Andy Lo A Foe <andy@loafoe.com>
 *  Based on code by Richard Boulton <richard@tartarus.org>
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
#include "ap_unused.h"
#include "../scope_view.h"

#define BARS 16

static scope_view view = SCOPE_VIEW_INIT("logFFTscope", 256, 128);
static guint32 palette[256];
static int fft_buf[512];
static int maxbar[BARS];
static pthread_t fftscope_thread;
static int running = 0;

#if 0
static int xranges[] = {1, 2, 3, 4, 6, 8, 11, 15, 21,
			29, 40, 54, 74, 101, 137, 187, 255};
#endif
#if 0
static int xranges[] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 13, 15, 17, 19, 21, 23, 25, 29, 33, 37, 40, 44, 48, 54, 61, 67, 74, 90, 101, 137, 158, 187, 255};
#endif
static int xranges[] = {0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255};

static int fftscope_running(void);


static void
logscope_set_fft(void *fft_data, int samples, int channels)
{
	if (!fft_data) {
			memset(fft_buf, 0, sizeof(fft_buf));
			return;
	}
	memcpy(fft_buf, fft_data, sizeof(int) * samples * channels);
}

static void the_fftscope(void)
{
	guchar *loc;
	guchar bits [256 * 129];
	int i, h;

	while (running) {
		guint val;
		gint j;
		gint k;

		memset(bits, 0, 256 * 128);

		for (i=0; i < BARS; i++) {
			val = 0;
			for (j = xranges[i]; j < xranges[i + 1]; j++) {
				/* k = (guint)(sqrt(fftout[j]) * fftmult); */
				k = (fft_buf[j] + fft_buf[256+j]) / 256;
				val += k;
			}
			if(val > 127) val = 127;
			if (val > (guint)maxbar[ i ])
				maxbar[ i ] = val;
			else {
				k = maxbar[ i ] - (4 + (8 / (128 - maxbar[ i ])));
				val = k > 0 ? k : 0;
				maxbar[ i ] = val;
			}
			loc = bits + 256 * 128;
			for (h = val; h > 0; h--) {
				for (j = (256 / BARS) * i + 0; j < (256 / BARS) * i + ((256 / BARS) - 1); j++) {
					*(loc + j) = val-h;
				}
				loc -=256;
			}
		}
		scope_view_put_indexed(&view, bits, 256, palette);
		dosleep(SCOPE_SLEEP);
	}
}


static void stop_fftscope(void);

static gboolean
close_fftscope_window(GtkWidget * UNUSED (widget), GdkEvent * UNUSED (event), gpointer UNUSED (data))
{
	stop_fftscope();
	return TRUE;
}


static void stop_fftscope(void)
{
	if (running) {
		running = 0;
		pthread_join(fftscope_thread, NULL);
	}
	scope_view_hide(&view);
}


static void *
run_fftscope(void * UNUSED (data))
{
	scope_thread_nice(); /* Be nice to most processes */
	the_fftscope();
	return NULL;
}


static void start_fftscope(void)
{
	if (running) {
		printf("fftscope already running\n");
		return;
	}
	scope_view_show(&view, G_CALLBACK(close_fftscope_window));
	running = 1;
	if (pthread_create(&fftscope_thread, NULL, run_fftscope, NULL) != 0)
		running = 0;
}


static int
init_fftscope(void * UNUSED (arg))
{
	int i;

	for (i = 0; i < BARS; i++) {
		maxbar[ i ] = 0;
	}

	for (i = 1; i < 64; i++) {
		palette[i] = scope_rgb(i * 4, 255, 0);
		palette[i + 63] = scope_rgb(255, (63 - i) * 4, 0);
	}

	if (prefs_get_bool(ap_prefs, "logbarfft", "active", 0))
		start_fftscope();

	return 1;
}

static void shutdown_fftscope(void)
{
	prefs_set_bool(ap_prefs, "logbarfft", "active", fftscope_running());

	stop_fftscope();
	scope_view_destroy(&view);
}

static int fftscope_running(void)
{
	return running;
}

scope_plugin logscope_plugin = {
	SCOPE_PLUGIN_VERSION,
	"logFFTscope",
	"Andy Lo A Foe",
	NULL,
	init_fftscope,
	start_fftscope,
	fftscope_running,
	stop_fftscope,
	shutdown_fftscope,
	NULL,
	logscope_set_fft,
};


scope_plugin *scope_plugin_info(void)
{
	return &logscope_plugin;
}

