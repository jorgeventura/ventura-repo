/*
 * snmpstuff.c
 *
 * These functions deal with processing mib/snmp data.
 * A bunch of this is ucd-snmp code hacked to deal with
 * a GUI interface.
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
#include "configuration.h"
#include "snmpstuff.h"

#define GTK_SNMP_PERROR disperror(snmp_api_errstring(snmp_errno));

#ifdef HAVE_BROKEN_API
#  define snprint_variable(a,b,c,d,e) \
     sprint_variable(binit(NULL, a, b),c,d,e);
#  define snprint_objid(a,b,c,d) \
     sprint_variable(binit(NULL,a,b),c,d);
#else
#  ifndef HAVE_NETSNMP
#    define snprint_variable(a,b,c,d,e) \
       sprint_variable(a,c,d,e);
#    define snprint_objid(a,b,c,d) \
       sprint_objid(a,c,d)
#  endif
#endif


oid objid_mib[] = {1, 3, 6, 1};

int numprinted;
struct tree *mibtree;

#define READ_ONLY	0
#define READ_WRITE 	1

static struct snmp_session *mbrowse_create_snmp_session(struct snmp_session *session,int comm_type) {

  config_t *config;

  char *peername = alloca(strlen(get_hostname()) + 1);
  strcpy(peername, get_hostname());
  char *readcomm = alloca(strlen(get_readcomm()) + 1);
  strcpy(readcomm, get_readcomm());
  char *writecomm = alloca(strlen(get_writecomm()) + 1);
  strcpy(writecomm, get_writecomm());

  config = get_config();
  snmp_sess_init(session);
  session->version = config->snmp_ver;
  session->peername = peername;
  session->retries = config->snmp_retries;
  session->timeout = config->snmp_timeout * 1000000L;
  session->remote_port = config->snmp_port; 
  switch(comm_type) {
    case READ_WRITE:
      session->community = (u_char *)writecomm;
      break;
    case READ_ONLY:
    default:
      session->community = (u_char *)readcomm;
      break;
  }
  session->community_len = strlen((const char *)session->community);
  return(snmp_open(session));
}

struct tree *init_mbrowse_snmp(void) {

  snmp_set_save_descriptions(1);
  init_snmp("browser");
/*  init_mib_internals();
  adopt_orphans();*/
  mibtree = read_all_mibs();
  snmp_set_full_objid(1);
  return(mibtree);
}

static void _snmpget(struct snmp_session *ss, oid *theoid, 
              size_t theoid_len, gpointer window) {

  struct snmp_pdu *pdu, *response;
  struct variable_list *vars;
  int status;
  char buf[SPRINT_MAX_LEN];
  GtkTextBuffer *textBuf;
  GtkTextIter textIter;

  pdu = snmp_pdu_create(SNMP_MSG_GET);
  snmp_add_null_var(pdu, theoid, theoid_len);

  status = snmp_synch_response(ss, pdu, &response);
  if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
    for(vars = response->variables; vars; vars = vars->next_variable) {
      numprinted++;
      snprint_variable(buf, sizeof(buf),vars->name,vars->name_length,vars);
      strcat(buf,"\n");
      textBuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window));
      gtk_text_buffer_get_end_iter(textBuf,&textIter);
      gtk_text_buffer_insert(textBuf,&textIter,buf,-1);
      handle_events();
    }
  }
  if (response)
    snmp_free_pdu(response);
}

