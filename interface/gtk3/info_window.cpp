/*  info_window.cpp
 *  Copyright (C) 2007 Madej
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

#include "info_window.h"
#include "prefs.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) gettext(String)
#define N_(String) noop_gettext(String)
#else
#define _(String) (String)
#define N_(String) String
#endif

static GtkWidget*
create_info_window()
{
	GtkWidget *block;
	GtkWidget *main_box;
	GtkWidget *speed_label;
	GtkWidget *balance_label;
	GtkWidget *title_label;
	GtkWidget *format_label;
	GtkWidget *volume_label;
	GtkWidget *position_label;

	block = gtk_frame_new(NULL);
	main_box = gtk_layout_new(NULL, NULL);

	g_object_set_data(G_OBJECT(block), "layout", main_box);
	gtk_container_add(GTK_CONTAINER(block), main_box);

	speed_label = gtk_label_new(NULL);
	g_object_set_data(G_OBJECT(block), "speed_label", speed_label);
	gtk_layout_put(GTK_LAYOUT(main_box), speed_label, 2, 0);

	balance_label = gtk_label_new(NULL);
	g_object_set_data(G_OBJECT(block), "balance_label", balance_label);
	gtk_layout_put(GTK_LAYOUT(main_box), balance_label, 0, 0);

	title_label = gtk_label_new(NULL);
	g_object_set_data(G_OBJECT(block), "title_label", title_label);
	gtk_layout_put(GTK_LAYOUT(main_box), title_label, 0, 0);

	format_label = gtk_label_new(NULL);
	g_object_set_data(G_OBJECT(block), "format_label", format_label);
	gtk_layout_put(GTK_LAYOUT(main_box), format_label, 0, 0);

	volume_label = gtk_label_new(NULL);
	g_object_set_data(G_OBJECT(block), "volume_label", volume_label);
	gtk_layout_put(GTK_LAYOUT(main_box), volume_label, 0, 0);

	position_label = gtk_label_new(NULL);
	g_object_set_data(G_OBJECT(block), "position_label", position_label);
	gtk_layout_put(GTK_LAYOUT(main_box), position_label, 0, 25);

	return block;
}

InfoWindow::InfoWindow()
{
	window = create_info_window();
	volume = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "volume_label"));
	balance = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "balance_label"));
	position = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "position_label"));
	title = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "title_label"));
	format = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "format_label"));
	speed = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "speed_label"));
	layout = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "layout"));

	leftwidth = 0;
	rightwidth = 0;
	labelheight = 0;

	const char *val = prefs_get_string(ap_prefs, "gtk2_interface", "background_colour", "#000000");
	this->set_background_color(val);
	val = prefs_get_string(ap_prefs, "gtk2_interface", "font_colour", "#ffffff");
	this->set_font_color(val);
	val = prefs_get_string(ap_prefs, "gtk2_interface", "fonts", "");
	this->set_fonts(val);
}

InfoWindow::~InfoWindow()
{
	gtk_widget_destroy(this->window);
}

void InfoWindow::set_volume(const gchar *text)
{
	gtk_label_set_text (GTK_LABEL(this->volume), text);
}

void InfoWindow::set_balance(const gchar *text)
{
	gtk_label_set_text (GTK_LABEL(this->balance), text);
}
void InfoWindow::set_positions()
{
	gint x, y, width, height;
	GtkAllocation vol_a, pos_a, speed_a, bal_a, layout_a;

	gtk_widget_get_allocation(this->volume, &vol_a);
	gtk_widget_get_allocation(this->position, &pos_a);
	gtk_widget_get_allocation(this->speed, &speed_a);
	gtk_widget_get_allocation(this->balance, &bal_a);
	gtk_widget_get_allocation(this->layout, &layout_a);

	if ((this->labelheight < 2) || (this->leftwidth < 2) || (this->rightwidth < 2) || (this->labelheight != vol_a.height)) {
		this->leftwidth = (speed_a.width > bal_a.width) ? speed_a.width : bal_a.width;
		this->rightwidth = (vol_a.width > pos_a.width) ? vol_a.width : pos_a.width;
		this->labelheight = vol_a.height;

		gtk_widget_set_size_request(this->window, -1, this->labelheight * 2 + this->labelheight / 3);
	}

	width = layout_a.width;
	height = layout_a.height;

	//speed has fixed position
	// 2 px padding
	x = 2;
	y = height - this->labelheight;
	gtk_layout_move(GTK_LAYOUT(this->layout), this->balance, x, y);

	x = this->leftwidth + this->labelheight;
	y = 0;
	gtk_widget_set_size_request (this->title, width - x - this->rightwidth - this->labelheight, -1);
	gtk_layout_move(GTK_LAYOUT(this->layout), this->title, x, y);

	x = this->leftwidth + this->labelheight;
	y = height - this->labelheight;
	gtk_widget_set_size_request (this->format, width - x - this->rightwidth - this->labelheight, -1);
	gtk_layout_move(GTK_LAYOUT(this->layout), this->format, x, y);

	x = width - vol_a.width -2;
	y = 0;
	gtk_layout_move(GTK_LAYOUT(this->layout), this->volume, x, y);

	x = width - pos_a.width - 2;
	y = height - this->labelheight;
	gtk_layout_move(GTK_LAYOUT(this->layout), this->position, x, y);

}

void InfoWindow::set_position(const gchar *text)
{
	gtk_label_set_text (GTK_LABEL(this->position), text);
}

void InfoWindow::set_title(const gchar *text)
{
	gtk_label_set_text (GTK_LABEL(this->title), text);
}

void InfoWindow::set_format(const gchar *text)
{
	gtk_label_set_text (GTK_LABEL(this->format), text);
}

void InfoWindow::set_speed(const gchar *text)
{
	gtk_label_set_text (GTK_LABEL(this->speed), text);
}

void InfoWindow::set_background_color(const gchar* str)
{
	GdkColor color;

	if (!gdk_color_parse(str, &color))
		return;

	do { GdkRGBA __rgba; __rgba.red = color.red/65535.0; __rgba.green = color.green/65535.0; __rgba.blue = color.blue/65535.0; __rgba.alpha = 1.0; gtk_widget_override_background_color(this->layout, GTK_STATE_FLAG_NORMAL, &__rgba); } while(0);
}

void InfoWindow::set_font_color(const gchar* str)
{
	GdkColor color;

	if (!gdk_color_parse(str, &color))
		return;

	do { GdkRGBA __rgba; __rgba.red = color.red/65535.0; __rgba.green = color.green/65535.0; __rgba.blue = color.blue/65535.0; __rgba.alpha = 1.0; gtk_widget_override_color(this->volume, GTK_STATE_FLAG_NORMAL, &__rgba); } while(0);
	do { GdkRGBA __rgba; __rgba.red = color.red/65535.0; __rgba.green = color.green/65535.0; __rgba.blue = color.blue/65535.0; __rgba.alpha = 1.0; gtk_widget_override_color(this->position, GTK_STATE_FLAG_NORMAL, &__rgba); } while(0);
	do { GdkRGBA __rgba; __rgba.red = color.red/65535.0; __rgba.green = color.green/65535.0; __rgba.blue = color.blue/65535.0; __rgba.alpha = 1.0; gtk_widget_override_color(this->title, GTK_STATE_FLAG_NORMAL, &__rgba); } while(0);
	do { GdkRGBA __rgba; __rgba.red = color.red/65535.0; __rgba.green = color.green/65535.0; __rgba.blue = color.blue/65535.0; __rgba.alpha = 1.0; gtk_widget_override_color(this->format, GTK_STATE_FLAG_NORMAL, &__rgba); } while(0);
	do { GdkRGBA __rgba; __rgba.red = color.red/65535.0; __rgba.green = color.green/65535.0; __rgba.blue = color.blue/65535.0; __rgba.alpha = 1.0; gtk_widget_override_color(this->speed, GTK_STATE_FLAG_NORMAL, &__rgba); } while(0);
	do { GdkRGBA __rgba; __rgba.red = color.red/65535.0; __rgba.green = color.green/65535.0; __rgba.blue = color.blue/65535.0; __rgba.alpha = 1.0; gtk_widget_override_color(this->balance, GTK_STATE_FLAG_NORMAL, &__rgba); } while(0);
}

void InfoWindow::set_fonts(const gchar* str)
{
	PangoFontDescription *fonts;

	fonts = pango_font_description_from_string(str);

	gtk_widget_override_font(this->volume, fonts);
	gtk_widget_override_font(this->position, fonts);
	gtk_widget_override_font(this->title, fonts);
	gtk_widget_override_font(this->format, fonts);
	gtk_widget_override_font(this->speed, fonts);
	gtk_widget_override_font(this->balance, fonts);

	pango_font_description_free(fonts);
}
