/*
 * interface.c
 *
 * Routines pertaining to the main window.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "configuration.h"

main_widgets_t main_widgets;
static GtkWidget *main_window;
static GtkWidget *readcomm_entry;
static GtkWidget *writecomm_entry;
static GtkWidget *instance_entry;
static GtkWidget *value_entry;
static GtkWidget *as_entry;
static GtkWidget *set_butt;
static GtkWidget *out_box;

int tview = 0;

struct menu_titles {
  char *title;
  struct menu_items *items;
};

struct as_choice {
  const char *title;
  const char snmp_type;
};

static const struct as_choice as_choices[] = {
  { "Automatic",	'=' },
  { "Integer",		'i' },
  { "Unsigned",		'u' },
  { "String",		's' },
  { "Hex string",	'x' },
  { "Decimal string",	'd' },
  { "Nullobj",		'n' },
  { "OID",		'o' },
  { "Timeticks",	't' },
  { "IP address",	'a' },
  { "Bits",		'b' },
  { NULL,		 0  },
};

int context_id;

main_widgets_t *get_main_widgets(void) {

  return(&main_widgets);
}

void set_hostname(const char *host) {

  gtk_entry_set_text(GTK_ENTRY(main_widgets.host),host);
}

const char *get_hostname(void) {

  return(gtk_entry_get_text(GTK_ENTRY(main_widgets.host)));
}

void set_oid(const char *oid) {

  gtk_entry_set_text(GTK_ENTRY(main_widgets.oid_entry),oid);
}

void set_readcomm(const char *comm) {

  gtk_entry_set_text(GTK_ENTRY(readcomm_entry),comm);
}

const char *get_readcomm(void) {

  return(gtk_entry_get_text(GTK_ENTRY(readcomm_entry)));
}

void set_writecomm(const char *comm) {

  gtk_entry_set_text(GTK_ENTRY(writecomm_entry),comm);
}

const char *get_writecomm(void) {

  return(gtk_entry_get_text(GTK_ENTRY(writecomm_entry)));
}

void set_instance(const char *inst) {

  gtk_entry_set_text(GTK_ENTRY(instance_entry),inst);
}

const char *get_instance(void) {

   return(gtk_entry_get_text(GTK_ENTRY(instance_entry)));
}

void set_value(const char *val) {

  gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(value_entry))), val);
}

const char *get_value(void) {

   return(gtk_combo_box_get_active_text(GTK_COMBO_BOX(value_entry)));
}

char get_as(void) {
  const gchar *selection;
  const struct as_choice *ch;

  selection = gtk_combo_box_get_active_text(GTK_COMBO_BOX(as_entry));

  for (ch = as_choices; ch->title; ch++) {
    if (strcmp(ch->title, selection) == 0)
      return ch->snmp_type;
  }
  return '='; /* the default */
}

void set_as(const char *title) {
  int idx = 0;
  const struct as_choice *ch;

  for (ch = as_choices; ch->title; ch++) {
    if (strcmp(ch->title, title) == 0) {
      gtk_combo_box_set_active(GTK_COMBO_BOX(as_entry), idx);
    }
    idx++;
  }
}

void get_window_size(int *width, int *height) {
  gtk_window_get_size(GTK_WINDOW(main_window), width, height);
}

void set_window_size(int width, int height) {
  gtk_window_set_default_size(GTK_WINDOW(main_window), width, height);
}

void set_window_maximized() {
  gtk_window_maximize(GTK_WINDOW(main_window));
}

void view_suffix(void) {
  tview =(1-tview);
  if (tview) {
    snmp_set_suffix_only(0);
  } else {
    snmp_set_full_objid(1);
  }
}

/* Disable/Enable the "Set" button, and the "as" and "Value"
 * entries depending of the access of the variable
 */

void variable_writable(gboolean p,gboolean a) {

  static gboolean cp = TRUE;
  static gboolean ca = TRUE;

  if (ca != a && p == TRUE) {
    gtk_widget_set_sensitive(as_entry,a);
    ca=a;
  } else if (p == FALSE && cp != p) {
    gtk_widget_set_sensitive(as_entry,FALSE);
    ca = FALSE;
  }
  if (cp == p)
    return;
  gtk_widget_set_sensitive(value_entry,p);
  gtk_widget_set_sensitive(set_butt,p);
  if (p == FALSE)
    populate_value_list(NULL);
  cp=p;
}

