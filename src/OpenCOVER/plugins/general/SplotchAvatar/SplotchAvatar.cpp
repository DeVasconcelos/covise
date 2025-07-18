#include <iostream>
#include <string>

#include <cover/VRSceneGraph.h>
#include <cover/coVRShader.h>
#include <osg/Timer>

#include "SplotchAvatar.h"

using namespace covise;
using namespace opencover;
using namespace ui;

SplotchAvatar::SplotchAvatar() : coVRPlugin(COVER_PLUGIN_NAME), Owner(COVER_PLUGIN_NAME, cover->ui)
{
    // Nothing to initialize for dynamic splotches
}

// Add member variable for timing
static double lastSplotchTime = 0.0;

int SplotchAvatar::getPlaneType(const osg::Vec3 &pos) const
{
    if (pos.z() > -23.0 && pos.z() < -9.0)
        return 0; // starry
    else if (pos.z() > -7.0 && pos.z() < 6.5)
        return 1; // water
    else if (pos.z() > 9.5 && pos.z() < 23.0)
        return 2; // forest
    return -1;    // not on a plane
}

bool SplotchAvatar::isNearExistingSplotch(const osg::Vec3 &pos) const
{
    for (const auto &splotchPos : splotchPositions)
    {
        if ((pos - splotchPos).length() < splotchRadius)
            return true;
    }
    return false;
}

osg::Vec3 SplotchAvatar::getAvatarPosition() const
{
    osg::Node *node = VRSceneGraph::instance()->findFirstNode<osg::Node>("WaterGhostMesh");
    if (!node)
        return osg::Vec3(0, 0, 0); // Not found

    osg::MatrixTransform *mt = dynamic_cast<osg::MatrixTransform *>(node);
    if (mt)
        return mt->getMatrix().getTrans();

    // If not a MatrixTransform, try to get world coordinates
    osg::Matrix worldMat = osg::computeLocalToWorld(node->getParentalNodePaths()[0]);
    return worldMat.getTrans();
}

bool areaActive[4] = {false, false, false, false};

void SplotchAvatar::preFrame()
{
    osg::Vec3 avatarPos = getAvatarPosition();
    int planeType = getPlaneType(avatarPos);

    // Activate area if avatar is above a plane
    /*std::cout << "Plane type: " << planeType << std::endl;
    if (planeType >= 0 && planeType < 3)
        areaActive[planeType] = true;
    else if (planeType == -1)
        areaActive[3] = true; // "none" area*/

    for (int i = 0; i < 4; ++i)
        areaActive[i] = true;

    osg::Vec4 areaActiveVec(
        areaActive[0] ? 1.0f : 0.0f,
        areaActive[1] ? 1.0f : 0.0f,
        areaActive[2] ? 1.0f : 0.0f,
        areaActive[3] ? 1.0f : 0.0f);

    osg::Node *node = VRSceneGraph::instance()->findFirstNode<osg::Node>("WaterGhostMesh");
    osg::ref_ptr<osg::Uniform> areaActiveUniform = new osg::Uniform("areaActive", areaActiveVec);
    node->getOrCreateStateSet()->addUniform(areaActiveUniform.get(), osg::StateAttribute::ON);
}

COVERPLUGIN(SplotchAvatar);
