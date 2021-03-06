#  praxis: services for TSora IRC networks.
#  modules/static_modules.sh: Generates static module information.
#
#  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
#  Copyright (c) 2003-2004 shrike development team.
#
#  $Id$

SYMS=`for x in $*; do basename $x .o|sed -es/^m_//; done`
cat > static_modules.c <<EOF
/*  praxis: services for TSora IRC networks.
 *  modules/static_modules.c: Autogenerated static module information.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 */

#include "praxis.h"
#include "ilog.h"

uchar moduleLoadStatic(void);

EOF

for x in $SYMS; do
        echo extern ModuleHeader "$x"_header\;
done >> static_modules.c

echo extern ModuleHeader userserv_header\; >> static_modules.c


echo ModuleHeader *module_headers[] = { >> static_modules.c
for x in $SYMS; do
        echo \&"$x"_header,
done >> static_modules.c

echo \&userserv_header, >> static_modules.c

echo NULL }\; >> static_modules.c

cat >> static_modules.c <<EOF
/* moduleLoadStatic()
 *     Calls static module initialisation routines.
 *
 * inputs     - none
 * outputs    - 1 on success or 0 on failure
 */
uchar
moduleLoadStatic(void)
{
    uint i;
    uchar ret;

    for (i = 0; module_headers[i] != NULL; i++)
    {
        ilog(L_DEBUG2, "moduleLoadStatic(): Initialising %s",
             module_headers[i]->name);

        ret = module_headers[i]->moduleInit();

        if (ret == 0)
        {
            ilog(L_INFO, "moduleLoadStatic(): Static module initialisation "
                 "failure for %s", module_headers[i]->name);

            return 0;
        }
    }

    return 1;
}
EOF
