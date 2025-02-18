/*
 * mbrowse - A graphical SNMP MIB browser based on GTK+ and Net-SNMP
 * Copyright (C) 2002-2003  Aaron Hodgen <ahodgen@munsterman.com>
 * Copyright (C) 2010  Marcel Šebek <sebek64@post.cz>
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "callbacks.h"
#include "interface.h"
#include "details.h"
#include "search.h"
#include "bookmarks.h"
#include "mibtree.h"
#include "configuration.h"
#include "version.h"

GtkWidget *main_window;

int main(int argc, char **argv) {

  int ch;

#ifdef WIN32
  WSADATA wsa;
  WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

  gtk_init (&argc, &argv);
  while((ch = getopt(argc,argv,"hv")) != -1) {
    switch(ch) {
      case 'h':
      case 'v':
	printf("mbrowse %s\n"
	       "Copyright 2010 Marcel Šebek <sebek64@post.cz>\n"
	       "Copyright 2002 Aaron Hodgen <ahodgen@munsterman.com>\n"
	       "This program comes with NO WARRANTY, to the extent permitted by law.\n"
	       "You may redistribute it under the terms of the GNU General Public License;\n"
	       "see the file named COPYING for details.\n",mbrowse_version);
	exit(EXIT_SUCCESS);
	break;
      default:
	break;
    }
  }
    load_bookmarks();
  main_window = create_main_window();
  create_mibtree_tab();
  create_detail_tab();
  create_search_tab();
  create_config_tab(3);
  init_stuff();
  load_config();
  gtk_widget_show(main_window);
  gtk_main();
#ifdef WIN32
  WSACleanup();
#endif
  return(EXIT_SUCCESS);
}
