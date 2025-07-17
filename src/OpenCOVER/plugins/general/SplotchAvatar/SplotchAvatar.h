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
    static constexpr int maxSplotches = 9; // maximum number of splotches
    std::vector<osg::Vec3> splotchPositions; // positions of collected splotches
    std::vector<int> splotchType; // type of each splotch (0=starry, 1=water, 2=forest)
    float splotchRadius = 2.0f; // increased for visibility
    bool isNearExistingSplotch(const osg::Vec3 &pos) const;
    int getPlaneType(const osg::Vec3 &pos) const;
};
#endif // COVER_PLUGIN_SPLOTCH_COLOR_PLUGIN_H