void snmpget(const char *host,const char *object,const char *community,gpointer user_data) {

  char buf[SPRINT_MAX_LEN],temp[SPRINT_MAX_LEN];
  char *error;
  struct snmp_session session, *ss;
  struct snmp_pdu *pdu;
  struct snmp_pdu *response = NULL;
  struct variable_list *vars;
  int count;
  oid name[MAX_OID_LEN];
  size_t name_length = MAX_OID_LEN;
  int status;
  GtkTextBuffer *textBuf;
  GtkTextIter textIter;

  ss = mbrowse_create_snmp_session(&session,READ_ONLY);
  if (ss == NULL){
    /* diagnose snmp_open errors with the input struct snmp_session pointer */
    snmp_error(&session,NULL,NULL,&error);
    disperror(error);
    return;
  }
  pdu = snmp_pdu_create(SNMP_MSG_GET);
  if (!snmp_parse_oid(object, name, &name_length)) {
    GTK_SNMP_PERROR;
    return;
  } else
    snmp_add_null_var(pdu, name, name_length);

    status = snmp_synch_response(ss, pdu, &response);
    if (status == STAT_SUCCESS){
      if (response->errstat == SNMP_ERR_NOERROR){
        for(vars = response->variables; vars; vars = vars->next_variable) {
	  snprint_variable(buf,sizeof(buf),vars->name,vars->name_length,vars);
	  textBuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(user_data));
	  gtk_text_buffer_get_end_iter(textBuf,&textIter);
	  gtk_text_buffer_insert(textBuf,&textIter,buf,-1);
	  handle_events();
        }
      } else {
        sprintf(buf,"Error in Packet: %s",snmp_errstring(response->errstat));
        disperror(buf);
        if (response->errstat == SNMP_ERR_NOSUCHNAME){
          sprintf(temp,"This name doesn't exist: ");
          for(count = 1, vars = response->variables;
                vars && count != response->errindex;
                vars = vars->next_variable, count++)
          if (vars) {
	    snprint_objid(buf,sizeof(buf),vars->name,vars->name_length);
            strcat(temp,buf);
          }
          disperror(temp);
        }
      }  /* endif -- SNMP_ERR_NOERROR */
    } else if (status == STAT_TIMEOUT){
      disperror("Timed out. (Check community string)");
      snmp_close(ss);
      return;
    } else {    /* status == STAT_ERROR */
      snmp_error(&session,NULL,NULL,&error);
      disperror(error);
      snmp_close(ss);
      return;
    }  /* endif -- STAT_SUCCESS */
    if (response)
      snmp_free_pdu(response);
    snmp_close(ss);
}

void snmpwalk(const char *host,const char *object,const char *community,gpointer user_data) {

  char buf[SPRINT_MAX_LEN],temp[SPRINT_MAX_LEN];
  char *error;
  struct snmp_session session, *ss;
  struct snmp_pdu *pdu;
  struct snmp_pdu *response = NULL;
  struct variable_list *vars;
  int count;
  oid root[MAX_OID_LEN];
  size_t rootlen = MAX_OID_LEN;
  oid name[MAX_OID_LEN];
  size_t name_length = MAX_OID_LEN;
  int status = 0;
  int notdone = 1;
  GtkTextBuffer *textBuf;
  GtkTextIter textIter;

  numprinted=0;
  ss = mbrowse_create_snmp_session(&session,READ_ONLY);
  if (ss == NULL){
   /* diagnose snmp_open errors with the input struct snmp_session pointer */
    snmp_error(&session,NULL,NULL,&error);
    disperror(error);
    return;
  }
  
  if (object && strlen(object)) {
    if (!snmp_parse_oid(object, root, &rootlen)) {
      GTK_SNMP_PERROR;
      return;
    }
  } else {
   memmove(root, objid_mib, sizeof(objid_mib));
   rootlen = sizeof(objid_mib) / sizeof(oid);
  }
  memmove(name, root, rootlen * sizeof(oid));
  name_length = rootlen;
  _snmpget(ss, root, rootlen, user_data);
  while(notdone) {
    pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
    snmp_add_null_var(pdu, name, name_length);

    /* do the request */
    status = snmp_synch_response(ss, pdu, &response);
      if (status == STAT_SUCCESS){
        if (response->errstat == SNMP_ERR_NOERROR){
          /* check resulting variables */
          for(vars = response->variables; vars; vars = vars->next_variable){
            if ((vars->name_length < rootlen) ||
                (memcmp(root, vars->name, rootlen * sizeof(oid))!=0)) {
              /* not part of this subtree */
              notdone = 0;
              continue;
            }
            numprinted++;
	    snprint_variable(buf,sizeof(buf),vars->name,vars->name_length,vars);
            strcat(buf,"\n");
	    textBuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(user_data));
	    gtk_text_buffer_get_end_iter(textBuf,&textIter);
	    gtk_text_buffer_insert(textBuf,&textIter,buf,-1);
	    handle_events();
            if ((vars->type != SNMP_ENDOFMIBVIEW) &&
                (vars->type != SNMP_NOSUCHOBJECT) &&
                (vars->type != SNMP_NOSUCHINSTANCE)){
              /* not an exception value */
              memmove((char *)name, (char *)vars->name,
              vars->name_length * sizeof(oid));
              name_length = vars->name_length;
            } else
              /* an exception value, so stop */
              notdone = 0;
          }
        } else {
          /* error in response, print it */
          notdone = 0;
          if (response->errstat == SNMP_ERR_NOSUCHNAME){
	    textBuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(user_data));
	    gtk_text_buffer_get_end_iter(textBuf,&textIter);
	    gtk_text_buffer_insert(textBuf,&textIter,"End of MIB\n",-1);
	    handle_events();
          } else {
            sprintf(buf,"Error in Packet: %s",
                     snmp_errstring(response->errstat));
            disperror(buf);
            if (response->errstat == SNMP_ERR_NOSUCHNAME) {
              sprintf(temp,"The request for this object identifier failed: ");
              for(count = 1, vars = response->variables;
                    vars && count != response->errindex;
                    vars = vars->next_variable, count++)
              if (vars) {
		snprint_objid(buf,sizeof(buf),vars->name,vars->name_length);
                strcat(temp,buf);
              }
              disperror(temp);
            }
          }
        }
      } else if (status == STAT_TIMEOUT){
        disperror("Timed out. (Check community string)");
        notdone = 0;
        snmp_close(ss);
        return;
      } else {    /* status == STAT_ERROR */
        snmp_sess_perror("snmpwalk", ss);
        notdone = 0;
      }
      if (response)
        snmp_free_pdu(response);
    }
    if (numprinted == 0 && status == STAT_SUCCESS) {
        /* no printed successful results, which may mean we were
           pointed at an only existing instance.  Attempt a GET, just
           for get measure. */
        _snmpget(ss, root, rootlen, user_data);
    }
    snmp_close(ss);
    sprintf(buf,"Variables found: %d", numprinted);
    disperror(buf);
}

