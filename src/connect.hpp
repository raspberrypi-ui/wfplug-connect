#ifndef WIDGETS_CONNECT_HPP
#define WIDGETS_CONNECT_HPP

#include <widget.hpp>
#include <gtkmm/button.h>

extern "C" {
#include "connect.h"
}

class WayfireConnect : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;
    bool wizard;

    /* plugin */
    ConnectPlugin *c;

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfireConnect ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
};

#endif /* end of include guard: WIDGETS_CONNECT_HPP */
