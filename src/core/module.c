/*  praxis: services for TSora IRC networks.
 *  src/module.c: dlopen() shared modules.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "balloc.h"
#include "ilog.h"
#include "imem.h"
#include "module.h"

#ifndef STATIC_MODULES
static Heap *module_heap;

/* moduleInit()
 *     Initialises the module_list.
 *
 * inputs     - none
 * outputs    - none
 */
void
moduleInit(void)
{
    module_heap = ballocHeapCreate(sizeof(Module), MODULE_HEAP_SIZE);
    memset(&module_list, '\0', sizeof(module_list));
    memset(&modules_loaded, '\0', sizeof(modules_loaded));
    memset(&module_paths, '\0', sizeof(module_paths));

    if (module_heap == NULL)
    {
        ilog(L_INFO, "moduleInit(): ballocHeapCreate() failed!");
        exit(EXIT_FAILURE);
    }
}

/* moduleAddAll()
 *     Adds all modules found in module_paths.
 *
 * inputs     - none
 * outputs    - none
 */
void
moduleAddAll(void)
{
    ModulePath *modulepath_p;
    DLinkNode *node_p;
    DIR *module_dir;
    struct dirent *ldirent = NULL;
    int len;
    char name[256];

    DLINK_FOREACH(node_p, module_paths.head)
    {
        modulepath_p = node_p->data;

        module_dir = opendir(modulepath_p->path);

        if (module_dir == NULL)
        {
            ilog(L_INFO, "moduleAddAll(): Cannot open directory %s: %s",
                 modulepath_p->path, strerror(errno));

            exit(EXIT_FAILURE);
        }

        while ((ldirent = readdir(module_dir)) != NULL)
        {
            len = strlen(ldirent->d_name);

            if ((len > 3) && (!strcmp((ldirent->d_name + len - 3), ".so")))
            {
                strlcpy(name, (ldirent->d_name + 2), 256);
                len = strlen(name);
                name[(len - 3)] = '\0';
                moduleAdd(name, ldirent->d_name, modulepath_p);
            }
        }

        closedir(module_dir);
    }
}

/* moduleAdd()
 *     Adds a module to the module_list.
 *
 * inputs     - name, file name, ModulePath
 * returns    - pointer to Module or NULL on failure
 */
Module *
moduleAdd(const char *name, const char *filename, ModulePath * modulepath_p)
{
    Module *module_p;

    iassert(name != NULL);
    iassert(filename != NULL);
    iassert(modulepath_p != NULL);

    ilog(L_DEBUG2, "moduleAdd(): Adding %s/%s", modulepath_p->path, filename);

    module_p = ballocHeapAlloc(module_heap);

    strlcpy(module_p->name, name, 256);
    strlcpy(module_p->filename, filename, 256);
    module_p->modulepath_p = modulepath_p;

    dlinkAddAlloc(module_p, &module_list);

    return module_p;
}

/* moduleFind()
 *     Finds a Module in the module_list.
 *
 * inputs     - file name, ModulePath
 * outputs    - pointer to Module or NULL on failure
 */
Module *
moduleFind(const char *filename, ModulePath * modulepath_p)
{
    Module *module_p;
    DLinkNode *node_p;

    iassert(filename != NULL);
    iassert(modulepath_p != NULL);

    DLINK_FOREACH(node_p, module_list.head)
    {
        module_p = node_p->data;

        if ((!strcmp(module_p->filename, filename)) &&
            (module_p->modulepath_p == modulepath_p))
            return module_p;
    }

    return NULL;
}

/* moduleFindAny()
 *     Finds a Module in the module_list, regardless of path.
 *
 * inputs     - file name
 * outputs    - pointer to Module or NULL on failure
 */
Module *
moduleFindAny(const char *filename)
{
    Module *module_p;
    DLinkNode *node_p;

    iassert(filename != NULL);

    DLINK_FOREACH(node_p, module_list.head)
    {
        module_p = node_p->data;

        if (!strcmp(module_p->filename, filename))
            return module_p;
    }

    return NULL;
}

