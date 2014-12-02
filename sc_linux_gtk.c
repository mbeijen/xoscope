/* -*- mode: C++; indent-tabs-mode: nil; fill-column: 100; c-basic-offset: 4; -*-
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
 * Copyright (C) 2014 Gerhard Schiller <gerhard.schiller@gmail.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the ALSA soundcard GUI.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include "xoscope.h"
#include "display.h"
#include "xoscope_gtk.h"

/* dialog to set the mV / steps for ALSA-Input */

/* from xoscope.h
   typedef struct Signal {
   ...
   int volts; * millivolts per 320 sample values *
   ...
   }
*/

extern double alsa_volts;

void alsa_gtk_option_dialog(void)
{
    GtkEntry *val;
    GtkEntry *peak;

    char valStr[8];

    val  = (GtkEntry *) LU("alsa_entry_mv");
    peak = (GtkEntry *) LU("alsa_radiobutton_peak");

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(peak))==TRUE) {
        snprintf(valStr, 8, "%7.2lf", alsa_volts / 3.2);
    } else {
        snprintf(valStr, 8, "%7.2lf", alsa_volts / (pow(2.0, 0.5) * 3.2));
    }

    gtk_entry_set_text(val, valStr);

    gtk_widget_show (alsa_options_dialog);
}

void on_alsa_buttonOk_clicked(void)
{
    GtkEntry *val;
    GtkEntry *peak;

    const char  *valStr;
    double      valNew = 0.0;

    val  = (GtkEntry *) LU("alsa_entry_mv");
    peak = (GtkEntry *) LU("alsa_radiobutton_peak");

    valStr = gtk_entry_get_text(val);
    if (sscanf(valStr, "%lf", &valNew) == 1) {
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(peak))==TRUE) {
            alsa_volts = valNew * 3.2;
        } else {
            alsa_volts = valNew * pow(2.0, 0.5) *3.2;
        }
        gtk_widget_hide (alsa_options_dialog);
        clear();
        /*              fprintf(stderr, "on_alsa_buttonOk_clicked %7.2f\n", alsa_volts);*/
    } else {
        gtk_widget_hide (alsa_options_dialog);
        /*              fprintf(stderr, "on_alsa_buttonOk_clicked NAN\n");*/
    }
}

void on_alsa_buttonCancel_clicked(void)
{
    gtk_widget_hide (alsa_options_dialog);
}
