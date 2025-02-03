/*============================================================================
Copyright (c) 2024-2025 Raspberry Pi Holdings Ltd.
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
============================================================================*/

#include <locale.h>
#include <glib/gi18n.h>

#include "lxutils.h"

#include "connect.h"

/*----------------------------------------------------------------------------*/
/* Typedefs and macros                                                        */
/*----------------------------------------------------------------------------*/

#define DEBUG_ON
#ifdef DEBUG_ON
#define DEBUG(fmt,args...) if(getenv("DEBUG_CONN"))g_message("connect: " fmt,##args)
#define DEBUG_VAR(fmt,var,args...) if(getenv("DEBUG_CONN")){gchar*vp=g_variant_print(var,TRUE);g_message("connect: " fmt,##args,vp);g_free(vp);}
#else
#define DEBUG(fmt,args...)
#define DEBUG_VAR(fmt,var,args...)
#endif

/*----------------------------------------------------------------------------*/
/* Global data                                                                */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

static void check_installed (ConnectPlugin *c);
static void cb_name_owned (GDBusConnection *, const gchar *, const gchar *, ConnectPlugin *);
static void cb_proxy_result (GObject *, GAsyncResult *, ConnectPlugin *);
static void cb_name_unowned (GDBusConnection *, const gchar *, ConnectPlugin *);
static void cb_status (GDBusProxy *, char *, char *, GVariant *, ConnectPlugin *);
static void handle_sign_in (GtkWidget *, ConnectPlugin *);
static void handle_sign_out (GtkWidget *, ConnectPlugin *);
static void handle_toggle_vnc (GtkWidget *, ConnectPlugin *);
static void handle_toggle_ssh (GtkWidget *, ConnectPlugin *);
static void cb_result (GObject *, GAsyncResult *, ConnectPlugin *);
static void handle_status_req (GtkWidget *, ConnectPlugin *c);
static void cb_status_req (GObject *, GAsyncResult *, ConnectPlugin *);
static void toggle_enabled (GtkWidget *, ConnectPlugin *);
static void show_help (GtkWidget *, ConnectPlugin *);
static void show_menu (ConnectPlugin *);
static void update_icon (ConnectPlugin *);
static void connect_button_press_event (GtkButton *, ConnectPlugin *);

/*----------------------------------------------------------------------------*/
/* Function definitions                                                       */
/*----------------------------------------------------------------------------*/

/* Helpers */

static void check_installed (ConnectPlugin *c)
{
    c->installed = FALSE;
    if (!system ("dpkg -l rpi-connect | tail -n 1 | cut -d ' ' -f 1 | grep -q i")) c->installed = TRUE;
    if (!system ("dpkg -l rpi-connect-lite | tail -n 1 | cut -d ' ' -f 1 | grep -q i")) c->installed = TRUE;
    DEBUG ("Installed state = %d\n", c->installed);
}

/* Bus watcher callbacks */

static void cb_name_owned (GDBusConnection *conn, const gchar *name, const gchar *, ConnectPlugin *c)
{
    DEBUG ("Name %s owned on DBus", name);
    check_installed (c);

    // create the proxy
    g_dbus_proxy_new (conn, G_DBUS_PROXY_FLAGS_NONE, NULL, name, "/com/raspberrypi/Connect", "com.raspberrypi.Connect", NULL, (GAsyncReadyCallback) cb_proxy_result, c);
}

static void cb_proxy_result (GObject *, GAsyncResult *res, ConnectPlugin *c)
{
    GError *error = NULL;
    c->proxy = g_dbus_proxy_new_finish (res, &error);

    if (error)
    {
        DEBUG ("Getting proxy - error %s", error->message);
        g_error_free (error);
    }
    else
    {
        DEBUG ("Getting proxy - success");

        // register for proxy status callbacks
        g_signal_connect (c->proxy, "g-signal", G_CALLBACK (cb_status), c);
        handle_status_req (NULL, c);
    }
}

static void cb_name_unowned (GDBusConnection *, const gchar *name, ConnectPlugin *c)
{
    DEBUG ("Name %s unowned on DBus", name);

    if (c->proxy) g_object_unref (c->proxy);
    c->proxy = NULL;
    c->enabled = FALSE;
    check_installed (c);
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
    DEBUG ("Calling SignIn");
    g_dbus_proxy_call (c->proxy, "com.raspberrypi.Connect.SignIn", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, (GAsyncReadyCallback) cb_result, c);
}

