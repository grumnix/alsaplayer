/*  scope_view.h - GTK3/Cairo output window shared by the scope plugins
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
 */

/*
 * GTK3 does not allow drawing from threads other than the main loop, and
 * the GDK 2 indexed-image / pixmap APIs are gone.  A scope_view owns a
 * fixed-size xRGB pixel buffer: the scope's render thread writes into it
 * between scope_view_lock() and scope_view_unlock_redraw(), which then
 * schedules a redraw on the main loop.  The GTK "draw" handler paints the
 * buffer with Cairo.
 *
 * scope_view_show/hide/destroy must be called from the GTK main thread,
 * and scope_view_destroy only after the render thread has been joined.
 * It cancels any pending redraw so no callback outlives the plugin
 * (scope plugins are dlclose()d after shutdown).
 */

#ifndef SCOPE_VIEW_H
#define SCOPE_VIEW_H

#include <gtk/gtk.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include "scope_plugin.h"

typedef struct {
	const char *title;
	int width;
	int height;
	pthread_mutex_t lock;	/* guards pixels and redraw_id */
	guint32 *pixels;	/* width * height, CAIRO_FORMAT_RGB24 */
	guint redraw_id;
	GtkWidget *window;	/* main thread only */
	GtkWidget *area;	/* main thread only */
	gboolean have_pos;
	gint x, y;
} scope_view;

#define SCOPE_VIEW_INIT(title, w, h) \
	{ (title), (w), (h), PTHREAD_MUTEX_INITIALIZER, NULL, 0, NULL, NULL, FALSE, 0, 0 }

/* Pack r, g, b (0..255) the way CAIRO_FORMAT_RGB24 expects it. */
static inline guint32 scope_rgb(guint r, guint g, guint b)
{
	return ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
}

static gboolean scope_view_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	scope_view *v = (scope_view *)data;
	cairo_surface_t *surface;

	(void)widget;

	cairo_set_source_rgb(cr, SCOPE_BG_RED / 255.0,
			SCOPE_BG_GREEN / 255.0, SCOPE_BG_BLUE / 255.0);
	cairo_paint(cr);

	pthread_mutex_lock(&v->lock);
	if (v->pixels) {
		surface = cairo_image_surface_create_for_data(
				(unsigned char *)v->pixels, CAIRO_FORMAT_RGB24,
				v->width, v->height, v->width * 4);
		cairo_set_source_surface(cr, surface, 0, 0);
		cairo_paint(cr);
		cairo_surface_destroy(surface);
	}
	pthread_mutex_unlock(&v->lock);

	return TRUE;
}

static gboolean scope_view_redraw_cb(gpointer data)
{
	scope_view *v = (scope_view *)data;

	pthread_mutex_lock(&v->lock);
	v->redraw_id = 0;
	pthread_mutex_unlock(&v->lock);

	if (v->area)
		gtk_widget_queue_draw(v->area);

	return G_SOURCE_REMOVE;
}

/* Render thread: get the pixel buffer (may be NULL) and hold the lock. */
static inline guint32 *scope_view_lock(scope_view *v)
{
	pthread_mutex_lock(&v->lock);
	return v->pixels;
}

/* Render thread: release the buffer and schedule a repaint. */
static inline void scope_view_unlock_redraw(scope_view *v)
{
	if (v->pixels && !v->redraw_id)
		v->redraw_id = g_idle_add(scope_view_redraw_cb, v);
	pthread_mutex_unlock(&v->lock);
}

/* Render thread: copy an 8-bit indexed image through a 256-entry palette. */
static inline void scope_view_put_indexed(scope_view *v, const guchar *src,
		int src_stride, const guint32 palette[256])
{
	guint32 *dst = scope_view_lock(v);
	int x, y;

	if (dst) {
		for (y = 0; y < v->height; y++) {
			const guchar *s = src + y * src_stride;
			for (x = 0; x < v->width; x++)
				*dst++ = palette[s[x]];
		}
	}
	scope_view_unlock_redraw(v);
}

/* Main thread: create the window on first use, then show it. */
static inline void scope_view_show(scope_view *v, GCallback on_delete)
{
	if (!v->window) {
		pthread_mutex_lock(&v->lock);
		if (!v->pixels)
			v->pixels = g_new0(guint32, v->width * v->height);
		pthread_mutex_unlock(&v->lock);

		v->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(v->window), v->title);
		gtk_window_set_resizable(GTK_WINDOW(v->window), FALSE);
		g_signal_connect(v->window, "delete-event", on_delete, NULL);

		v->area = gtk_drawing_area_new();
		gtk_widget_set_size_request(v->area, v->width, v->height);
		g_signal_connect(v->area, "draw", G_CALLBACK(scope_view_draw_cb), v);
		gtk_container_add(GTK_CONTAINER(v->window), v->area);
		gtk_widget_show(v->area);
	}
	if (v->have_pos)
		gtk_window_move(GTK_WINDOW(v->window), v->x, v->y);
	gtk_widget_show(v->window);
}

/* Main thread: hide the window, remembering where it was. */
static inline void scope_view_hide(scope_view *v)
{
	if (v->window && gtk_widget_get_visible(v->window)) {
		gtk_window_get_position(GTK_WINDOW(v->window), &v->x, &v->y);
		v->have_pos = TRUE;
		gtk_widget_hide(v->window);
	}
}

/* Main thread, render thread joined: free everything. */
static inline void scope_view_destroy(scope_view *v)
{
	pthread_mutex_lock(&v->lock);
	if (v->redraw_id) {
		g_source_remove(v->redraw_id);
		v->redraw_id = 0;
	}
	g_free(v->pixels);
	v->pixels = NULL;
	pthread_mutex_unlock(&v->lock);

	if (v->window) {
		gtk_widget_destroy(v->window);
		v->window = NULL;
		v->area = NULL;
	}
}

/* Render thread: lower our priority; failure is harmless. */
static inline void scope_thread_nice(void)
{
	errno = 0;
	if (nice(SCOPE_NICE) == -1 && errno != 0) {
		/* keep running at normal priority */
	}
}

#endif /* SCOPE_VIEW_H */
