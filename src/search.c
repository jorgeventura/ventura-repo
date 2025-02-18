/*
 * search.c
 *
 * Create search tab,and accompanying functions.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "search.h"

extern struct tree *mibtree;
extern GtkWidget *oid_entry;

static search_t search;

static void look_for(char *needle,int where,struct tree *child) {

#  define DESCRIPTION   0
#  define MIB_NAME      1

  struct tree *next;
  struct tree *tmp;
  char *haystack;
  char *p;
  char *args[2];
  char what[40][7],real[1024];
  int i;

  for(next=child->child_list;next;next=next->next_peer) {
    haystack = NULL;
    switch(where) {
      case DESCRIPTION:
        if(next->description)
          haystack = strdup(next->description);
        break;
      case MIB_NAME:
        if(next->label)
          haystack = strdup(next->label);
        break;
      default:
	break;
    }
    if(haystack) {
      for(p=haystack;*p;p++)
        *p=TOUPPER(*p);
      if(strstr(haystack,needle)) {
        /* INsert stuff */
        args[0] = next->label;
        snprintf(what[0],6,".%lu",next->subid);
        for(i=1,tmp=next;tmp;tmp=tmp->parent,i++)
          snprintf(what[i],6,".%lu",tmp->subid);
        *what[i] = 0x00;
        *real = 0x00;
        while(--i)
          strcat(real,what[i]);
        args[1]=real;
        gtk_clist_append(GTK_CLIST(search.list),args);
      }
      free(haystack);
    }
    if(next->child_list)
      look_for(needle,where,next);
  }
}

static void mib_search(void) {

  const char *crit;
  char *needle,*tmp;
  int where;

  gtk_clist_clear(GTK_CLIST(search.list));
  crit = gtk_combo_box_get_active_text(GTK_COMBO_BOX(search.criteria));
  needle = strdup(gtk_entry_get_text(GTK_ENTRY(search.entry)));
  if(strcmp(crit,"Description")==0)
    where = DESCRIPTION;
  else
    where = MIB_NAME;
  for(tmp=needle;*tmp;tmp++)
    *tmp = TOUPPER(*tmp);
  look_for(needle,where,mibtree);
  free(needle);
}

static void clear_search(void) {

  gtk_clist_clear(GTK_CLIST(search.list));
  gtk_entry_set_text(GTK_ENTRY(search.entry),"");
}

static void search_row_selected(GtkCList *clist,gint row,gint column,
                                GdkEvent *event,gpointer user_data) {

  gchar *oid;

  if(gtk_clist_get_text(GTK_CLIST(search.list),row,1,&oid))
    gtk_entry_set_text(GTK_ENTRY(get_main_widgets()->oid_entry),oid);
}

void create_search_tab(void) {

  GtkWidget *search_tab_label;
  GtkWidget *search_frame;
  GtkWidget *vbox1;
  GtkWidget *hbox2;
  GtkWidget *label3;
  GtkWidget *combo1;
  GtkWidget *label4;
  GtkWidget *hbuttonbox1;
  GtkWidget *button2;
  GtkWidget *button3;
  GtkWidget *scrolledwindow1;
  GtkWidget *label5;
  GtkWidget *notebook;
  
  char *tmp,*columns[] = { "Mib Label","OID",NULL };
  int i=0;

  notebook = get_main_widgets()->notebook;
  search_frame = gtk_frame_new(NULL);
  gtk_widget_show(search_frame);
  gtk_container_set_border_width(GTK_CONTAINER(search_frame),5);
  gtk_frame_set_shadow_type(GTK_FRAME(search_frame),GTK_SHADOW_ETCHED_OUT);

  search_tab_label = gtk_label_new("Search");
  gtk_widget_show(search_tab_label);

  gtk_container_add(GTK_CONTAINER(notebook),search_frame);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),2),
      search_tab_label);

  vbox1 = gtk_vbox_new(FALSE,0);
  gtk_widget_show(vbox1);
  gtk_container_add(GTK_CONTAINER(search_frame),vbox1);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_widget_show(hbox2);
  gtk_box_pack_start(GTK_BOX(vbox1),hbox2,FALSE,TRUE,0);
  gtk_container_set_border_width(GTK_CONTAINER(hbox2),5);

  label3 = gtk_label_new("Where");
  gtk_widget_show(label3);
  gtk_box_pack_start(GTK_BOX(hbox2),label3,FALSE,FALSE,0);
  gtk_misc_set_padding(GTK_MISC(label3),5,0);

  combo1 = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo1), "Description");
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo1), "MIB Name");
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo1), 0);
  gtk_widget_show(combo1);
  gtk_box_pack_start(GTK_BOX(hbox2),combo1,TRUE,TRUE,0);
  search.criteria = combo1;

  label4 = gtk_label_new("Contains");
  gtk_widget_show(label4);
  gtk_box_pack_start(GTK_BOX(hbox2),label4,FALSE,FALSE,0);
  gtk_misc_set_padding(GTK_MISC(label4),5,0);

  search.entry = gtk_entry_new();
  gtk_widget_show(search.entry);
  gtk_box_pack_start(GTK_BOX(hbox2),search.entry,TRUE,TRUE,0);

  hbuttonbox1 = gtk_hbutton_box_new();
  gtk_widget_show(hbuttonbox1);
  gtk_box_pack_start(GTK_BOX(vbox1),hbuttonbox1,FALSE,TRUE,0);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox1),GTK_BUTTONBOX_SPREAD);

  button2 = gtk_button_new_with_label("Search");
  gtk_widget_show(button2);
  gtk_container_add(GTK_CONTAINER(hbuttonbox1),button2);
  GTK_WIDGET_SET_FLAGS(button2,GTK_CAN_DEFAULT);

  button3 = gtk_button_new_with_label("Clear");
  gtk_widget_show(button3);
  gtk_container_add(GTK_CONTAINER(hbuttonbox1),button3);
  GTK_WIDGET_SET_FLAGS(button3,GTK_CAN_DEFAULT);

  scrolledwindow1 = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_show(scrolledwindow1);
  gtk_box_pack_start(GTK_BOX(vbox1),scrolledwindow1,TRUE,TRUE,0);
  gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow1),5);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1),
      GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  search.list = gtk_clist_new(2);
  gtk_widget_show(search.list);
  gtk_container_add(GTK_CONTAINER(scrolledwindow1),search.list);
  gtk_clist_column_titles_show(GTK_CLIST(search.list));

  for(tmp=columns[0];tmp;tmp=columns[i]) {
    gtk_clist_set_column_width(GTK_CLIST(search.list),i,180);
    label5 = gtk_label_new(tmp);
    gtk_widget_show(label5);
    gtk_clist_set_column_widget(GTK_CLIST(search.list),i,label5);
    i++;
  }

  gtk_signal_connect(GTK_OBJECT(button2),"clicked",
      GTK_SIGNAL_FUNC(mib_search),
      NULL);

  gtk_signal_connect(GTK_OBJECT(button3),"clicked",
      GTK_SIGNAL_FUNC(clear_search),
      NULL);

  gtk_signal_connect(GTK_OBJECT(search.list),"select_row",
      GTK_SIGNAL_FUNC(search_row_selected),
      NULL);

  return;
}

