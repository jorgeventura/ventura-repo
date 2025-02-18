/*
 * mibtree.c
 *
 * Routines pertaining to manipulation of the mibtree
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
#include "icons.h"
#include "snmpstuff.h"

static GtkWidget *mib_tree_view;
static GtkTreeStore *mib_tree_store;
static GtkTreeSelection *mib_tree_selection;
static GtkWidget *menu;
static GtkWidget *mib_scrolledwin;
extern GtkWidget *oid_entry;

static GdkPixbuf *table_pixbuf;
GdkPixbuf *child_pixbuf;
GdkPixbuf *end_pixbuf;
static GdkPixbuf *trap_pixbuf;

static gboolean is_selected;
int dont_select_damnit;
int dont_update_entry;
extern GtkWidget *main_window;

static gint context_menu(GtkWidget *widget,GdkEvent *event);

void load_xpm(void) {
  table_pixbuf = gdk_pixbuf_new_from_xpm_data(snmp_table_xpm);
  child_pixbuf = gdk_pixbuf_new_from_xpm_data(folder_xpm);
  end_pixbuf = gdk_pixbuf_new_from_xpm_data(leaf_xpm);
  trap_pixbuf = gdk_pixbuf_new_from_xpm_data(trap);
}

/* Step through the tree until we arrive at the
 * last node associated with the oid.
 */

void select_oid(oid poid[],size_t size) {
  GtkTreeIter cur_node;
  GtkTreeIter last_selected;
  GtkTreePath *tree_path;
  GtkTreeSelection *tree_selection;
  struct tree *node_tree;
  int i;
  gboolean has_children;

  if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(mib_tree_store), &cur_node)) {
    return;
  }

  tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(mib_tree_view));

  for(i=0;i<size;i++) {
    has_children = FALSE;
    do {
      gtk_tree_model_get(GTK_TREE_MODEL(mib_tree_store), &cur_node, 2, &node_tree, -1);
      if (node_tree->subid == poid[i]) {
        last_selected = cur_node;
	tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(mib_tree_store), &cur_node);
	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(mib_tree_view), tree_path);
	gtk_tree_path_free(tree_path);
	has_children = gtk_tree_model_iter_children(GTK_TREE_MODEL(mib_tree_store), &cur_node, &last_selected);
        break;
      }
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(mib_tree_store), &cur_node));
    if (!has_children) {
      break;
    }
  }
  gtk_tree_selection_select_iter(tree_selection, &last_selected);

  tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(mib_tree_store), &last_selected);
  gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(mib_tree_view), tree_path, NULL, TRUE, 0.5, 0);
  gtk_tree_path_free(tree_path);
}

static int cmp_nodes(const void *a,const void *b) {

  return(((*(struct tree **)a)->subid - (*(struct tree **)b)->subid));
}

static void populate_varbinds(struct tree * next, GtkTreeIter *tree) {
  GtkTreeIter subtree;
  struct varbind_list * vl;
  struct tree * vt;
  struct module * vm;
  int vmi;
  vm = find_module(next->modid);
  for (vl = next->varbinds; vl; vl = vl->next) {
    vt = find_tree_node( vl->vblabel, next->modid );
    if (!vt) {
      for(vmi = 0; vmi < vm->no_imports ; vmi++) {
        if (!strcmp(vm->imports[vmi].label, vl->vblabel)) {
    	  vt = find_tree_node( vl->vblabel, vm->imports[vmi].modid );
          break;
    	}
      }
    }
    if (!vt) {
      fprintf(stderr, "Could not resolve trap varbind '%s' used in module %s (%s).\n", vl->vblabel, vm->name, vm->file);
      continue;
    }
    gtk_tree_store_append(mib_tree_store, &subtree, tree);
    gtk_tree_store_set(mib_tree_store, &subtree, 0, end_pixbuf, 1, vl->vblabel, 2, vt, -1);
  }
}

