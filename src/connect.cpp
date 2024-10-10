#include <glibmm.h>
#include "connect.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfireConnect; }
    void destroy (WayfireWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[1] = {
        {CONF_NONE, NULL, NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("Connect"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

void WayfireConnect::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") c->bottom = TRUE;
    else c->bottom = FALSE;
}

void WayfireConnect::icon_size_changed_cb (void)
{
    c->icon_size = icon_size;
    connect_update_display (c);
}

bool WayfireConnect::set_icon (void)
{
    connect_update_display (c);
    return false;
}

void WayfireConnect::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    c = g_new0 (ConnectPlugin, 1);
    c->plugin = (GtkWidget *)((*plugin).gobj());
    c->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireConnect::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    connect_init (c);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireConnect::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireConnect::bar_pos_changed_cb));
}

WayfireConnect::~WayfireConnect()
{
    icon_timer.disconnect ();
    connect_destructor (c);
}