static void handle_sign_out (GtkWidget *, ConnectPlugin *c)
{
    DEBUG ("Calling SignOut");
    g_dbus_proxy_call (c->proxy, "com.raspberrypi.Connect.SignOut", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, (GAsyncReadyCallback) cb_result, c);
}

static void handle_toggle_vnc (GtkWidget *, ConnectPlugin *c)
{
    if (c->vnc_on)
    {
        DEBUG ("Calling VncOff");
        g_dbus_proxy_call (c->proxy, "com.raspberrypi.Connect.VncOff", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, (GAsyncReadyCallback) cb_result, c);
    }
    else
    {
        DEBUG ("Calling VncOn");
        g_dbus_proxy_call (c->proxy, "com.raspberrypi.Connect.VncOn", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, (GAsyncReadyCallback) cb_result, c);
    }
}

static void handle_toggle_ssh (GtkWidget *, ConnectPlugin *c)
{
    if (c->ssh_on)
    {
        DEBUG ("Calling ShellOff");
        g_dbus_proxy_call (c->proxy, "com.raspberrypi.Connect.ShellOff", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, (GAsyncReadyCallback) cb_result, c);
    }
    else
    {
        DEBUG ("Calling ShellOn");
        g_dbus_proxy_call (c->proxy, "com.raspberrypi.Connect.ShellOn", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, (GAsyncReadyCallback) cb_result, c);
    }
}

static void cb_result (GObject *source, GAsyncResult *res, ConnectPlugin *)
{
    GError *error = NULL;
    GVariant *var = g_dbus_proxy_call_finish (G_DBUS_PROXY (source), res, &error);

    if (error)
    {
        DEBUG ("Result - error %s", error->message);
        g_error_free (error);
    }
    else
    {
        DEBUG ("Result - success");
    }
    if (var) g_variant_unref (var);
}

static void handle_status_req (GtkWidget *, ConnectPlugin *c)
{
    DEBUG ("Calling Status");
    g_dbus_proxy_call (c->proxy, "com.raspberrypi.Connect.Status", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, (GAsyncReadyCallback) cb_status_req, c);
}

static void cb_status_req (GObject *source, GAsyncResult *res, ConnectPlugin *c)
{
    GError *error = NULL;
    GVariant *var = g_dbus_proxy_call_finish (G_DBUS_PROXY (source), res, &error);
    
    // update the enabled flag here in case it has changed externally
    if (!system ("systemctl --user -q status rpi-connect.service | grep -q -w active")) c->enabled = TRUE;
    else c->enabled = FALSE;

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

    // auto sign in
    if (c->enabling)
    {
        if (c->enabled && !c->signed_in) handle_sign_in (NULL, c);
        c->enabling = FALSE;
    }
}


/* GUI... */

/* Functions to manage main menu */

static void toggle_enabled (GtkWidget *, ConnectPlugin *c)
{
    if (c->enabled)
    {
        system ("rpi-connect off");
        c->enabled = FALSE;
    }
    else
    {
        system ("rpi-connect on");
        c->enabled = TRUE;
        c->enabling = TRUE;
    }
    update_icon (c);
}

static void show_help (GtkWidget *, ConnectPlugin *)
{
    system ("x-www-browser https://www.raspberrypi.com/documentation/services/connect.html &");
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
        item = gtk_menu_item_new_with_label (_("Turn On Raspberry Pi Connect"));
        g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (toggle_enabled), c);
        gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);
    }
    else
    {
        item = gtk_menu_item_new_with_label (_("Turn Off Raspberry Pi Connect"));
        g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (toggle_enabled), c);
        gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);

        item = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);

        if (!c->signed_in)
        {
            item = gtk_menu_item_new_with_label (_("Sign In..."));
            g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (handle_sign_in), c);
            gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);
        }
        else
        {
            if (c->vnc_avail)
            {
                item = gtk_check_menu_item_new_with_label (_("Allow Screen Sharing"));
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), c->vnc_on);
                g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (handle_toggle_vnc), c);
                gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);
            }

            item = gtk_check_menu_item_new_with_label (_("Allow Remote Shell Access"));
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), c->ssh_on);
            g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (handle_toggle_ssh), c);
            gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);

            item = gtk_separator_menu_item_new ();
            gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);

            item = gtk_menu_item_new_with_label (_("Sign Out"));
            g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (handle_sign_out), c);
            gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);
        }
    }

    item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);

    item = gtk_menu_item_new_with_label (_("Raspberry Pi Connect Help..."));
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (show_help), c);
    gtk_menu_shell_append (GTK_MENU_SHELL (c->menu), item);

    gtk_widget_show_all (c->menu);
}

