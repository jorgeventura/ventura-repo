struct tree *init_mbrowse_snmp(void);
void snmpget(const char *host,const char *object,const char *community,gpointer );
void snmpwalk(const char *host,const char *object,const char *community,gpointer );
void snmpset(const char *host,const char *object,const char *community,const char *,gpointer );
