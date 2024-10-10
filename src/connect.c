/*
Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "connect.h"

#define DEBUG_ON
#ifdef DEBUG_ON
#define DEBUG(fmt,args...) if(getenv("DEBUG_CONN"))g_message("connect: " fmt,##args)
#define DEBUG_VAR(fmt,var,args...) if(getenv("DEBUG_CONN")){gchar*vp=g_variant_print(var,TRUE);g_message("connect: " fmt,##args,vp);g_free(vp);}
#else
#define DEBUG(fmt,args...)
#define DEBUG_VAR(fmt,var,args...)
#endif

/*---------------------------------------------------------------------------*/
/* Prototypes */
/*---------------------------------------------------------------------------*/

static void cb_name_owned (GDBusConnection *, const gchar *, const gchar *, ConnectPlugin *);
static void cb_name_unowned (GDBusConnection *, const gchar *, ConnectPlugin *);
static void cb_status (GDBusProxy *, char *, char *, GVariant *, ConnectPlugin *);
static void handle_sign_in (GtkWidget *, ConnectPlugin *);
static void cb_sign_in (GObject *, GAsyncResult *res, ConnectPlugin *);
static void handle_sign_out (GtkWidget *, ConnectPlugin *);
static void cb_sign_out (GObject *, GAsyncResult *, ConnectPlugin *);
static void handle_status_req (GtkWidget *, ConnectPlugin *c);
static void cb_status_req (GObject *, GAsyncResult *, ConnectPlugin *);
static void toggle_enabled (GtkWidget *, ConnectPlugin *);
static void show_menu (ConnectPlugin *);
static void update_icon (ConnectPlugin *);
static void connect_button_press_event (GtkButton *, ConnectPlugin *);
static void connect_gesture_pressed (GtkGestureLongPress *, gdouble, gdouble, ConnectPlugin *);
static void connect_gesture_end (GtkGestureLongPress *, GdkEventSequence *, ConnectPlugin *);

/*---------------------------------------------------------------------------*/
/* Function Definitions */
/*---------------------------------------------------------------------------*/

/* Bus watcher callbacks */

static void cb_name_owned (GDBusConnection *conn, const gchar *name, const gchar *, ConnectPlugin *c)
{
    GError *error = NULL;
    DEBUG ("Name %s owned on DBus", name);
    
    // create the proxy
    c->proxy = g_dbus_proxy_new_sync (conn, G_DBUS_PROXY_FLAGS_NONE, NULL, name, "/com/raspberrypi/Connect", "com.raspberrypi.Connect", NULL, &error);
    if (error)
    {
        DEBUG ("Error getting proxy - %s", error->message);
        g_error_free (error);
    }
    else
    {
        DEBUG ("No error");

        // handle proxy status callbacks
        g_signal_connect (c->proxy, "g-signal", G_CALLBACK (cb_status), c);
        handle_status_req (NULL, c);
    }
}

static void cb_name_unowned (GDBusConnection *, const gchar *name, ConnectPlugin *c)
{
    DEBUG ("Name %s unowned on DBus", name);
    g_object_unref (c->proxy);
    c->proxy = NULL;
    c->enabled = FALSE;
    update_icon (c);
}

/* Proxy callbacks */

static void cb_status (GDBusProxy *, char *, char *signal, GVariant *params, ConnectPlugin *c)
{
    if (strcmp (signal, "Status"))
    {
        DEBUG ("Unexpected signal");
    }
    else
    {
        DEBUG ("Status message received - %s", g_variant_print (params, TRUE));
        g_variant_get (params, "((bbbbii))", &c->signed_in, &c->vnc_avail, &c->vnc_on, &c->ssh_on, &c->vnc_sess_count, &c->ssh_sess_count);
        update_icon (c);
    }
}

/* Proxy methods */

