#include <vector>
#include <osg/Vec3>
#include "coVRPlugin.h"

class SplotchAvatar : public coVRPlugin, public Owner {
public:
    SplotchAvatar();
    virtual ~SplotchAvatar();

    void preFrame() override;

private:
    std::vector<osg::Vec3> splotchPositions; // Dynamic splotch positions
    std::vector<int> splotchType;            // Persistent splotch types (0=starry, 1=water, 2=forest)
    float splotchRadius = 0.5f;              // Splotch proximity radius

    osg::Vec3 getAvatarPosition() const;
    int getPlaneType(const osg::Vec3 &pos) const;
    bool isNearExistingSplotch(const osg::Vec3 &pos) const;
};