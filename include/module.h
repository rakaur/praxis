/*  praxis: services for TSora IRC networks.
 *  include/modules.h: Contains forward declarations for modules.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_modules_h
#define INCLUDED_modules_h

#ifndef STATIC_MODULES
typedef struct Module Module;
typedef struct ModulePath ModulePath;

struct Module
{
    char name[256];             /* file name minus extension */
    char filename[256];         /* full file name (without path name) */
    void *handle;               /* handle returned by dlopen() */

    ModuleHeader *moduleheader_p;       /* ModuleHeader */
    ModulePath *modulepath_p;   /* ModulePath */
};

struct ModulePath
{
    char path[1024];
};

DLinkList module_list;
DLinkList modules_loaded;
DLinkList module_paths;

void moduleInit(void);
void moduleAddAll(void);
Module *moduleAdd(const char *, const char *, ModulePath *);
Module *moduleFind(const char *, ModulePath *);
Module *moduleFindAny(const char *);
uchar moduleDelete(Module *);
uchar moduleFlush(void);
void modulePathAdd(const char *);
ModulePath *modulePathFind(const char *path);
uchar modulePathDelete(ModulePath *);
uchar modulePathFlush(void);
uchar moduleLoadAll(void);
uchar moduleLoad(Module *);
uchar moduleUnload(Module *);
#endif /* STATIC_MODULES */

#endif /* INCLUDED_modules_h */