static void handle_sign_in (GtkWidget *, ConnectPlugin *c)
{
    g_dbus_proxy_call (c->proxy, "com.raspberrypi.Connect.SignIn", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, (GAsyncReadyCallback) cb_sign_in, c);
}

static void cb_sign_in (GObject *source, GAsyncResult *res, ConnectPlugin *)
{
    GError *error = NULL;
    GVariant *var = g_dbus_proxy_call_finish (G_DBUS_PROXY (source), res, &error);
    
    if (error)
    {
        DEBUG ("Sign in - error %s", error->message);
        g_error_free (error);
    }
    else
    {
        DEBUG_VAR ("Sign in - result %s", var);
    }
    if (var) g_variant_unref (var);
}

static void handle_sign_out (GtkWidget *, ConnectPlugin *c)
{
    g_dbus_proxy_call (c->proxy, "com.raspberrypi.Connect.SignOut", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, (GAsyncReadyCallback) cb_sign_out, c);
}

static void cb_sign_out (GObject *source, GAsyncResult *res, ConnectPlugin *)
{
    GError *error = NULL;
    GVariant *var = g_dbus_proxy_call_finish (G_DBUS_PROXY (source), res, &error);
    
    if (error)
    {
        DEBUG ("Sign out - error %s", error->message);
        g_error_free (error);
    }
    else
    {
        DEBUG_VAR ("Sign out - result %s", var);
    }
    if (var) g_variant_unref (var);
}

static void handle_status_req (GtkWidget *, ConnectPlugin *c)
{
    g_dbus_proxy_call (c->proxy, "com.raspberrypi.Connect.Status", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, (GAsyncReadyCallback) cb_status_req, c);
}

static void cb_status_req (GObject *source, GAsyncResult *res, ConnectPlugin *c)
{
    GError *error = NULL;
    GVariant *var = g_dbus_proxy_call_finish (G_DBUS_PROXY (source), res, &error);
    
    if (error)
    {
        DEBUG ("Status - error %s", error->message);
        g_error_free (error);
    }
    else
    {
        DEBUG_VAR ("Status - result %s", var);
        g_variant_get (var, "((bbbbii))", &c->signed_in, &c->vnc_avail, &c->vnc_on, &c->ssh_on, &c->vnc_sess_count, &c->ssh_sess_count);
        update_icon (c);
    }
    if (var) g_variant_unref (var);
}


/* GUI... */

/* Functions to manage main menu */

static void toggle_enabled (GtkWidget *, ConnectPlugin *c)
{
    if (c->enabled)
    {
        system ("systemctl --global -q stop rpi-connect.service");
        system ("systemctl --global -q disable rpi-connect.service rpi-connect-wayvnc.service rpi-connect-wayvnc-watcher.path");
        c->enabled = FALSE;
    }
    else
    {
        system ("systemctl --global -q enable rpi-connect.service rpi-connect-wayvnc.service rpi-connect-wayvnc-watcher.path");
        system ("systemctl --global -q start rpi-connect.service");
        c->enabled = TRUE;
    }
    connect_update_display (c);
}

static void show_menu (ConnectPlugin *c)
{
    GtkWidget *item;
    GList *items;

    // if the menu is currently on screen, delete all the items and rebuild rather than creating a new one
    if (c->menu)
    {
        items = gtk_container_get_children (GTK_CONTAINER (c->menu));
        g_list_free_full (items, (GDestroyNotify) gtk_widget_destroy);
    }
    else c->menu = gtk_menu_new ();
    gtk_menu_set_reserve_toggle_size (GTK_MENU (c->menu), TRUE);

    if (!c->enabled) // is it running...
    {
        item = gtk_menu_item_new_with_label (_("Turn On Connect"));
        g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (toggle_enabled), c);
        gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);
    }
    else
    {
        item = gtk_menu_item_new_with_label (_("Turn Off Connect"));
        g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (toggle_enabled), c);
        gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);
        
        if (!c->signed_in)
        {
            item = gtk_menu_item_new_with_label (_("Sign In..."));
            g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (handle_sign_in), c);
            gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);
        }
        else
        {
            item = gtk_menu_item_new_with_label (_("Sign Out"));
            g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (handle_sign_out), c);
            gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);
            
            // some more stuff here...
        }
    }

    gtk_widget_show_all (c->menu);
}