/* Recursively populate the mib tree with
 * parsed mibs
 */

void populate_mib_tree(struct tree *child, GtkTreeIter *tree) {

  struct tree *next;
  GtkTreeIter subtree;
  struct tree *items[1024];
  int i=0,j;
  GdkPixbuf *pixbuf;
  int has_varbinds = 0;

  next = (tree == NULL) ? child : child->child_list;
  for(; next; next = next->next_peer){
    items[i] = next;
    i++;
  }
  qsort(items,i,sizeof(struct tree *),cmp_nodes);

  for(j=0;j<i;j++) {
    next = items[j];
    if (next->type == TYPE_TRAPTYPE || next->type == TYPE_NOTIFTYPE) {
      pixbuf = trap_pixbuf;
      has_varbinds = 1;
    } else if (next->access == MIB_ACCESS_NOACCESS) {
      pixbuf = table_pixbuf;
    } else if (next->child_list) {
      pixbuf = child_pixbuf;
    } else {
      pixbuf = end_pixbuf;
    }
    gtk_tree_store_append(mib_tree_store, &subtree, tree);
    gtk_tree_store_set(mib_tree_store, &subtree, 0, pixbuf, 1, next->label, 2, next, -1);
    if (has_varbinds) populate_varbinds(next, &subtree);
    if (next->child_list) {
      populate_mib_tree(next,&subtree);
    }
  }
}


static gboolean on_tree_select(GtkTreeSelection *sel, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer user_data) {

  struct tree *signame;
  struct tree *next;
  char *tmp,*start,*end;
  char real[1024];
  unsigned long val;
  GtkTreeIter selection;

  if (path_currently_selected) {
    return TRUE;
  }
  is_selected = TRUE;

  gtk_tree_model_get_iter(model, &selection, path);

  gtk_tree_model_get(model, &selection, 2, &signame, -1);
  if (!signame)
    return TRUE;
  populate_detail_tab(signame);
  if (dont_update_entry) {
    return TRUE;
  }
  /* Put everything into a buffer backwards, then reverse it */
  start = tmp = real;
  for(next = signame;next && (tmp-real < 1023);next = next->parent) {
    val = next->subid;
    while(val > 0) {
      *tmp++ = val%10+'0';
      val /= 10;
    }
    *tmp++='.';
  }
  *tmp = 0x00;
  end = tmp-1;
  while(start < end) {
    char c;
    c = *start;
    *start = *end;
    *end = c;
    end--;
    start++;
  }
  dont_select_damnit = 1;
  gtk_entry_set_text(GTK_ENTRY(get_main_widgets()->oid_entry),real);
  dont_select_damnit = 0;

  return TRUE;
}

void expand_all(void) {
  gtk_tree_view_expand_all(GTK_TREE_VIEW(mib_tree_view));
}

void expand_below(void) {
  GtkTreePath *tree_path;
  GtkTreeIter selected;

  if (!is_selected)
    expand_all();
  else {
    gtk_tree_selection_get_selected(mib_tree_selection, NULL, &selected);
    tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(mib_tree_store), &selected);
    gtk_tree_view_expand_row(GTK_TREE_VIEW(mib_tree_view), tree_path, TRUE);
    gtk_tree_path_free(tree_path);
  }
}

void collapse_all(void) {
  gtk_tree_view_collapse_all(GTK_TREE_VIEW(mib_tree_view));
}

void collapse_below(void) {
  GtkTreePath *tree_path;
  GtkTreeIter selected;

  if (!is_selected)
    collapse_all();
  else {
    gtk_tree_selection_get_selected(mib_tree_selection, NULL, &selected);
    tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(mib_tree_store), &selected);
    gtk_tree_view_collapse_row(GTK_TREE_VIEW(mib_tree_view), tree_path);
    gtk_tree_path_free(tree_path);
  }
}