/* Populate the "Value" combo with enums found
 * in the MIB
 */

void populate_value_list(GList *vals) {

  static int count = 0;
  const GList *cur;

  /* delete the previous content */
  for (; count; count--) {
    gtk_combo_box_remove_text(GTK_COMBO_BOX(value_entry), 0);
  }

  /* fill up with the new content */
  count = 0;
  for (cur = vals; cur; cur = g_list_next(cur)) {
    count++;
    gtk_combo_box_append_text(GTK_COMBO_BOX(value_entry), cur->data);
  }

  /* set selected value */
  if (count) {
    set_value(vals->data);
    /* reset to automatic mode for enums */
    set_as("Automatic");
  } else {
    set_value("");
  }

  g_list_free(vals);
}

static GtkWidget *create_as_entry(void) {
  GtkWidget *entry;
  const struct as_choice *ch;

  entry = gtk_combo_box_new_text();

  for (ch = as_choices; ch->title; ch++) {
    gtk_combo_box_append_text(GTK_COMBO_BOX(entry), ch->title);
  }

  gtk_combo_box_set_active(GTK_COMBO_BOX(entry), 0);
  gtk_widget_show(entry);

  return entry;
}

GtkWidget *create_main_window(void) {

  GtkWidget *label;
  GtkWidget *main_vbox;
  GtkWidget *menu_bar;
  GtkWidget *file_drop;
  GtkWidget *file_menu;
  GtkWidget *file_open;
  GtkWidget *commhost_frame;
  GtkWidget *commhost_hbox;
  GtkWidget *mibbutton_hbox;
  GtkWidget *oid_hbox;
  GtkWidget *oid_frame;
  GtkWidget *action_frame;
  GtkWidget *action_buttonbox;
  GtkWidget *get_butt;
  GtkWidget *walk_butt;
/*  GtkWidget *graph_butt;*/
  GtkWidget *exit_button;
  GtkWidget *instval_frame;
  GtkWidget *instval_hbox;
  GtkWidget *fun_pane;
  GtkWidget *pane1_hbox;
  GtkWidget *paned2_vbox;
  GtkWidget *out_scrolledwin;

  struct menu_items *temp_item;
  int i;

  struct menu_items file_items[] = {
    { "Open MIB",select_file,NULL,NULL },
    { "Exit",need_to_exit,NULL,NULL },
    { NULL,NULL,NULL,NULL }
  };

  struct menu_items option_items[] = {
/*    { "Graph",create_graph_win,NULL,NULL },*/
/*    { "-",NULL,NULL,NULL },*/
    { "+View Module::Suffix",view_suffix,NULL,NULL },
    { NULL,NULL,NULL,NULL }
  };

  extern struct menu_items bookmark_items[];

  struct menu_titles menu_stuff[] = {
    { "File",file_items },
    { "Options",option_items },
    { "Bookmarks",bookmark_items },
    { NULL,NULL }
  };



/* Create the main window */

  main_widgets.accels = gtk_accel_group_new();
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data(GTK_OBJECT(main_window),"main_window",main_window);
  gtk_window_set_title(GTK_WINDOW(main_window),"MIB Browser");

  gtk_signal_connect(GTK_OBJECT(main_window),"delete-event",
      GTK_SIGNAL_FUNC(need_to_exit),NULL);
  gtk_signal_connect(GTK_OBJECT(main_window),"window-state-event",
      GTK_SIGNAL_FUNC(window_state_changed),NULL);

  main_vbox = gtk_vbox_new(FALSE,0);
  gtk_widget_show(main_vbox);
  gtk_container_add(GTK_CONTAINER(main_window),main_vbox);

  menu_bar = gtk_menu_bar_new();
  gtk_widget_show(menu_bar);
  gtk_box_pack_start(GTK_BOX(main_vbox),menu_bar,FALSE,FALSE,0);
/* Not sure about this one. */
/*  gtk_menu_bar_set_shadow_type(GTK_MENU_BAR(menu_bar),GTK_SHADOW_ETCHED_IN); */

  /* Build Menus */

  for(i=0;menu_stuff[i].title;i++) {
    file_drop = gtk_menu_item_new_with_label(menu_stuff[i].title);
    gtk_widget_show(file_drop);
    gtk_container_add(GTK_CONTAINER(menu_bar),file_drop);
    file_menu = gtk_menu_new();
    main_widgets.bookmark_menu = file_menu;
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_drop),file_menu);
    temp_item = menu_stuff[i].items;
    for(;temp_item->item;temp_item++) {
      switch(*temp_item->item) { 
        case '-':
          file_open = gtk_menu_item_new();
          break;
        case '+':
          file_open = gtk_check_menu_item_new_with_label(++temp_item->item);
          break;
        default:
          file_open = gtk_menu_item_new_with_label(temp_item->item);
      }
      temp_item->mi = file_open;
      gtk_menu_append(GTK_MENU(file_menu),file_open);
      if(temp_item->func) 
        gtk_signal_connect_object(GTK_OBJECT(file_open),"activate",
                             GTK_SIGNAL_FUNC(temp_item->func),temp_item->data);
      gtk_widget_show(file_open);
    }
  }

  commhost_frame = gtk_frame_new(NULL);
  gtk_widget_show(commhost_frame);
  gtk_box_pack_start(GTK_BOX(main_vbox),commhost_frame,FALSE,FALSE,0);
  gtk_container_set_border_width(GTK_CONTAINER(commhost_frame),3);

  commhost_hbox = gtk_hbox_new(FALSE,0);
  gtk_widget_show(commhost_hbox);
  gtk_container_add(GTK_CONTAINER(commhost_frame),commhost_hbox);
  gtk_container_set_border_width(GTK_CONTAINER(commhost_hbox),5);

  label = gtk_label_new("Host Name");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(commhost_hbox),label,FALSE,FALSE,5);

  main_widgets.host = gtk_entry_new();
  gtk_widget_show(main_widgets.host);
  gtk_box_pack_start(GTK_BOX(commhost_hbox),main_widgets.host,TRUE,TRUE,0);
  gtk_widget_set_usize(main_widgets.host,100,-2);

  label = gtk_label_new("Read Community");

  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(commhost_hbox),label,FALSE,FALSE,5);

  readcomm_entry = gtk_entry_new();
  gtk_widget_show(readcomm_entry);
  gtk_box_pack_start(GTK_BOX(commhost_hbox),readcomm_entry,FALSE,FALSE,5);
  gtk_widget_set_usize(readcomm_entry,100,-2);

  label = gtk_label_new("Write Community");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(commhost_hbox),label,FALSE,FALSE,5);

