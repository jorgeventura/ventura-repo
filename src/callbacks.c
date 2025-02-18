/*
 * callbacks.c
 *
 * All things related to the main window.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "details.h"
#include "snmpstuff.h"
#include "mibtree.h"
#include "configuration.h"
#include "os.h"

GtkWidget *rootitem;
extern int delete;

extern int context_id;
extern int tview;

extern struct menu_items bookmark_items[128];
extern int dont_select_damnit;
extern int dont_update_entry;
static struct tree *mibtree;

GtkWidget *desc_win = NULL;
GtkWidget *subtree;

const gchar *readcomm,
      *writecomm,
      *loid,
      *instance,
      *value;

/* Display something on the status bar. */

void disperror(const char *message) {

 static int count = 0;
 GtkWidget *statusbar;

 statusbar = get_main_widgets()->statusbar;
 if (count)
   gtk_statusbar_pop(GTK_STATUSBAR(statusbar),context_id); 
 gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,message); 
} 
  
/* Load a mib into memory. */

void load_mib(GtkWidget *fs) {

  const gchar *name;

  gtk_widget_hide(fs);
  name = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
  mibtree = read_mib(name);
  retree(mibtree);
}

void fsdestroy(GtkWidget *widget,gpointer data) {

  *((GtkWidget **)data) = NULL;

}

/* File selection dialog for loading a mib */

void select_file(void) {

  static GtkWidget *fileselection = NULL;

  if (!fileselection) {
    fileselection = gtk_file_selection_new("Select MIB");
    gtk_signal_connect(GTK_OBJECT(fileselection),"destroy",
                       (GtkSignalFunc) fsdestroy,&fileselection);
    gtk_signal_connect_object(
                       GTK_OBJECT(GTK_FILE_SELECTION(fileselection)->ok_button),
                       "clicked",(GtkSignalFunc) load_mib,
                       GTK_OBJECT(fileselection));
    gtk_signal_connect_object(
                   GTK_OBJECT(GTK_FILE_SELECTION(fileselection)->cancel_button),
                   "clicked",(GtkSignalFunc) gtk_widget_hide,
                    GTK_OBJECT(fileselection));
    gtk_widget_show(fileselection);
  } else {
    if (GTK_WIDGET_MAPPED(fileselection))
      gdk_window_raise(fileselection->window);
    else
      gtk_widget_show(fileselection);
  }
}

void on_get_butt_clicked(GtkButton *button,gpointer user_data) {

  char temp[256];
  config_t *config;
  main_widgets_t *mwid;
  GtkTextBuffer *textBuf;

  mwid = get_main_widgets();
  disperror("");
  textBuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(user_data));
  gtk_text_buffer_set_text(textBuf,"",0);
  handle_events();
  config = get_config();
  strcpy(temp,config->oid);
  if (instance && strlen(instance)) {
    if (*(config->oid+strlen(config->oid)-1) != '.') 
      strcat(temp,".");
    strcat(temp,instance);
  }
  snmpget(get_hostname(),temp,config->readcomm,user_data);
}

void on_walk_butt_clicked(GtkButton *button,gpointer user_data) {

  char temp[256];
  config_t *config;
  main_widgets_t *mwid;
  GtkTextBuffer *textBuf;

  mwid = get_main_widgets();
  disperror("");
  textBuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(user_data));
  gtk_text_buffer_set_text(textBuf,"",0);
  handle_events();
  config = get_config();
  strcpy(temp,config->oid);
#if 0  /* Ignore instance on walk.... keed code just in case... */
  if (instance && strlen(instance)) {
    if (*(config->oid+strlen(config->oid)-1) != '.')
      strcat(temp,".");
    strcat(temp,instance);
  }
#endif
  snmpwalk(get_hostname(),config->oid,config->readcomm,user_data);
}

