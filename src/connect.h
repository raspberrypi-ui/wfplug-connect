/*============================================================================
Copyright (c) 2021-2025 Raspberry Pi Holdings Ltd.
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

/*----------------------------------------------------------------------------*/
/* Typedefs and macros */
/*----------------------------------------------------------------------------*/

typedef struct
{
    int icon_size;                  /* Variables used under wf-panel */
    gboolean bottom;
    GtkWidget *plugin;              /* Back pointer to the widget */

    GtkWidget *tray_icon;           /* Displayed image */
    GtkWidget *menu; 

    guint watch;
    GDBusProxy *proxy;

    gboolean installed;
    gboolean enabled;
    gboolean enabling;
    gboolean signed_in;
    gboolean vnc_avail;
    gboolean vnc_on;
    gboolean ssh_on;
    int vnc_sess_count;
    int ssh_sess_count;
} ConnectPlugin;

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

extern void connect_init (ConnectPlugin *c);
extern void connect_update_display (ConnectPlugin *c);
extern gboolean connect_control_msg (ConnectPlugin *c, const char *cmd);
extern void connect_destructor (ConnectPlugin *c);

/* End of file */
/*----------------------------------------------------------------------------*/
