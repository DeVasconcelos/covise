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
        float dx = pos.x() - splotchPos.x();
        float dy = pos.y() - splotchPos.y();
        if (dx * dx + dy * dy < splotchRadius * splotchRadius)
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

    // Reset colors if z changes sign from + to -
    if (this->prevZ > 0.0f && avatarPos.z() < 0.0f)
    {
        splotchPositions.clear();
    }
    this->prevZ = avatarPos.z();

    // Example: map avatar z to Texcoord.y
    float minZ = -23.0f, maxZ = 23.0f;
    float v = (avatarPos.z() - minZ) / (maxZ - minZ); // Texcoord.y in [0,1]
    float u = 0.5f;                                   // Center x

    if (planeType >= 0 && planeType < 3)
    {
        // Only add if not near an existing splotch
        osg::Vec4 splotch(u, v, float(planeType), 0.0f);
        bool alreadyThere = false;
        for (const auto &sp : splotchPositions)
            if ((sp.x() - u) * (sp.x() - u) + (sp.y() - v) * (sp.y() - v) < splotchRadius * splotchRadius)
                alreadyThere = true;
        if (!alreadyThere)
            splotchPositions.push_back(splotch);
    }

    osg::Node *node = VRSceneGraph::instance()->findFirstNode<osg::Node>("WaterGhostMesh");
    
    osg::ref_ptr<osg::Uniform> splotchPositionsUniform = new osg::Uniform(osg::Uniform::FLOAT_VEC4, "splotchPositions", splotchPositions.size());
    for (size_t i = 0; i < splotchPositions.size(); ++i)
        splotchPositionsUniform->setElement(i, splotchPositions[i]);
    osg::ref_ptr<osg::Uniform> numSplotchesUniform = new osg::Uniform("numSplotches", int(splotchPositions.size()));
    osg::ref_ptr<osg::Uniform> splotchRadiusUniform = new osg::Uniform("splotchRadius", splotchRadius);

    node->getOrCreateStateSet()->addUniform(splotchPositionsUniform.get(), osg::StateAttribute::ON);
    node->getOrCreateStateSet()->addUniform(numSplotchesUniform.get(), osg::StateAttribute::ON);
    node->getOrCreateStateSet()->addUniform(splotchRadiusUniform.get(), osg::StateAttribute::ON);
}

COVERPLUGIN(SplotchAvatar);