void retree(struct tree *mibtree) {
  gtk_tree_view_set_model(GTK_TREE_VIEW(mib_tree_view), NULL);
  g_object_unref(mib_tree_store);
  mib_tree_store = gtk_tree_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_tree_view_set_model(GTK_TREE_VIEW(mib_tree_view), GTK_TREE_MODEL(mib_tree_store));
  populate_mib_tree(mibtree,NULL);
}

void set_mibtree_style(int exp_style, int lines_enabled) {
  gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(mib_tree_view), lines_enabled);
}

static struct menu_items context_items[] = {
  { "Collapse All",collapse_all,NULL,NULL },
  { "Collaps All Below",collapse_below,NULL,NULL },
  { "-",NULL,NULL,NULL },
  { "Expand All",expand_all,NULL,NULL },
  { "Expand All Below",expand_below,NULL,NULL },
  { NULL,NULL,NULL,NULL },
};

static gint context_menu(GtkWidget *widget,GdkEvent *event) {

  GdkEventButton *event_button;

  if (event->type == GDK_BUTTON_PRESS) {
    event_button = (GdkEventButton *) event;
    if (event_button->button == 3) {  /* Right mouse button clicked */
      gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL, event_button->button, event_button->time);
    }
  }
  return(FALSE);
}

void create_mibtree_tab(void) {

  GtkWidget *mibtree_tab;
  GtkWidget *menu_item;
  GtkWidget *notebook;
  struct menu_items *m;
  GtkTreeViewColumn *tree_col;
  GtkCellRenderer *tree_renderer1;
  GtkCellRenderer *tree_renderer2;

  load_xpm();
  notebook = get_main_widgets()->notebook;
  mib_scrolledwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(mib_scrolledwin),
      GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_widget_show(mib_scrolledwin);
  gtk_container_add(GTK_CONTAINER(notebook),mib_scrolledwin);
  mibtree_tab = gtk_label_new("Mib Tree");
  gtk_widget_show(mibtree_tab);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),0),mibtree_tab);

  menu = gtk_menu_new();
  for(m=context_items;m->item;m++) {
    if (*m->item == '-')
      menu_item = gtk_menu_item_new();
    else
      menu_item = gtk_menu_item_new_with_label(m->item);
    gtk_menu_append(GTK_MENU(menu),menu_item);
    if (m->func) {
      gtk_signal_connect_object(GTK_OBJECT(menu_item),"activate",
                       		GTK_SIGNAL_FUNC(m->func),m->data);
    }
    gtk_widget_show(menu_item);
  } 

  mib_tree_view = gtk_tree_view_new();
  gtk_widget_show(mib_tree_view);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mib_tree_view), FALSE);
  gtk_container_add(GTK_CONTAINER(mib_scrolledwin),mib_tree_view);

  tree_col = gtk_tree_view_column_new();
  gtk_tree_view_append_column(GTK_TREE_VIEW(mib_tree_view), tree_col);
  tree_renderer1 = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(tree_col, tree_renderer1, FALSE);
  gtk_tree_view_column_add_attribute(tree_col, tree_renderer1, "pixbuf", 0);

  tree_renderer2 = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(tree_col, tree_renderer2, TRUE);
  gtk_tree_view_column_add_attribute(tree_col, tree_renderer2, "text", 1);

  mib_tree_store = gtk_tree_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_tree_view_set_model(GTK_TREE_VIEW(mib_tree_view), GTK_TREE_MODEL(mib_tree_store));
  mib_tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(mib_tree_view));
  gtk_tree_selection_set_mode(mib_tree_selection, GTK_SELECTION_BROWSE);

  gtk_signal_connect_object(GTK_OBJECT(mib_tree_view),"event",
      GTK_SIGNAL_FUNC(context_menu),GTK_OBJECT(menu));
  gtk_tree_selection_set_select_function(mib_tree_selection, on_tree_select, NULL, NULL);
}
