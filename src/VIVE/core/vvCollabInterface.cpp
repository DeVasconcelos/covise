/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include "vvCollabInterface.h"
#include "vvPluginSupport.h"

using namespace covise;
using namespace vive;
/** Constructor.
  @param m           pointer to the current plugin or NULL if not within a plugin
*/
coCOIM::coCOIM(vvPlugin *m)
{
    myPlugin = m;
}

/// Destructor.
coCOIM::~coCOIM()
{
}

/// get the pointer to the plugin
vvPlugin *coCOIM::getPlugin()
{
    return myPlugin;
}

/// @param m pointer to the plugin
void coCOIM::setPlugin(vvPlugin *m)
{
    myPlugin = m;
}
