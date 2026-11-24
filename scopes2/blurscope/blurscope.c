/*  blurscope.c
 *  Copyright (C) 2002 Andy Lo A Foe <andy@loafoe.com>
 *  Ported from XMMS:
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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

#include "scope_plugin.h"
#include "utilities.h"
#include "prefs.h"
#include "ap_unused.h"
#include "../scope_view.h"

static pthread_t bscope_thread;
static pthread_mutex_t edit_mutex = PTHREAD_MUTEX_INITIALIZER;
static gint running = 0;
static gint16 audio_data[2][256];

#define WIDTH 256
#define HEIGHT 128
#define min(x,y) ((x)<(y)?(x):(y))
#define BPL	((WIDTH + 2))

static guchar rgb_buf[(WIDTH + 2) * (HEIGHT + 2)];
static guint32 palette[256];
static scope_view view = SCOPE_VIEW_INIT("Blurscope", WIDTH, HEIGHT);

static int bscope_running(void);

static inline void draw_pixel_8(guchar *buffer,gint x, gint y, guchar c)
{
	buffer[((y + 1) * BPL) + (x + 1)] = c;
}


#ifndef I386_ASSEM
static void bscope_blur_8(guchar *ptr,gint UNUSED (w), gint h, gint bpl)
{
	register guint i,sum;
	register guchar *iptr;

	iptr = ptr + bpl + 1;
	i = bpl * h;
	while(i--)
	{
		sum = (iptr[-bpl] + iptr[-1] + iptr[1] + iptr[bpl]) >> 2;
		if(sum > 2)
			sum -= 2;
		*(iptr++) = sum;
	}


}
#else
extern void bscope_blur_8(guchar *ptr,gint w, gint h, gint bpl);
#endif

static void
generate_cmap(void)
{
	guint32 i,red,blue,green;

	red = (guint32)(0xFF3F7F / 0x10000);
	green = (guint32)((0xFF3F7F % 0x10000)/0x100);
	blue = (guint32)(0xFF3F7F % 0x100);
	for(i = 255; i > 0; i--)
	{
		palette[i] = scope_rgb(i*red/256, i*green/256, i*blue/256);
	}
	palette[0]=0;
}

static void stop_bscope(void)
{
	if (running) {
		running = 0;
		pthread_join(bscope_thread, NULL);
	}
	scope_view_hide(&view);
}


static gboolean close_bscope_window(GtkWidget * UNUSED (widget), GdkEvent * UNUSED (event), gpointer UNUSED (data))
{
	stop_bscope();
	return TRUE;
}


static void bscope_init(void)
{
	generate_cmap();
	memset(rgb_buf,0,(WIDTH + 2) * (HEIGHT + 2));
}

static void shutdown_bscope(void)
{
	prefs_set_bool(ap_prefs, "blurscope", "active", bscope_running());

	stop_bscope();
	scope_view_destroy(&view);
}

static inline void draw_vert_line(guchar *buffer, gint x, gint y_1, gint y_2)
{
	int y;
	if(y_1 < y_2)
	{
		for(y = y_1; y <= y_2; y++)
			draw_pixel_8(buffer,x,y,0xFF);
	}
	else if(y_2 < y_1)
	{
		for(y = y_2; y <= y_1; y++)
			draw_pixel_8(buffer,x,y,0xFF);
	}
	else
		draw_pixel_8(buffer,x,y_1,0xFF);
}


static void bscope_set_data(void *audio_buffer, int size)
{
#ifndef FREQ_BLUR
	short i;
#endif
	short *sound = (short *)audio_buffer;
#ifdef FREQ_BLUR
	static gint i,y, prev_y;
#endif
	if (pthread_mutex_trylock(&edit_mutex) != 0) {
			return;
	}
	if (running && sound && size >= 1024) {
     	for(i=0;i<256;i++) {
           	audio_data[0][i] = *(sound++);
            audio_data[1][i] = *(sound++);
     	}
	}
#ifdef FREQ_BLUR
		bscope_blur_8(rgb_buf, WIDTH, HEIGHT, BPL);

		prev_y = y = (HEIGHT / 2) + (audio_data[0][0] >> 9) + (audio_data[1][0] >> 9) / 2;
		if (prev_y < 0)
			y = prev_y = 0;
		if (y >= HEIGHT)
			y = prev_y = HEIGHT - 1;

		for(i = 0; i < WIDTH; i++)
		{
			y = (HEIGHT / 2) + (audio_data[0][i >> 1] >> 9) +
												 (audio_data[1][i >> 1] >> 9) / 2; /* Take half of other */
			if(y < 0)
				y = 0;
			if(y >= HEIGHT)
				y = HEIGHT - 1;
			draw_vert_line(rgb_buf,i,prev_y,y);
			prev_y = y;
		}
#endif
	pthread_mutex_unlock(&edit_mutex);
}


static void the_bscope(void)
{
	while (running) {
		gint i,y, prev_y;
#ifndef FREQ_BLUR
		pthread_mutex_lock(&edit_mutex);

		bscope_blur_8(rgb_buf, WIDTH, HEIGHT, BPL);

		prev_y = y = (HEIGHT / 2) + (audio_data[0][0] >> 9) + (audio_data[1][0] >> 9) / 2;
		if (prev_y < 0)
			y = prev_y = 0;
		if (y >= HEIGHT)
			y = prev_y = HEIGHT - 1;

		for(i = 0; i < WIDTH; i++)
		{
			y = (HEIGHT / 2) + (audio_data[0][i >> 1] >> 9) +
												 (audio_data[1][i >> 1] >> 9) / 2; /* Take half of other */
			if(y < 0)
				y = 0;
			if(y >= HEIGHT)
				y = HEIGHT - 1;
			draw_vert_line(rgb_buf,i,prev_y,y);
			prev_y = y;
		}
		pthread_mutex_unlock(&edit_mutex);
#endif

		/* edit_mutex keeps set_data from touching rgb_buf mid-copy */
		pthread_mutex_lock(&edit_mutex);
		scope_view_put_indexed(&view, rgb_buf + BPL + 1, BPL, palette);
		pthread_mutex_unlock(&edit_mutex);

		dosleep(SCOPE_SLEEP);
	}
}


static void *run_bscope(void * UNUSED (data))
{
	scope_thread_nice();
	the_bscope();
	return NULL;
}



static void start_bscope(void)
{
	if (running) {
			printf("blurscope already running\n");
			return;
	}

	scope_view_show(&view, G_CALLBACK(close_bscope_window));
	running = 1;
	if (pthread_create(&bscope_thread, NULL, run_bscope, NULL) != 0)
		running = 0;
}


static int init_bscope(void * UNUSED (arg))
{
	bscope_init();

	if (prefs_get_bool(ap_prefs, "blurscope", "active", 0)) {
		start_bscope();
	}
	return 1;
}


static int bscope_running()
{
	return running;
}


static scope_plugin bscope_plugin;

scope_plugin *scope_plugin_info(void)
{
	memset(&bscope_plugin, 0, sizeof(scope_plugin));
	bscope_plugin.version = SCOPE_PLUGIN_VERSION;
	bscope_plugin.name = "Blurscope";
	bscope_plugin.author = "Andy Lo A Foe";
	bscope_plugin.init = init_bscope;
	bscope_plugin.start = start_bscope;
	bscope_plugin.running = bscope_running;
	bscope_plugin.stop = stop_bscope;
	bscope_plugin.shutdown = shutdown_bscope;
	bscope_plugin.set_data = bscope_set_data;
	return &bscope_plugin;
}

