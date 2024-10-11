#include "lxutils.h"

/* Plug-in global data */

typedef struct {
    GtkWidget *plugin;              /* Back pointer to the widget */
    GtkWidget *tray_icon;           /* Displayed image */
    int icon_size;                  /* Variables used under wf-panel */
    gboolean bottom;

    GtkGesture *gesture;
    GtkWidget *menu; 

    guint watch;
    GDBusProxy *proxy;

    gboolean installed;
    gboolean enabled;
    gboolean signed_in;
    gboolean vnc_avail;
    gboolean vnc_on;
    gboolean ssh_on;
    int vnc_sess_count;
    int ssh_sess_count;
} ConnectPlugin;

extern void connect_init (ConnectPlugin *c);
extern void connect_update_display (ConnectPlugin *c);
extern void connect_destructor (ConnectPlugin *c);
