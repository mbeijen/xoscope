/*
 * @(#)$Id: com_gtk.c,v 1.4 2003/06/17 22:52:32 baccala Exp $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file holds GTK code that is common to xoscope and xy.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "oscope.h"
#include "display.h"
#include "com_gtk.h"

GdkPixmap *pixmap = NULL;
int fixing_widgets = 0;
char *colors[] = {		/* X colors similar to 16 console colors */
  "black",			/* 0 */
  "blue",
  "green",			/* 2 */
  "cyan",
  "red",			/* 4 */
  "magenta",
  "orange",			/* 6 */
  "gray66",
  "gray33",			/* 8 */
  "blue4",
  "green4",			/* 10 */
  "cyan4",
  "red4",			/* 12 */
  "magenta4",
  "yellow",			/* 14 */
  "white"
};

/* emulate several libsx function in GTK */
int
OpenDisplay(int argc, char *argv[])
{
  gtk_init(&argc, &argv);
  return(argc);
}

void
ClearDrawArea()
{
  if (pixmap)
    gdk_draw_rectangle(pixmap,
		       drawing_area->style->black_gc,
		       TRUE,
		       0, 0,
		       drawing_area->allocation.width,
		       drawing_area->allocation.height);

  gdk_draw_rectangle(drawing_area->window,
		     drawing_area->style->black_gc,
		     TRUE,
		     0, 0,
		     drawing_area->allocation.width,
		     drawing_area->allocation.height);
}

void
SetColor(int c)
{
  gdk_gc_set_foreground(gc, &gdkcolor[c]);
}

void
DrawPixel(int x, int y)
{
  gdk_draw_point(drawing_area->window, gc, x, y);
}

void
DrawLine(int x1, int y1, int x2, int y2)
{
  gdk_draw_line(drawing_area->window, gc, x1, y1, x2, y2);
}

void
SyncDisplay()
{

#if 0
#if USE_PIXMAP

  /* This code is called at the end of show_data(), after the pixmap
   * has been changed around, and generates an extra expose event for
   * the entire drawing_area, which pushes us into expose_event()
   * below, and that actually copies the pixmap to the screen.
   */

  update_rect.x  = update_rect.y = 0;
  update_rect.width = drawing_area->allocation.width;
  update_rect.height = drawing_area->allocation.height;
  gtk_widget_draw(drawing_area, &update_rect);

#endif
#endif

}

void
AddTimeOut(unsigned long interval, int (*func)(), void *data) {
  static int done = 0;

  if (done) return;
  gtk_idle_add((GtkFunction)func, (gpointer)data);
  done = 1;
}

/* XXX this is really a hack, because these functions aren't defined
 * for 'xy'
 */

void clear_text_memory() __attribute__ ((weak));
void clear_text_memory() {}
void draw_text() __attribute__ ((weak));
void draw_text() {}

/* set up some common event handlers */
gint
expose_event(GtkWidget *widget, GdkEventExpose *event)
{
#if USE_PIXMAP
  gdk_draw_pixmap(widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                  pixmap,
                  event->area.x, event->area.y,
                  event->area.x, event->area.y,
                  event->area.width, event->area.height);
  clear_text_memory();
  draw_text(1);
#else
  /* This will trigger a redisplay of the entire screen, including whatever
   * happened to be exposed in the expose event
   */
  clear();
#endif
  return FALSE;
}

void
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_main_quit ();
}

gint
key_press_event(GtkWidget *widget, GdkEventKey *event)
{
  handle_key(event->string[0]);
  return TRUE;
}

/* simple evnet callback that emulates the user hitting the given key */
void
hit_key(GtkWidget *w, gpointer data)
{
  if (fixing_widgets) return;
  handle_key(((char *)data)[0]);
}
