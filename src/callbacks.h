#include <gtk/gtk.h>
#include <sys/types.h>
#ifndef WIN32
#include <netinet/in.h>
#endif

#ifdef HAVE_NETSNMP
#  include <net-snmp/net-snmp-config.h>
#  include <net-snmp/net-snmp-includes.h>
#  include <net-snmp/version.h>
#else
#  include <ucd-snmp/ucd-snmp-config.h>
#  include <ucd-snmp/ucd-snmp-includes.h>
#  include <ucd-snmp/parse.h>
#  include <ucd-snmp/system.h>
#endif

void select_file(void);
void on_get_butt_clicked(GtkButton *button,gpointer user_data);
void on_walk_butt_clicked(GtkButton *button,gpointer user_data);
void on_graph_butt_clicked(GtkButton *button,gpointer user_data);
void on_set_butt_clicked(GtkButton *button,gpointer user_data);
void on_desc_butt_clicked(GtkButton *button,gpointer user_data);
void on_readcomm_entry_changed(GtkEditable *editable,gpointer user_data);
void on_writecomm_entry_changed(GtkEditable *editable,gpointer user_data);
void oid_entry_changed(GtkEditable *editable,gpointer user_data);
void on_instance_entry_changed(GtkEditable *editable,gpointer user_data);
void on_value_entry_changed(GtkEditable *editable,gpointer user_data);
void init_stuff(void);
void disperror(const char *);
void set_oid_entry(const char *);
void need_to_exit(void);
gboolean window_state_changed(GtkWidget *widget, GdkEventWindowState *event, gpointer user_data);