/*  writecomm_entry = gtk_entry_new_with_max_length(10);*/
  writecomm_entry = gtk_entry_new();
  gtk_widget_show(writecomm_entry);
  gtk_box_pack_start(GTK_BOX(commhost_hbox),writecomm_entry,FALSE,FALSE,5);
  gtk_widget_set_usize(writecomm_entry,100,-2);

  oid_frame = gtk_frame_new(NULL);
  gtk_widget_show(oid_frame);
  gtk_box_pack_start(GTK_BOX(main_vbox),oid_frame,FALSE,FALSE,0);
  gtk_container_set_border_width(GTK_CONTAINER(oid_frame),3);

  oid_hbox = gtk_hbox_new(FALSE,0);
  gtk_widget_show(oid_hbox);
  gtk_container_add(GTK_CONTAINER(oid_frame),oid_hbox);

  label = gtk_label_new("Object Identifier");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(oid_hbox),label,FALSE,FALSE,5);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_RIGHT);

  main_widgets.oid_entry = gtk_entry_new();
  gtk_widget_show(main_widgets.oid_entry);
  gtk_box_pack_start(GTK_BOX(oid_hbox),main_widgets.oid_entry,TRUE,TRUE,10);

  fun_pane = gtk_vpaned_new();
  gtk_widget_show(fun_pane);
  gtk_box_pack_start(GTK_BOX(main_vbox),fun_pane,TRUE,TRUE,0);

  pane1_hbox = gtk_hbox_new(FALSE,0);
  gtk_widget_show(pane1_hbox);
  gtk_paned_pack1(GTK_PANED(fun_pane),pane1_hbox,FALSE,FALSE);

  mibbutton_hbox = gtk_hbox_new(FALSE,0);
  gtk_widget_show(mibbutton_hbox);
  gtk_box_pack_start(GTK_BOX(pane1_hbox),mibbutton_hbox,FALSE,FALSE,0);

  main_widgets.notebook = gtk_notebook_new();
  gtk_widget_show(main_widgets.notebook);
  gtk_container_add(GTK_CONTAINER(pane1_hbox),main_widgets.notebook);

