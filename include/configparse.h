/*  praxis: services for TSora IRC networks.
 *  src/config.c: Configuration file parser.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *  Copyright (c) 1999-2004 csircd development team.
 *
 *  $Id$
 */


#ifndef INCLUDED_configparse_h
#define INCLUDED_configparse_h

typedef struct ConfigFile ConfigFile;
typedef struct ConfigEntry ConfigEntry;
typedef struct ConfigTable ConfigTable;

struct ConfigFile
{
    char *cf_filename;
    ConfigEntry *cf_entries;
    ConfigFile *cf_next;
};

struct ConfigEntry
{
    ConfigFile *ce_fileptr;

    int ce_varlinenum;
    char *ce_varname;
    char *ce_vardata;
    int ce_vardatanum;
    int ce_fileposstart;
    int ce_fileposend;

    int ce_sectlinenum;
    ConfigEntry *ce_entries;

    ConfigEntry *ce_prevlevel;

    ConfigEntry *ce_next;
};

struct ConfigTable
{
    char *name;
    uchar rehashable;
    uchar (*func) (ConfigEntry *);
};

void config_free(ConfigFile *cfptr);
ConfigFile *config_load(char *filename);
ConfigEntry *config_find(ConfigEntry *ceptr, char *name);

#endif /* INCLUDED_configparse_h */