/* moduleDelete()
 *     Deletes a Module from the module_list.
 *
 * inputs     - Module
 * outputs    - 1 on success or 0 on failure
 */
uchar
moduleDelete(Module * module_p)
{
    uchar ret;

    iassert(module_p != NULL);
    iassert(module_p->handle == NULL);

    ilog(L_DEBUG2, "moduleDelete(): Deleting %s/%s",
         module_p->modulepath_p->path, module_p->filename);

    ret = dlinkFindDestroy(module_p, &module_list);

    if (ret == 0)
    {
        ilog(L_DEBUG2, "moduleDelete(): Cannot find Module %s/%s",
             module_p->modulepath_p->path, module_p->filename);

        return 0;
    }

    ballocHeapFree(module_heap, module_p);

    return 1;
}

/* moduleFlush()
 *     Deletes all entries in the module_list.
 *
 * inputs     - none
 * outputs    - 1 on success, 0 on failure
 */
uchar
moduleFlush(void)
{
    Module *module_p;
    DLinkNode *node_p, *tnode_p;
    uchar ret = 0;

    ilog(L_DEBUG2, "moduleFlush(): Flushing module_list");

    DLINK_FOREACH_SAFE(node_p, tnode_p, module_list.head)
    {
        module_p = node_p->data;

        ret = moduleDelete(module_p);

        if (ret == 0)
        {
            ilog(L_DEBUG2, "moduleFlush(): moduleDelete() failed: %s/%s",
                 module_p->modulepath_p->path, module_p->filename);

            return 0;
        }
    }

    return 1;
}

/* modulePathAdd()
 *     Adds a ModulePath to the module_paths list.
 *
 * inputs     - path
 * outputs    - none
 */
void
modulePathAdd(const char *path)
{
    ModulePath *modulepath_p;

    iassert(path != NULL);

    ilog(L_DEBUG2, "modulePathAdd(): Adding %s", path);

    modulepath_p = icalloc(1, sizeof(ModulePath));
    strlcpy(modulepath_p->path, path, 1024);

    dlinkAddAlloc(modulepath_p, &module_paths);
}

/* modulePathFind()
 *     Finds a ModulePath in the module_paths list.
 *
 * inputs     - path
 * outputs    - ModulePath or NULL on failure
 */
ModulePath *
modulePathFind(const char *path)
{
    ModulePath *modulepath_p;
    DLinkNode *node_p;

    iassert(path != NULL);

    DLINK_FOREACH(node_p, module_paths.head)
    {
        modulepath_p = node_p->data;

        if (!strcmp(modulepath_p->path, path))
            return modulepath_p;
    }

    return NULL;
}

/* modulePathDelete()
 *     Deletes a ModulePath from the module_paths list.
 *
 * inputs     - ModulePath
 * outputs    - 1 on success or 0 on failure
 */
uchar
modulePathDelete(ModulePath * modulepath_p)
{
    uchar ret;

    iassert(modulepath_p != NULL);

    ilog(L_DEBUG2, "modulePathDelete(): Deleting %s", modulepath_p->path);

    ret = dlinkFindDestroy(modulepath_p, &module_paths);

    free(modulepath_p);

    return ret;
}

/* modulePathFlush()
 *     Deletes all ModulePaths from module_paths.
 *
 * inputs     - none
 * outputs    - 1 on success or 0 on failure
 */
uchar
modulePathFlush(void)
{
    DLinkNode *node_p, *tnode_p;
    uchar ret;

    DLINK_FOREACH_SAFE(node_p, tnode_p, module_paths.head)
    {
        ret = modulePathDelete((ModulePath *) node_p->data);

        if (ret == 0)
            return 0;
    }

    return 1;
}

/* moduleLoadAll()
 *     Loads all modules.
 *
 * inputs     - none
 * outputs    - 1 on success or 0 on failure
 */