/*  gtk_box_pack_start(GTK_BOX(pane1_hbox),main_widgets.notebook,TRUE,TRUE,0);*/

  action_frame = gtk_frame_new(NULL);
  gtk_widget_show(action_frame);
  gtk_box_pack_start(GTK_BOX(mibbutton_hbox),action_frame,FALSE,TRUE,0);
  gtk_container_set_border_width(GTK_CONTAINER(action_frame),3);

  action_buttonbox = gtk_vbutton_box_new();
  gtk_widget_show(action_buttonbox);
  gtk_container_add(GTK_CONTAINER(action_frame),action_buttonbox);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(action_buttonbox),
      GTK_BUTTONBOX_START);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(action_buttonbox),0);

  get_butt = gtk_button_new_with_label("Get");
  gtk_widget_show(get_butt);
  gtk_container_add(GTK_CONTAINER(action_buttonbox),get_butt);
  GTK_WIDGET_SET_FLAGS(get_butt,GTK_CAN_DEFAULT);

  gtk_widget_add_accelerator(get_butt,"clicked",main_widgets.accels,GDK_g,
      GDK_CONTROL_MASK,GTK_ACCEL_LOCKED);
  gtk_widget_add_accelerator(get_butt, "clicked", main_widgets.accels,
      GDK_g, 0, GTK_ACCEL_LOCKED);

  walk_butt = gtk_button_new_with_label("Walk");
  gtk_widget_show(walk_butt);
  gtk_container_add(GTK_CONTAINER(action_buttonbox),walk_butt);
  GTK_WIDGET_SET_FLAGS(walk_butt,GTK_CAN_DEFAULT);

  gtk_widget_add_accelerator(walk_butt,"clicked",main_widgets.accels,GDK_w,
      GDK_CONTROL_MASK,GTK_ACCEL_LOCKED);
  gtk_widget_add_accelerator(walk_butt, "clicked", main_widgets.accels,
      GDK_w, 0, GTK_ACCEL_LOCKED);

  set_butt = gtk_button_new_with_label("Set");
  gtk_widget_show(set_butt);
  gtk_container_add(GTK_CONTAINER(action_buttonbox),set_butt);
  GTK_WIDGET_SET_FLAGS(set_butt,GTK_CAN_DEFAULT);

  gtk_widget_add_accelerator(set_butt,"clicked",main_widgets.accels,GDK_s,
      GDK_CONTROL_MASK,GTK_ACCEL_LOCKED);
  gtk_widget_add_accelerator(set_butt, "clicked", main_widgets.accels,
      GDK_s, 0, GTK_ACCEL_LOCKED);

