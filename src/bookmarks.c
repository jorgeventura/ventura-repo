/*
 * bookmarks.c
 *
 * All routines related to anything dealing with bookmarks.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "details.h"
#include "bookmarks.h"
#include "os.h"

static GtkWidget *bkdial;
int delete = 0;

static GtkWidget* create_bookmark_dialogue(void);

static void mark_for_delete(void) {

  if (delete == 0)
    disperror("Select the bookmark to delete.");
  else
    disperror("Bookmark delete canceled.");

  delete = 1-delete;
}

static void disp_bookmark_dialogue(void) {

  if (!bkdial || !GTK_IS_WIDGET(bkdial)) {
    bkdial = create_bookmark_dialogue();
  } else {
    if (GTK_WIDGET_MAPPED(bkdial))
      gtk_widget_show(bkdial);
    else
      gtk_widget_show(bkdial);
  }
}

void load_bookmarks(void) {

  const char *home;
  char fn[1024];
  FILE *fp;
  char *tmp;
  int i=3;

  bookmark_items[0].item = "Add Bookmark";
  bookmark_items[0].func = disp_bookmark_dialogue;
  bookmark_items[1].item = "Delete Bookmark";
  bookmark_items[1].func = mark_for_delete;
  bookmark_items[2].item = "-";
  bookmark_items[2].func = NULL;

#ifdef WIN32
  home = ".\\";
#else
  home = getenv("HOME");
#endif

  strncpy(fn,home,1023);

  strcat(fn,PATH_SEPARATOR".mbrowse_bookmarks");

  if ((fp = fopen(fn,"r")) == NULL) {
    fp = fopen(fn,"w");
  } else {
    while(fgets(fn,1023,fp)) {
      *(fn+strlen(fn)-1) = 0x00;
      for(tmp=fn;*tmp;tmp++) {
        if (i == 126)
          break;
        switch(*tmp) {
          case ';':
            *tmp = 0x00;
            bookmark_items[i].item = strdup(fn);
            bookmark_items[i].func = GTK_SIGNAL_FUNC(set_oid_entry);
            bookmark_items[i].data = strdup(tmp+1);
            i++;
          default:
            break;
        }
      }
      bookmark_items[i].item = NULL;
      bookmark_items[i].func = NULL;
    }
  }
  fclose(fp);
}

/* When a bookmark is added, rewrite the file, and
 * update the menu.
 */

static void add_bookmark(gpointer data) {

  const gchar *text,*oid;
  char fn[1024];
  FILE *fp;
  GtkWidget *new_item;
  struct menu_items *p;
  main_widgets_t *mwid;

  mwid = get_main_widgets();
  text = gtk_entry_get_text(GTK_ENTRY(data));
  oid = gtk_entry_get_text(GTK_ENTRY(get_main_widgets()->oid_entry));

#ifdef WIN32
  strcpy(fn,".\\");
#else
  strcpy(fn,getenv("HOME"));
#endif
  strcat(fn, PATH_SEPARATOR ".mbrowse_bookmarks");
  fp = fopen(fn,"a");
  snprintf(fn,1023,"%s;%s\n",text,oid);
  fputs(fn,fp);
  fclose(fp);
  gtk_widget_hide(bkdial);
  for(p = bookmark_items;p->item;p++);
  p->item = strdup(text);
  p->func = GTK_SIGNAL_FUNC(set_oid_entry);
  p->data = strdup(oid);
  new_item = gtk_menu_item_new_with_label(p->item);
  p->mi = new_item;
  gtk_menu_append(GTK_MENU(mwid->bookmark_menu),new_item);
  gtk_signal_connect_object(GTK_OBJECT(new_item),"activate",
                            GTK_SIGNAL_FUNC(p->func),
                            (gpointer) p->data);
  gtk_widget_show(new_item);
  p++;
  p->item = NULL;
}

static GtkWidget* create_bookmark_dialogue(void) {

  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *vbox1;
  GtkWidget *label1;
  GtkWidget *entry1;
  GtkWidget *dialog_action_area1;
  GtkWidget *hbuttonbox1;
  GtkWidget *button1;
  GtkWidget *button2;

  /* Create the Dialog */

  dialog1 = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog1),"dialog1");
  GTK_WINDOW(dialog1)->type = GTK_WINDOW_TOPLEVEL;
  gtk_window_set_modal(GTK_WINDOW(dialog1),TRUE);
  gtk_window_set_position(GTK_WINDOW(dialog1),GTK_WIN_POS_MOUSE);
  gtk_window_set_policy(GTK_WINDOW(dialog1),TRUE,TRUE,FALSE);

  dialog_vbox1 = GTK_DIALOG(dialog1)->vbox;
  gtk_widget_show(dialog_vbox1);

  vbox1 = gtk_vbox_new(FALSE,0);
  gtk_widget_show(vbox1);
  gtk_box_pack_start(GTK_BOX(dialog_vbox1),vbox1,FALSE,TRUE,0);

  label1 = gtk_label_new("Enter a name for this bookmark");
  gtk_widget_show(label1);
  gtk_box_pack_start(GTK_BOX(vbox1),label1,FALSE,FALSE,0);
  gtk_misc_set_padding(GTK_MISC(label1),0,3);

  entry1 = gtk_entry_new();
  gtk_widget_show(entry1);
  gtk_box_pack_start(GTK_BOX(vbox1),entry1,FALSE,FALSE,10);

  dialog_action_area1 = GTK_DIALOG(dialog1)->action_area;
  gtk_widget_show(dialog_action_area1);
  gtk_container_set_border_width(GTK_CONTAINER(dialog_action_area1),10);

  hbuttonbox1 = gtk_hbutton_box_new();
  gtk_widget_show(hbuttonbox1);
  gtk_box_pack_start(GTK_BOX(dialog_action_area1),hbuttonbox1,FALSE,FALSE,0);

  button1 = gtk_button_new_with_label("OK");
  gtk_widget_show(button1);

  gtk_container_add(GTK_CONTAINER(hbuttonbox1),button1);
  GTK_WIDGET_SET_FLAGS(button1,GTK_CAN_DEFAULT);

  button2 = gtk_button_new_with_label("Cancel");
  gtk_widget_show(button2);
  gtk_container_add(GTK_CONTAINER(hbuttonbox1),button2);
  GTK_WIDGET_SET_FLAGS(button2, GTK_CAN_DEFAULT);

  gtk_signal_connect(GTK_OBJECT(dialog1),"destroy",
      (GtkSignalFunc) gtk_widget_destroy,&dialog1);
  gtk_signal_connect_object(GTK_OBJECT(button2),"clicked",
      (GtkSignalFunc) gtk_widget_hide,GTK_OBJECT(dialog1));
  gtk_signal_connect_object(GTK_OBJECT(button1),"clicked",
      (GtkSignalFunc) add_bookmark,GTK_OBJECT(entry1));
  gtk_widget_show(dialog1);
  return dialog1;
}
