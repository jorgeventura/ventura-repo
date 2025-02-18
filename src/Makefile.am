## Process this file with automake to produce Makefile.in
INCLUDES = \
	@GTK_CFLAGS@ @SNMP_CFLAGS@

bin_PROGRAMS = mbrowse

mbrowse_SOURCES = \
	main.c \
	interface.c interface.h \
	details.c details.h \
	callbacks.c callbacks.h \
	snmpstuff.c snmpstuff.h \
	bookmarks.c bookmarks.h \
	mibtree.c mibtree.h \
	configuration.c configuration.h \
	search.c search.h icons.h version.h

mbrowse_LDADD = @GTK_LIBS@ @SNMP_LIBS@