static void update_icon (ConnectPlugin *c)
{
    // hide icon if not installed ?
    if (!c->installed)
    {
        gtk_widget_hide (c->plugin);
        gtk_widget_set_sensitive (c->plugin, FALSE);
    }
    else
    {
        if (!c->enabled)
        {
            set_taskbar_icon (c->tray_icon, "rpc-disabled", c->icon_size);
            gtk_widget_set_tooltip_text (c->tray_icon, _("Disabled - Raspberry Pi Connect"));
        }
        else if (!c->signed_in)
        {
            set_taskbar_icon (c->tray_icon, "rpc-disabled", c->icon_size);
            gtk_widget_set_tooltip_text (c->tray_icon, _("Sign-in required - Raspberry Pi Connect"));
        }
        else
        {
            if (c->vnc_sess_count + c->ssh_sess_count > 0)
            {
                if (c->vnc_sess_count == 0)
                    gtk_widget_set_tooltip_text (c->tray_icon, _("Your device is being accessed via remote shell - Raspberry Pi Connect"));
                else if (c->ssh_sess_count == 0)
                    gtk_widget_set_tooltip_text (c->tray_icon, _("Your screen is being shared - Raspberry Pi Connect"));
                else
                    gtk_widget_set_tooltip_text (c->tray_icon, _("Your device is being accessed - Raspberry Pi Connect"));

                set_taskbar_icon (c->tray_icon, "rpc-active", c->icon_size);
            }
            else
            {
                set_taskbar_icon (c->tray_icon, "rpc-enabled", c->icon_size);
                gtk_widget_set_tooltip_text (c->tray_icon, _("Signed in - Raspberry Pi Connect"));
            }
        }
        gtk_widget_show_all (c->plugin);
        gtk_widget_set_sensitive (c->plugin, TRUE);
    }
}

/*----------------------------------------------------------------------------*/
/* wf-panel plugin functions                                                  */
/*----------------------------------------------------------------------------*/

/* Handler for button click */
static void connect_button_press_event (GtkButton *, ConnectPlugin *c)
{
    CHECK_LONGPRESS
    show_menu (c);
    show_menu_with_kbd (c->plugin, c->menu);
}

/* Handler for system config changed message from panel */
void connect_update_display (ConnectPlugin *c)
{
    update_icon (c);
}

void connect_init (ConnectPlugin *c)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    /* Allocate icon as a child of top level */
    c->tray_icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (c->plugin), c->tray_icon);

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (c->plugin), GTK_RELIEF_NONE);
    g_signal_connect (c->plugin, "clicked", G_CALLBACK (connect_button_press_event), c);

    /* Set up variables */
    c->menu = NULL;

    check_installed (c);

    if (!system ("systemctl --user -q status rpi-connect.service | grep -q -w active")) c->enabled = TRUE;
    else c->enabled = FALSE;
    c->enabling = FALSE;

    /* Set up callbacks to see if Connect is on DBus */
    c->watch = g_bus_watch_name (G_BUS_TYPE_SESSION, "com.raspberrypi.Connect", 0,
        (GBusNameAppearedCallback) cb_name_owned, (GBusNameVanishedCallback) cb_name_unowned, c, NULL);

    /* Show the widget and return */
    gtk_widget_show_all (c->plugin);
    update_icon (c);
}

void connect_destructor (ConnectPlugin *c)
{
    g_bus_unwatch_name (c->watch);

    g_free (c);
}


/* End of file */
/*----------------------------------------------------------------------------*/
