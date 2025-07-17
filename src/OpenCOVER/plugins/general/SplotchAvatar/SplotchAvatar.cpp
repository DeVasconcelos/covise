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

void SplotchAvatar::preFrame()
{
    osg::Vec3 avatarPos = getAvatarPosition();
    int planeType = getPlaneType(avatarPos);
    double currentTime = osg::Timer::instance()->time_s();
    if (planeType != -1 && !isNearExistingSplotch(avatarPos) &&
        (currentTime - lastSplotchTime > 1.0) &&
        splotchPositions.size() < maxSplotches)
    {
        // Only z changes, x and y are fixed
        float v = (avatarPos.z() + 23) / (46); // v in [0,1]
        splotchPositions.push_back(osg::Vec3(0.99, v, 0));
        std::cout <<  "v"  << v << std::endl;

        splotchType.push_back(planeType);
        lastSplotchTime = currentTime;
    }

    // Pass splotchPositions and splotchType arrays to shader
    osg::ref_ptr<osg::Uniform> splotchPosUniform = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "splotchPositions", splotchPositions.size());
    for (size_t i = 0; i < splotchPositions.size(); ++i)
        splotchPosUniform->setElement(static_cast<unsigned int>(i), splotchPositions[i]);
    coVRShaderList::instance()->addGlobalUniform("splotchPositions", splotchPosUniform.get());

    osg::ref_ptr<osg::Uniform> splotchTypeUniform = new osg::Uniform(osg::Uniform::INT, "splotchType", splotchType.size());
    for (size_t i = 0; i < splotchType.size(); ++i)
        splotchTypeUniform->setElement(static_cast<unsigned int>(i), splotchType[i]);
    coVRShaderList::instance()->addGlobalUniform("splotchType", splotchTypeUniform.get());

    osg::ref_ptr<osg::Uniform> numSplotchesUniform = new osg::Uniform("numSplotches", static_cast<int>(splotchPositions.size()));
    coVRShaderList::instance()->addGlobalUniform("numSplotches", numSplotchesUniform.get());

/*     std::cout << "numSplotches: " << splotchPositions.size() << std::endl;
    for (size_t i = 0; i < splotchPositions.size(); ++i)
    {
        std::cout << "Splotch " << i << ": pos=("
                  << splotchPositions[i].x() << ", "
                  << splotchPositions[i].y() << ", "
                  << splotchPositions[i].z() << "), type=" << splotchType[i] << std::endl;
    } */
}

COVERPLUGIN(SplotchAvatar);