static void update_icon (ConnectPlugin *c)
{
    // hide icon if not installed ?
    if (0)
    {
        gtk_widget_hide (c->plugin);
        gtk_widget_set_sensitive (c->plugin, FALSE);
    }
    else
    {
        if (!c->enabled || !c->signed_in) set_taskbar_icon (c->tray_icon, "rpc-disabled", c->icon_size);
        else
        {
            if (c->vnc_sess_count + c->ssh_sess_count > 0)
                set_taskbar_icon (c->tray_icon, "rpc-active", c->icon_size);
            else
                set_taskbar_icon (c->tray_icon, "rpc-enabled", c->icon_size);
        }
        gtk_widget_show_all (c->plugin);
        gtk_widget_set_sensitive (c->plugin, TRUE);
    }
    // do the tooltip in here too
}

/* Handler for menu button click */

static void connect_button_press_event (GtkButton *, ConnectPlugin *c)
{
    if (pressed != PRESS_LONG)
    {   
        show_menu (c);
        show_menu_with_kbd (c->plugin, c->menu);
    }
    pressed = PRESS_NONE;
}

static void connect_gesture_pressed (GtkGestureLongPress *, gdouble x, gdouble y, ConnectPlugin *)
{
    pressed = PRESS_LONG;
    press_x = x;
    press_y = y;
}

static void connect_gesture_end (GtkGestureLongPress *, GdkEventSequence *, ConnectPlugin *bt)
{
    if (pressed == PRESS_LONG) pass_right_click (bt->plugin, press_x, press_y);
}

/* Handler for system config changed message from panel */

void connect_update_display (ConnectPlugin *c)
{
    update_icon (c);
}

/* Plugin destructor */

void connect_destructor (gpointer user_data)
{
    ConnectPlugin *c = (ConnectPlugin *) user_data;

    g_bus_unwatch_name (c->watch);

    /* Deallocate memory */
    if (c->gesture) g_object_unref (c->gesture);
    g_free (c);
}

/* Plugin constructor */

void connect_init (ConnectPlugin *c)
{
    /* Allocate icon as a child of top level */
    c->tray_icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (c->plugin), c->tray_icon);
    set_taskbar_icon (c->tray_icon, "rpc-off", c->icon_size);
    gtk_widget_set_tooltip_text (c->tray_icon, _("Raspberry Pi Connect"));

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (c->plugin), GTK_RELIEF_NONE);
    g_signal_connect (c->plugin, "clicked", G_CALLBACK (connect_button_press_event), c);

    /* Set up long press */
    c->gesture = gtk_gesture_long_press_new (c->plugin);
    gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (c->gesture), TRUE);
    g_signal_connect (c->gesture, "pressed", G_CALLBACK (connect_gesture_pressed), c);
    g_signal_connect (c->gesture, "end", G_CALLBACK (connect_gesture_end), c);
    gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (c->gesture), GTK_PHASE_BUBBLE);

    /* Set up variables */
    c->menu = NULL;

    if (!system ("systemctl --user -q status rpi-connect.service | grep -q -w active")) c->enabled = TRUE;
    else c->enabled = FALSE;

    /* Set up callbacks to see if Connect is on DBus */
    c->watch = g_bus_watch_name (G_BUS_TYPE_SESSION, "com.raspberrypi.Connect", 0,
        (GBusNameAppearedCallback) cb_name_owned, (GBusNameVanishedCallback) cb_name_unowned, c, NULL);

    /* Show the widget and return */
    gtk_widget_show_all (c->plugin);
}

