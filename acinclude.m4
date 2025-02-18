dnl AM_PATH_NETSNMP([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]])

dnl Test for net-snmp and define NETSNMP_CFLAGS and NETSNMP_LIBS

AC_DEFUN(AM_PATH_NETSNMP,[
    AC_ARG_WITH(snmp-type,
      [  --with-snmp-type=TYPE   Which snmp version to use (net or ucd)],
      snmp_type="$withval",snmp_type="")
    AC_ARG_WITH(snmp-prefix,
      [  --with-snmp-prefix=PFX  Prefix where net/ucd snmp is installed (optional)],
      snmp_prefix="$withval",snmp_prefix="")
    AC_ARG_WITH(snmp-include,
      [  --with-snmp-include=DIR Directory pointing to snmp include files (optional)],
      snmp_include="$withval",snmp_include="")
    AC_ARG_WITH(snmp-lib,
      [  --with-snmp-lib=LIB     Directory containing the snmp library to use (optional)
                           (Notes: -lib ignored for net-snmp, -lib and -include
                            override -prefix)],
      snmp_lib="$withval",snmp_lib="")

    guess_type=""
    SNMP_CFLAGS=""
    SNMP_LIBS=""

    dnl
    dnl Check the user-provided include and libs first
    dnl

    if test "x$snmp_include" != "x" ; then
      for trytype in ucd net ${snmp_type} ; do
        if test -d "$snmp_include/$trytype-snmp" ; then
	  SNMP_CFLAGS="-I$snmp_include"
	  guess_type="$trytype"
	  found_snmp_incl="1"
        fi
      done
    fi

    if test "x$snmp_lib" != "x" ; then
      for trytype in libsnmp.sl libsnmp.so libsnmp.a  libnetsnmp.sl libnetsnmp.so libnetsnmp.a ; do
        if test -r "$snmp_lib/$trytype" ; then
	  SNMP_LIBS="-L$snmp_lib -lsnmp"
	  found_snmp_lib="1"
        fi
      done
    fi

    dnl
    dnl Check for --with-snmp-type and -prefix at the same time
    dnl

    AC_MSG_CHECKING([for snmp-type])

    for tryprefix in /usr /usr/local /usr/pkg $snmp_prefix ; do

      if test "x$found_snmp_incl" = "x" ; then
        for trytype in ucd net ${snmp_type} ; do
          if test -d "$tryprefix/include/$trytype-snmp" ; then
            SNMP_CFLAGS="-I$tryprefix/include"
	    guess_type="$trytype"
	    backup_prefix="$tryprefix"
	  fi
        done
      fi

      if test "x$found_snmp_lib" = "x" ; then
        for trytype in libsnmp.sl libsnmp.so libsnmp.a libnetsnmp.sl libnetsnmp.so libnetsnmp.a ; do
          if test -r "$tryprefix/lib/$trytype" ; then
            SNMP_LIBS="-L$tryprefix/lib64 -lsnmp"
          fi
        done
      fi

    done

    if test "x$SNMP_LIBS" = "x" ; then
      if test "x$SNMP_CFLAGS" = "x" ; then
        echo 
        echo "***"
        echo "*** Could not locate any version of ucd-snmp or net-snmp"
        echo "***"
        echo "*** Either it is not installed, or installed in a bizarre location."
        echo "***"
        echo "*** Try using one of the following switches:"
        echo "***   --with-snmp-prefix=PFX"
        echo "***   --with-snmp-include=DIR"
        echo "***   --with-snmp-lib=DIR"
        echo "***"
        ifelse([$3], ,:,[$3])
      else
        echo
        echo "***"
        echo "*** Found snmp headers, but could not locate the libraries."
        echo "***"
        echo "*** Try using the following switch:"
        echo "***   --with-snmp-prefix=PFX"
        echo "***   --with-snmp-lib=DIR"
        echo "***"
        ifelse([$3], ,:,[$3])
      fi
    elif test "x$SNMP_CFLAGS" = "x" ; then
      echo
      echo "***"
      echo "*** Found snmp libraries, but could not locate headers."
      echo "***"
      echo "*** If this was installed from a package, the -devel package"
      echo "*** was proabably not installed."
      echo "***"
      echo "*** Otherwise, try one of the following switches:"
      echo "***   --with-snmp-prefix=PFX"
      echo "***   --with-snmp-include=DIR"
      echo "***"
      ifelse([$3], ,:,[$3])
    fi

    if test "x$guess_type" = "xnet" ; then
      if test "x$snmp_prefix" != "x" ; then
        if test -x "$snmp_prefix/bin/net-snmp-config" ; then
          SNMP_NET_LIBS=`$snmp_prefix/bin/net-snmp-config --netsnmp-libs`
          SNMP_EXT_LIBS=`$snmp_prefix/bin/net-snmp-config --external-libs`
          SNMP_LIBS="$SNMP_NET_LIBS $SNMP_EXT_LIBS"
	  SNMP_CFLAGS="$SNMP_CFLAGS -DHAVE_NETSNMP"
        else
          guess_type="ucd"
        fi
      else
        if test -x "$backup_prefix/bin/net-snmp-config" ; then
	  SNMP_LIBS=`$backup_prefix/bin/net-snmp-config --libs`
	  SNMP_CFLAGS="$SNMP_CFLAGS -DHAVE_NETSNMP"
	else
         guess_type="ucd"
        fi
      fi
    fi

    AC_MSG_RESULT($guess_type)

    if test "x$guess_type" = "xucd" ; then
      AC_CHECK_LIB(socket,getservbyname)
      AC_CHECK_FUNC(gethostbyname, ,
      AC_CHECK_LIB(nsl, gethostbyname))
      AC_CHECK_LIB(nsl,gethostbyname)
      AC_CHECK_LIB(crypto,EVP_md5)
      AC_CHECK_LIB(kstat,kstat_lookup)
    elif test "x$guess_type" != "xnet" ; then
      echo
      echo "***"
      echo "*** I have no clue what to do with this version of SNMP."
      echo "*** Net-snmp or ucd-snmp must be installed."
      echo "***"
    fi
    
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"

    CFLAGS="$CFLAGS $SNMP_CFLAGS"
    LIBS="$SNMP_LIBS $LIBS"

    AC_MSG_CHECKING([for $guess_type-snmp ifelse([$1], , ,[>= v$1])])
    dnl if no minimum version is given, just try to compile
    dnl else try to compile AND run

    if test "x$1" = "x" ; then
      AC_TRY_COMPILE([
        #include <stdarg.h>
        #include <stdio.h>
        #include <string.h>
        #include <sys/types.h>
        #ifdef HAVE_NETSNMP
        #  include <net-snmp/net-snmp-config.h>
        #  include <net-snmp/net-snmp-includes.h>
        #else
        #  include <ucd-snmp/asn1.h>
        #  include <ucd-snmp/snmp_api.h>
        #  include <ucd-snmp/mib.h>
        #  include <ucd-snmp/snmp.h>
        #endif
      ],[
        struct snmp_session s;
	bzero( &s, sizeof( struct snmp_session));
	snmp_open(&s);
      ],[AC_MSG_RESULT(yes)
        CFLAGS="$ac_save_CFLAGS"
        LIBS="$ac_save_LIBS"
        ifelse([$2], ,:,[$2])
      ],[
        $snmp_fail="yes"
      ])
    else
      snmp_min_version=$1
      AC_TRY_RUN([
        #include <stdarg.h>
        #include <stdio.h>
        #include <string.h>

        #include <sys/types.h>
        #ifdef HAVE_NETSNMP
        #  include <net-snmp/net-snmp-config.h>
        #  include <net-snmp/net-snmp-includes.h>
        #  include <net-snmp/version.h>
        #else
        #  include <ucd-snmp/asn1.h>
        #  include <ucd-snmp/snmp_api.h>
        #  include <ucd-snmp/mib.h>
        #  include <ucd-snmp/snmp.h>
        #  include <ucd-snmp/version.h>
        #endif

        int main() {
          int major, minor,micro;
          int snmp_major, snmp_minor, snmp_micro;
          char *version1, *version2;
          struct snmp_session s;

          bzero(&s, sizeof( struct snmp_session));
          snmp_open(&s);

	  version1 = strdup( "$snmp_min_version" );
          if( sscanf( version1, "%d.%d.%d", &major, &minor, &micro) != 2) {
            printf( "%s, bad supplied version string\n", "$netsnmp_min_version");
            exit(1);
          }

          #ifdef HAVE_NETSNMP
	    version2 = strdup( netsnmp_get_version() );
          #else
            version2 = strdup( VersionInfo );
          #endif
          if( sscanf( version2, "%d.%d.%d", &snmp_major, &snmp_minor, &snmp_micro) != 3) {
	    /* Some people tripped on this with 4.2... check for it */
	    if ( sscanf( version2, "%d.%d", &snmp_major, &snmp_minor) != 2) {
              printf( "%s, bad net-snmp version string\n", version2);
              exit(1);
	    }
	    snmp_micro=0;
          }

          if((snmp_major > major) ||
            ((snmp_major == major) && (snmp_minor > minor)) ||
            ((snmp_major == major) && (snmp_minor == minor)) ||
	    ((snmp_major == major) && (snmp_minor == minor) && (snmp_micro >= micro))) {

	    if ((snmp_major == 4) && (snmp_minor <= 2) && (snmp_micro <=1)) {
	      printf("\n***\n*** You are running ucd-snmp < 4.2.2. It\n"
		     "*** is strongly suggested that you upgrade\n"
		     "*** for security reasons.  See CERT Advisory\n"
		     "**** CA-2002-03 for more details.\n***\n");
	    }
            return 0;
          } else {
            printf("\n***\n*** An old version of net-snmp (%d.%d) was found.\n",
                   snmp_major, snmp_minor);
            printf("*** You need a version of net-snmp newer than %d.%d. The latest version of\n",
                   major, minor);
            printf("*** net-snmp is available from ftp://net-snmp.sourceforge.net/ .\n***\n");
          }
          return 1;
      }
    ],[AC_MSG_RESULT(yes)
      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"
      ifelse([$2], ,:,[$2])
    ],[
      snmp_fail="yes"
    ])
  fi

  if test "x$snmp_fail" != "x" ; then
    echo
    echo "***"
    echo "*** Snmp test source had problems, check your config.log."
    echo "*** Chances are, it couldn't find the headers or libraries."
    echo "***"
    echo "*** Also try one of the following switches :"
    echo "***   --with-snmp-prefix=PFX"
    echo "***   --with-snmp-include=DIR"
    echo "***   --with-snmp-lib=DIR"
    echo "***"
    CFLAGS="$ac_save_CFLAGS"
    LIBS="$ac_save_LIBS"
    ifelse([$3], ,:,[$3])
  fi

  CFLAGS="$ac_save_CFLAGS"
  LIBS="$ac_save_LIBS"

  AC_SUBST(SNMP_CFLAGS)
  AC_SUBST(SNMP_LIBS)
])