void snmpset(const char *host,const char *object,const char *community,const char *value,
             gpointer window) {

  char buf[SPRINT_MAX_LEN];
  char *error;
  struct snmp_session session,*ss;
  struct snmp_pdu *pdu;
  struct snmp_pdu *response;
  struct variable_list *vars;
  struct tree *subtree;
  oid name[MAX_OID_LEN];
  size_t name_length = MAX_OID_LEN;
  int status;
  char type = 0;
  GtkTextBuffer *textBuf;
  GtkTextIter textIter;

  if (!value || (*value == 0)) {
    disperror("Value is required for a set.");
    return;
  }
  if (!read_objid(object,name,&name_length)) {
    disperror("Invalid Object ID");
    return;
  }
  subtree = get_tree(name,name_length,get_tree_head());
  type = get_as();
  ss = mbrowse_create_snmp_session(&session,READ_WRITE);
  if (ss == NULL) {
    snmp_error(&session,NULL,NULL,&error);
    disperror(error);
    return;
  }
  pdu = snmp_pdu_create(SNMP_MSG_SET);
  if (!snmp_parse_oid(object,name,&name_length)) {
    GTK_SNMP_PERROR;
    return;
  } else {
     if (snmp_add_var(pdu,name,name_length,type,value)) {
       GTK_SNMP_PERROR;
       return;
     }
  }
  status = snmp_synch_response(ss,pdu,&response);
  if (status == STAT_SUCCESS) {
    if (response->errstat == SNMP_ERR_NOERROR) {
      for(vars = response->variables; vars; vars = vars->next_variable) {
	snprint_variable(buf,sizeof(buf),vars->name,vars->name_length,vars);
        strcat(buf,"\n");
	  textBuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window));
	  gtk_text_buffer_get_end_iter(textBuf,&textIter);
	  gtk_text_buffer_insert(textBuf,&textIter,buf,-1);
	  handle_events();
      }
    } else {
      sprintf(buf,"Error in Packet: %s",snmp_errstring(response->errstat));
      disperror(buf);
    }
  } else if (status == STAT_TIMEOUT) {
    disperror("Timed out. (Check community string)");
    snmp_close(ss);
    return;
  } else {
    snmp_error(&session,NULL,NULL,&error);
    disperror(error);
    snmp_close(ss);
  }
  if (response)
    snmp_free_pdu(response);
  snmp_close(ss);
}