uchar
moduleLoadAll(void)
{
    Module *module_p;
    DLinkNode *node_p;
    uchar ret;

    DLINK_FOREACH(node_p, module_list.head)
    {
        module_p = node_p->data;

        if (!strstr(module_p->modulepath_p->path, "autoload"))
            continue;

        ret = moduleLoad(module_p);

        if (ret == 0)
        {
            ilog(L_INFO, "moduleLoadAll(): moduleLoad() failed for %s/%s",
                 module_p->modulepath_p->path, module_p->filename);

            return 0;
        }
    }

    return 1;
}

/* moduleLoad()
 *     Loads a shared object into memory.
 *
 * inputs     - Module to load
 * outputs    - 1 on success or 0 on failure
 */
uchar
moduleLoad(Module * module_p)
{
    ModuleHeader *moduleheader_p;
    void *handle;
    uchar (*funcInit) ();
    void (*funcFini) ();
    const char *error;
    char path[(1024 + 256) + 1], header[512];
    uchar ret;

    iassert(module_p != NULL);

    ilog(L_DEBUG2, "moduleLoad(): Loading %s/%s",
         module_p->modulepath_p->path, module_p->filename);

    strlcpy(path, module_p->modulepath_p->path, 1280);
    strlcat(path, "/", 1280);
    strlcat(path, module_p->filename, 1280);

    handle = dlopen(path, RTLD_NOW);

    if (handle == NULL)
    {
        ilog(L_ERROR, "moduleLoad(): Error loading %s/%s: %s",
             module_p->modulepath_p->path, module_p->filename, dlerror());

        return 0;
    }

    /* Make sure we have a ModuleHeader. */
    strlcpy(header, module_p->name, 512);
    strlcat(header, "_header", 512);

    moduleheader_p = dlsym(handle, header);

    if ((error = dlerror()) != NULL)
    {
        ilog(L_ERROR, "moduleLoad(): Unable to find ModuleHeader for %s/%s: %s",
             module_p->modulepath_p->path, module_p->filename, error);

        dlclose(handle);

        return 0;
    }

    /* Make sure we have proper routines. */
    funcInit = moduleheader_p->moduleInit;

    if (funcInit == NULL)
    {
        ilog(L_ERROR, "moduleLoad(): Malformed ModuleHeader for %s/%s: no "
             "moduleInit specified",
             module_p->modulepath_p->path, module_p->filename);

        dlclose(handle);

        return 0;
    }

    funcFini = moduleheader_p->moduleFini;

    if (funcFini == NULL)
    {
        ilog(L_ERROR, "moduleLoad(): Malformed ModuleHeader for %s/%s: no "
             "moduleFini specified",
             module_p->modulepath_p->path, module_p->filename);

        dlclose(handle);

        return 0;
    }

    /* Make sure it initialises successfully. */
    ret = funcInit();

    if (ret == 0)
    {
        ilog(L_ERROR, "moduleLoad(): Initialisation routine failed for %s/%s",
             module_p->modulepath_p->path, module_p->filename);

        dlclose(handle);

        return 0;
    }

    /* Ok, it's loaded and ready. */
    module_p->handle = handle;
    module_p->moduleheader_p = moduleheader_p;

    dlinkAddAlloc(module_p, &modules_loaded);

    return 1;
}

/* moduleUnload()
 *     Unloads a shared object from memory.
 *
 * inputs     - Module to unload
 * outputs    - 1 on success or 0 on failure
 */
uchar
moduleUnload(Module * module_p)
{
    iassert(module_p != NULL);

    if (module_p->handle == NULL)
    {
        ilog(L_ERROR, "moduleUnload(): %s/%s isn't loaded.",
             module_p->modulepath_p->path, module_p->filename);

        return 0;
    }

    ilog(L_DEBUG2, "moduleUnload(): Unloading %s/%s",
         module_p->modulepath_p->path, module_p->filename);

    /* Call the cleanup routine. */
    module_p->moduleheader_p->moduleFini();

    module_p->handle = NULL;
    dlinkFindDestroy(module_p, &modules_loaded);

    return 1;
}
#endif /* STATIC_MODULES */