#if 0
void on_graph_butt_clicked(GtkButton *button,gpointer user_data) {

  char line[1024],string[1024];
  FILE *fp;
  int i;
  char *hostname;
  GtkTextBuffer *textBuf;
  GtkTextIter textIter;

  disperror("");
  hostname = gtk_entry_get_text(GTK_ENTRY(host));
  textBuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(user_data));
  gtk_text_buffer_set_text(textBuf,"",0);
  handle_events();
  sprintf(string,"snmptable %s %s %s %s","-Os",hostname,config.readcomm,config.oid);
  fp = popen(string,"r");
  while(fgets(line,1023,fp)) {
    gtk_text_buffer_get_end_iter(textBuf,&textIter);
    gtk_text_buffer_insert(textBuf,&textIter,line,-1);

  }

}
#endif

void on_set_butt_clicked(GtkButton *button,gpointer user_data) {

  char temp[256];
  config_t *config;
  main_widgets_t *mwid;
  const char *val;
  GtkTextBuffer *textBuf;

  mwid = get_main_widgets();
  disperror("");
  textBuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(user_data));
  gtk_text_buffer_set_text(textBuf,"",0);
  handle_events();
  config = get_config();
  strcpy(temp,config->oid);
  if (instance && strlen(instance)) {
    if (*(config->oid+strlen(config->oid)-1) != '.')
      strcat(temp,".");
    strcat(temp,instance);
  }
  /* See if there is an enum for this */
  if ((val = find_val_from_enum(get_value())) == NULL) {
    val = get_value();
  }
  snmpset(get_hostname(),temp,config->writecomm,val,user_data);
}

void init_stuff(void) {


  mibtree = init_mbrowse_snmp();
  populate_mib_tree(mibtree,NULL);
}

void on_readcomm_entry_changed(GtkEditable *editable,gpointer user_data) {

  get_config()->readcomm = gtk_entry_get_text(GTK_ENTRY(user_data));
}

void on_writecomm_entry_changed(GtkEditable *editable,gpointer user_data) {

  get_config()->writecomm = gtk_entry_get_text(GTK_ENTRY(user_data));
}

void set_oid_entry(const char *text) {

  if (delete == 1) {
    struct menu_items *p = bookmark_items;
    FILE *fp;
    char fn[1024];

#ifdef WIN32
    strcpy(fn,".\\");
#else
    strcpy(fn,getenv("HOME"));
#endif
    strcat(fn, PATH_SEPARATOR ".mbrowse_bookmarks");
    fp = fopen(fn,"w");
    for(;p->item;p++) {
      if (p->data == NULL)
        continue;
      if (strcmp(text,p->data) == 0) {
        struct menu_items *l;

        gtk_widget_destroy(p->mi);
        for(l=p;l->item;l++) {
          l->item = (l+1)->item;
          l->data = (l+1)->data;
          l->func = (l+1)->func;
          l->mi = (l+1)->mi;
        }
        p--;
      } else {
        snprintf(fn,1023,"%s;%s\n",p->item,(char *)p->data);
        fputs(fn,fp);
      }
    }
    fclose(fp);
    delete = 0;
    disperror("");
    return;
  }
  gtk_entry_set_text(GTK_ENTRY(get_main_widgets()->oid_entry),text);
}

void oid_entry_changed(GtkEditable *editable,gpointer user_data) {

  size_t size;
  oid test[MAX_OID_LEN];
  const char *oid;

  oid = get_config()->oid = gtk_entry_get_text(GTK_ENTRY(user_data));
  if (dont_select_damnit) {
    return;
  } 
  size = MAX_OID_LEN;
  if (snmp_parse_oid(oid,test,&size)) {
    dont_update_entry = 1;
    select_oid(test,size);
    dont_update_entry = 0;
  }
}


void on_instance_entry_changed(GtkEditable *editable,gpointer user_data) {

  instance = gtk_entry_get_text(GTK_ENTRY(user_data));
}


void on_value_entry_changed(GtkEditable *editable,gpointer user_data) {

  if (gtk_combo_box_get_active(GTK_COMBO_BOX(editable)) != -1) {
    set_as("Automatic");
  }
}

void need_to_exit(void) {

  save_config();
  gtk_main_quit();
}

gboolean window_state_changed(GtkWidget *widget, GdkEventWindowState *event, gpointer user_data) {
  if (event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED)
    get_config()->window_maximized = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED);
  return FALSE;
}
