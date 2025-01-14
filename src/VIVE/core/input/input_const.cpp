/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include "input_const.h"

namespace vive
{

ConstInputDevice::ConstInputDevice(const std::string &name)
    : InputDevice(name)
{
    vsg::dmat4 mat = vsg::dmat4::identity();
    m_bodyMatrices.push_back(mat);
    m_bodyMatrices.push_back(mat);
    m_bodyMatricesValid.push_back(true);
    m_bodyMatricesValid.push_back(true);

    m_isVarying = false;
    m_is6Dof = false;
}

bool ConstInputDevice::needsThread() const
{

    return false;
}
}
