/*  levelmeter.c
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
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "scope_config.h"
#include "prefs.h"
#include "ap_unused.h"
#include "../scope_view.h"

#define DECAY	3

#define LM_WIDTH	256
#define LM_HEIGHT	40
#define LM_BAR_HEIGHT	18

static char actlEq[257];
static char oldlEq[257];
static char actrEq[257];
static char oldrEq[257];

static char scX[256];
static char scY[256];

static scope_view view = SCOPE_VIEW_INIT("Levelmeter", LM_WIDTH, LM_HEIGHT);
static guint32 bar_colors[LM_WIDTH];	/* colour of each meter column */
static pthread_t levelmeter_thread;
static gint running = 0;	/* the render thread loops while set */


static int levelmeter_running(void);

static void levelmeter_set_data(void *audio_buffer, int size)
{
	short i;
	short *sound = (short *)audio_buffer;

	if (running && sound) {
		char *newsetl = actlEq;
	        char *newsetr = actrEq;
		int bufsize=size/512;
        	for(i=0;i<256;i++) {
                	*newsetl++=(char)((int)(*(sound  ))>>8);
                	*newsetr++=(char)((int)(*(sound+1))>>8);
                	sound+=bufsize;
        	}
        }
}


/* Copy columns [x0, x1) of the meter gradient into rows y0..y0+BAR_HEIGHT. */
static void
draw_bar(guint32 *px, int y0, int x0, int x1)
{
	int x, y;

	if (x0 < 0)
		x0 = 0;
	if (x1 > LM_WIDTH)
		x1 = LM_WIDTH;
	for (y = y0; y < y0 + LM_BAR_HEIGHT && y < LM_HEIGHT; y++)
		for (x = x0; x < x1; x++)
			px[y * LM_WIDTH + x] = bar_colors[x];
}


static void
the_levelmeter(void)
{
	static int oldl = 0;
	static int oldr = 0;
	char *oldsetl = oldlEq;
	char *newsetl = actlEq;
	char *oldsetr = oldrEq;
	char *newsetr = actrEq;
	int levell, levelr;
	int maxl = 0, maxr = 0, count = 0;
	int i;
	guint32 *px;

	while (running) {
		memcpy(oldsetl, newsetl, 256);
		memcpy(oldsetr, newsetr, 256);

		count++;

		if (count > 30) {
			count = 0;
			maxl = 0;
			maxr = 0;
		}
		levell = 0;
		for (i = 0; i < 256; i++) {
			if (oldsetl[i] > 0) {
				levell=MAX(levell,oldsetl[i]);
			} else {
				levell=MAX(levell,-oldsetl[i]);
			}
		}

		levelr = 0;
		for (i = 0; i < 256; i++) {
			if (oldsetr[i] > 0) {
				levelr=MAX(levelr, oldsetr[i]);
			} else {
				levelr=MAX(levelr,-oldsetr[i]);
			}
		}
		levelr >>= 1;
		levell >>= 1;

		if (oldr > 0 && (oldr-=2) > levelr)
				levelr = oldr;
		else oldr = levelr;

		if (oldl > 0 && (oldl-=2) > levell)
				levell = oldl;
		else oldl = levell;

		if (maxl < levell<<2) {
			maxl = (levell<<2)-4;
			count = 0;
		}
		if (maxr < levelr<<2) {
			maxr = (levelr<<2)-4;
			count = 0;
		}
		px = scope_view_lock(&view);
		if (px) {
			memset(px, 0, LM_WIDTH * LM_HEIGHT * sizeof(*px));
			draw_bar(px, 1, 0, levell<<2);
			draw_bar(px, 1, maxl, maxl + 4);
			draw_bar(px, 21, 0, levelr<<2);
			draw_bar(px, 21, maxr, maxr + 4);
		}
		scope_view_unlock_redraw(&view);
		dosleep(SCOPE_SLEEP);
	}
}


static void stop_levelmeter(void);

static gboolean
close_levelmeter_window(GtkWidget * UNUSED (widget), GdkEvent * UNUSED (event), gpointer UNUSED (data))
{
	stop_levelmeter();
	return TRUE;
}


static void init_levelmeter_tables(void)
{
	int i;

	/* Green to yellow to red in groups of three columns plus a gap. */
	for (i = 0; i < LM_WIDTH; i += 4) {
		guint32 col;
		if (i < 128)
			col = scope_rgb(i << 1, 255, 0);
		else
			col = scope_rgb(255, 255 - ((i - 128) << 1), 0);
		bar_colors[i] = col;
		bar_colors[i + 1] = col;
		bar_colors[i + 2] = col;
		bar_colors[i + 3] = scope_rgb(SCOPE_BG_RED, SCOPE_BG_GREEN, SCOPE_BG_BLUE);
	}

	// Create sin/cos tables

	for (i = 0; i < 256; i++) {
                scX[i] = (char) (sin(((2*M_PI)/255)*i)*128);
                scY[i] = (char) (-cos(((2*M_PI)/255)*i)*128);
        }
}


static void stop_levelmeter(void)
{
	if (running) {
		running = 0;
		pthread_join(levelmeter_thread, NULL);
	}
	scope_view_hide(&view);
}


static void *
run_levelmeter(void * UNUSED (data))
{
	scope_thread_nice();
	the_levelmeter();
	return NULL;
}



static void start_levelmeter(void)
{
	if (running) {
			printf("levelmeter already running\n");
			return;
	}

	scope_view_show(&view, G_CALLBACK(close_levelmeter_window));
	running = 1;
	if (pthread_create(&levelmeter_thread, NULL, run_levelmeter, NULL) != 0)
		running = 0;
}

static int
init_levelmeter(void * UNUSED (arg))
{
	init_levelmeter_tables();
	if (prefs_get_bool(ap_prefs, "levelmeter", "active", 0))
		start_levelmeter();
	return 1;
}


static void shutdown_levelmeter(void)
{
	prefs_set_bool(ap_prefs, "levelmeter", "active", levelmeter_running());

	stop_levelmeter();
	scope_view_destroy(&view);
}


static int levelmeter_running()
{
	return running;
}

scope_plugin levelmeter_plugin = {
	SCOPE_PLUGIN_VERSION,
	"Levelmeter",
	"Andy Lo A Foe",
	NULL,
	init_levelmeter,
	start_levelmeter,
	levelmeter_running,
	stop_levelmeter,
	shutdown_levelmeter,
	levelmeter_set_data,
	NULL
};


scope_plugin *scope_plugin_info(void)
{
	return &levelmeter_plugin;
}