/*  graph_butt = gtk_button_new_with_label("Graph");
  gtk_widget_show(graph_butt);
  gtk_container_add(GTK_CONTAINER(action_buttonbox),graph_butt);
  GTK_WIDGET_SET_FLAGS(graph_butt,GTK_CAN_DEFAULT);*/

  exit_button = gtk_button_new_with_label("Exit");
  gtk_widget_show(exit_button);
  gtk_container_add(GTK_CONTAINER(action_buttonbox),exit_button);
  GTK_WIDGET_SET_FLAGS(exit_button,GTK_CAN_DEFAULT);

  gtk_widget_add_accelerator(exit_button,"clicked",main_widgets.accels,GDK_e,
      GDK_CONTROL_MASK,GTK_ACCEL_LOCKED);
  gtk_widget_add_accelerator(exit_button, "clicked", main_widgets.accels,
      GDK_e, 0, GTK_ACCEL_LOCKED);

  paned2_vbox = gtk_vbox_new(FALSE,0);
  gtk_widget_show(paned2_vbox);

  gtk_paned_pack2(GTK_PANED(fun_pane),paned2_vbox,FALSE,FALSE);

  instval_frame = gtk_frame_new(NULL);
  gtk_widget_show(instval_frame);
  gtk_box_pack_start(GTK_BOX(paned2_vbox),instval_frame,FALSE,FALSE,0);
  gtk_container_set_border_width(GTK_CONTAINER(instval_frame),3);

  instval_hbox = gtk_hbox_new(FALSE,0);
  gtk_widget_show(instval_hbox);
  gtk_container_add(GTK_CONTAINER(instval_frame),instval_hbox);

  label = gtk_label_new("Instance");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(instval_hbox),label,FALSE,FALSE,5);

  instance_entry = gtk_entry_new();
  gtk_widget_show(instance_entry);
  gtk_box_pack_start(GTK_BOX(instval_hbox),instance_entry,FALSE,FALSE,5);
  gtk_widget_set_usize(instance_entry,50,-2);

  label = gtk_label_new("Value");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(instval_hbox),label,FALSE,FALSE,5);

  value_entry = gtk_combo_box_entry_new_text();
  gtk_widget_show(value_entry);
  gtk_box_pack_start(GTK_BOX(instval_hbox),value_entry,TRUE,TRUE,5);

  label = gtk_label_new("as");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(instval_hbox),label,FALSE,FALSE,5);

  as_entry = create_as_entry();
  gtk_box_pack_start(GTK_BOX(instval_hbox),as_entry,FALSE,FALSE,5);

  out_scrolledwin = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_show(out_scrolledwin);
  gtk_box_pack_start(GTK_BOX(paned2_vbox),out_scrolledwin,TRUE,TRUE,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(out_scrolledwin),
      GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  
  out_box = gtk_text_view_new();
  gtk_widget_show(out_box);
  gtk_container_add(GTK_CONTAINER(out_scrolledwin),out_box);

  main_widgets.statusbar = gtk_statusbar_new();
  gtk_widget_show(main_widgets.statusbar);
  context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(main_widgets.statusbar),"");
  gtk_box_pack_start(GTK_BOX(main_vbox),main_widgets.statusbar,FALSE,FALSE,0);

  gtk_signal_connect_after(GTK_OBJECT(readcomm_entry),"changed",
      GTK_SIGNAL_FUNC(on_readcomm_entry_changed),readcomm_entry);
  gtk_signal_connect_after(GTK_OBJECT(writecomm_entry),"changed",
      GTK_SIGNAL_FUNC(on_writecomm_entry_changed),writecomm_entry);
  gtk_signal_connect_after(GTK_OBJECT(main_widgets.oid_entry),"changed",
      GTK_SIGNAL_FUNC(oid_entry_changed),main_widgets.oid_entry);
  gtk_signal_connect_after(GTK_OBJECT(get_butt),"clicked",
      GTK_SIGNAL_FUNC(on_get_butt_clicked),out_box);
  gtk_signal_connect_after(GTK_OBJECT(walk_butt),"clicked",
      GTK_SIGNAL_FUNC(on_walk_butt_clicked),out_box);
  /*  gtk_signal_connect_after(GTK_OBJECT(graph_butt),"clicked",
      GTK_SIGNAL_FUNC(on_graph_butt_clicked),
      out_box);*/
  gtk_signal_connect_after(GTK_OBJECT(set_butt),"clicked",
      GTK_SIGNAL_FUNC(on_set_butt_clicked),out_box);
  gtk_signal_connect_after(GTK_OBJECT(exit_button),"clicked",
      GTK_SIGNAL_FUNC(need_to_exit),NULL);
  gtk_signal_connect(GTK_OBJECT(instance_entry),"changed",
      GTK_SIGNAL_FUNC(on_instance_entry_changed),instance_entry);
  gtk_signal_connect(GTK_OBJECT(value_entry),"changed",
                     GTK_SIGNAL_FUNC(on_value_entry_changed),
                     value_entry);

/* accelerators don't work correctly with gtk2, comment them out */
/*  gtk_window_add_accel_group(GTK_WINDOW(main_window),main_widgets.accels);*/
  gtk_widget_realize(main_window);

  return main_window;
}

void set_out_box_wrap_mode(GtkWrapMode mode) {
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(out_box), mode);
}

void handle_events() {
  while (gtk_events_pending())
    if (gtk_main_iteration()) {
      exit(0);
    }
}
