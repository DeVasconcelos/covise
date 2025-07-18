#ifndef COVER_PLUGIN_SPLOTCH_COLOR_PLUGIN_H
#define COVER_PLUGIN_SPLOTCH_COLOR_PLUGIN_H

#include <vector>
#include <osg/Vec3>

#include <cover/coVRPlugin.h>
#include <cover/coVRPluginSupport.h>
#include <cover/ui/Owner.h>


class SplotchAvatar : public opencover::coVRPlugin, public opencover::ui::Owner
{
public:
    SplotchAvatar();
    virtual void preFrame() override;
    osg::Vec3 getAvatarPosition() const;

private:
    static constexpr int maxSplotches = 9;
    std::vector<osg::Vec4> splotchPositions; 

    float splotchRadius = 0.2f; 
    float prevZ = -23.0f; // previous z position of the avatar
    bool isNearExistingSplotch(const osg::Vec3 &pos) const;
    int getPlaneType(const osg::Vec3 &pos) const;
};
#endif // COVER_PLUGIN_SPLOTCH_COLOR_PLUGIN_H