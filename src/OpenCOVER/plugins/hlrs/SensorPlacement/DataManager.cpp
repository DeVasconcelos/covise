#include<osg/Material>
#include<osg/LightModel>
#include<osg/StateSet>
#include <cover/coVRPluginSupport.h>

#include <algorithm>

#include "DataManager.h"
#include "Zone.h"
#include "Sensor.h"
#include "SensorPlacement.h"
#include "GA.h"

void setStateSet(osg::StateSet *stateSet)
{
    osg::Material *material = new osg::Material();
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE); 
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
}

std::vector<VisibilityMatrix<float>> convertVisMatTo2D(const VisibilityMatrix<float>& visMat)
{
    std::vector<int> sizes;
    for(const auto& zone : DataManager::GetSafetyZones())
    {
        sizes.push_back(zone->getNumberOfPoints());
    } 
    
    std::vector<VisibilityMatrix<float>> result;
    result.reserve(sizes.size());
    
    size_t startPos{0};
    size_t endPos;
    for(const auto& size : sizes)
    {
        endPos = startPos + size;
        VisibilityMatrix<float> temp = {visMat.begin() + startPos,visMat.begin() + endPos};
        result.push_back(std::move(temp));
        startPos += size;
    }
    
    return result;
}

DataManager::DataManager()
{
    m_Root = new osg::Group();
    m_Root->setName("SensorPlacement");
    m_Root->setNodeMask(m_Root->getNodeMask() & ~ 4096);
    osg::StateSet *mystateset = m_Root->getOrCreateStateSet();
    setStateSet(mystateset);
    cover->getObjectsRoot()->addChild(m_Root.get());
    std::cout<<"Singleton DataManager created!"<<std::endl;
};

void DataManager::preFrame()
{
    for(const auto& zone : GetInstance().m_SafetyZones)
    {
        bool status = zone->preFrame();
        if(!status)
        {
            GetInstance().RemoveZone(zone.get());
            return;
        }
        
    }

    for(const auto& zone : GetInstance().m_SensorZones)
    {
        bool status = zone->preFrame();
        if(!status)
        {
            GetInstance().RemoveZone(zone.get());
            return;
        } 
      
    }

    for(const auto& sensor : GetInstance().m_Sensors)
    {
        bool status = sensor->preFrame();
        if(!status)
        {
            GetInstance().RemoveSensor(sensor.get());
            return;
        }
    }

    for(const auto& udpSensor : GetInstance().m_UDPSensors)
    {
        bool status = udpSensor->preFrame();
    }
};
void DataManager::Destroy()
{
    GetInstance().m_Root->getParent(0)->removeChild(GetInstance().m_Root.get());

};

const std::vector<osg::Vec3> DataManager::GetWorldPosOfObervationPoints()
{
    std::vector<osg::Vec3> allPoints;
    size_t reserve_size {0};
    
    for(const auto& i : GetInstance().m_SafetyZones)
        reserve_size += i->getNumberOfPoints();

    if(!GetInstance().m_UDPSafetyZones.empty())
    {
        for(const auto& i : GetInstance().m_UDPSafetyZones)
            reserve_size += i->getNumberOfPoints();
    }

    allPoints.reserve(reserve_size);

    for(const auto& points :  GetInstance().m_SafetyZones)
    {
        auto vecWorldPositions = points->getWorldPositionOfPoints();
        allPoints.insert(allPoints.end(),vecWorldPositions.begin(),vecWorldPositions.end());
    }

    if(!GetInstance().m_UDPSafetyZones.empty())
    {
        for(const auto& points :  GetInstance().m_UDPSafetyZones)
        {
            auto vecWorldPositions = points->getWorldPositionOfPoints();
            allPoints.insert(allPoints.end(),vecWorldPositions.begin(),vecWorldPositions.end());
        }
    }

    return allPoints;
}

void DataManager::AddSafetyZone(upSafetyZone zone)
{   
    GetInstance().m_Root->addChild(zone.get()->getZone().get());
    GetInstance().m_SafetyZones.push_back(std::move(zone));  
}

void DataManager::AddSensorZone(upSensorZone zone)
{
    GetInstance().m_Root->addChild(zone.get()->getZone().get());
    GetInstance().m_SensorZones.push_back(std::move(zone));  
}

void DataManager::AddSensor(upSensor sensor)
{
    GetInstance().m_Root->addChild(sensor.get()->getSensor().get());
    GetInstance().m_Sensors.push_back(std::move(sensor));     
}


void DataManager::AddUDPSensor(upSensor sensor)
{
    GetInstance().m_Root->addChild(sensor.get()->getSensor().get());
    GetInstance().m_UDPSensors.push_back(std::move(sensor));     
}

void DataManager::AddUDPZone(upSafetyZone zone)
{
    GetInstance().m_Root->addChild(zone.get()->getZone().get());
    GetInstance().m_UDPSafetyZones.push_back(std::move(zone));  
}

void DataManager::AddUDPObstacle(osg::ref_ptr<osg::Node> node, const osg::Matrix& mat)
{
    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform(mat);
    mt->addChild(node);

    GetInstance().m_UDPObstacles.push_back(mt);
    GetInstance().m_Root->addChild(mt);
}

void DataManager::RemoveSensor(SensorPosition* sensor)
{  
    GetInstance().m_Root->removeChild(sensor->getSensor());

    GetInstance().m_Sensors.erase(std::remove_if(GetInstance().m_Sensors.begin(),GetInstance().m_Sensors.end(),[sensor](std::unique_ptr<SensorPosition>const& it){return sensor == it.get();}));  
}

void DataManager::RemoveUDPSensor(int pos)
{
    GetInstance().m_Root->removeChild(GetInstance().m_UDPSensors.at(pos)->getSensor());
    GetInstance().m_UDPSensors.erase(GetInstance().m_UDPSensors.begin() + pos);
}

void DataManager::RemoveZone(Zone* zone)
{
    GetInstance().m_Root->removeChild(zone->getZone());

    if(dynamic_cast<SensorZone*>(zone))
         GetInstance().m_SensorZones.erase(std::remove_if(GetInstance().m_SensorZones.begin(),GetInstance().m_SensorZones.end(),[zone](std::unique_ptr<SensorZone>const& it){return zone == it.get();}));
    else if(dynamic_cast<SafetyZone*>(zone))
        GetInstance().m_SafetyZones.erase(std::remove_if(GetInstance().m_SafetyZones.begin(),GetInstance().m_SafetyZones.end(),[zone](std::unique_ptr<SafetyZone>const& it){return zone == it.get();}));
}

void DataManager::RemoveUDPObstacle(int pos)
{
    GetInstance().m_Root->removeChild(GetInstance().m_UDPObstacles.at(pos));
    GetInstance().m_UDPObstacles.erase(GetInstance().m_UDPObstacles.begin() + pos);

}

void DataManager::RemoveUDPZone(int pos)
{
    GetInstance().m_Root->removeChild(GetInstance().m_UDPSafetyZones.at(pos)->getZone());
    GetInstance().m_UDPSafetyZones.erase(GetInstance().m_UDPSafetyZones.begin() + pos);
}

void DataManager::highlitePoints(const VisibilityMatrix<float>& visMat)
{
    auto visMat2D = convertVisMatTo2D(visMat);

    size_t count{0};
    for(const auto& zone : GetSafetyZones())
    {
        zone->highlitePoints(visMat2D.at(count));
        count ++;
    }

}

void DataManager::visualizeCoverage()
{
    std::vector<int> sensorsPerPoint(GetWorldPosOfObervationPoints().size(),0);
    std::vector<float> sumVisMat(GetWorldPosOfObervationPoints().size(),0.0f);

    for(const auto& sensor : DataManager::GetSensors())
    {
        std::transform(sensor->getVisibilityMatrix().begin(), sensor->getVisibilityMatrix().end(), sensorsPerPoint.begin(), sensorsPerPoint.begin(),[](float i, int j ) {return (i == 0.0f ? j : j+1);});  // count nbrOf sensors 
                                                                                              
        std::transform(sensor->getVisibilityMatrix().begin(), sensor->getVisibilityMatrix().end(), sumVisMat.begin(), sumVisMat.begin(), std::plus<float>());                                              // add coefficients 
    }

    for(const auto& zone : DataManager::GetSensorZones())
    {
        for(const auto& sensor : zone->getSensors())
        {
             std::transform(sensor->getVisibilityMatrix().begin(), sensor->getVisibilityMatrix().end(), sensorsPerPoint.begin(), sensorsPerPoint.begin(),[](float i, int j ) {return (i == 0.0f ? j : j+1);});  // count nbrOf sensors 
                                                                                              
            std::transform(sensor->getVisibilityMatrix().begin(), sensor->getVisibilityMatrix().end(), sumVisMat.begin(), sumVisMat.begin(), std::plus<float>());                                              // add coefficients
        }
    }

    std::vector<int> requiredSensorsPerPoint = calcRequiredSensorsPerPoint();
    std::vector<float> update;

    auto ItsensorsPerPoint = sensorsPerPoint.begin();
    auto ItRequiredSensors = requiredSensorsPerPoint.begin();
    while( ItsensorsPerPoint != sensorsPerPoint.end())
    {
        int distance = std::distance(sensorsPerPoint.begin(), ItsensorsPerPoint);
        int diff = *ItsensorsPerPoint - *ItRequiredSensors;         // difference between actual an required number of sensors

        if( diff >=0 && (sumVisMat.at(distance) / requiredSensorsPerPoint.at(distance) >= GA::s_VisibiltyThreshold )) 
            update.push_back(sumVisMat.at(distance));
        else
            update.push_back(0.0f);

        // increment iterators
        if( ItsensorsPerPoint != sensorsPerPoint.end())
        {
            ++ItsensorsPerPoint;
            ++ItRequiredSensors;
        }
    }


    auto visMat2D = convertVisMatTo2D(update);

    size_t count{0};
    osg::Vec4 notVisible{0.8,0.0,0,1};
    osg::Vec4 visible{0.0,0.4,0,1};

    for(const auto& zone : GetSafetyZones())
    {
        zone->highlitePoints(visMat2D.at(count),visible, notVisible);
        count ++;
    }
}                                   


void DataManager::updateFoV(float fov)
{
    for(const auto& sensor : GetInstance().m_Sensors)
        sensor->updateFoV(fov);
    
    for(const auto& zone : GetInstance().m_SensorZones)
        zone->updateFoV(fov);
}
void DataManager::updateDoF(float dof) 
{
    for(const auto& sensor : GetInstance().m_Sensors)
        sensor->updateDoF(dof);

    for(const auto& zone : GetInstance().m_SensorZones)
        zone->updateDoF(dof);   
}

void DataManager::UpdateAllSensors(std::vector<Orientation>& orientations)
{
    auto size =  GetInstance().m_Sensors.size();
    for(size_t incrementor = 0; incrementor < size; incrementor++)
        GetInstance().m_Sensors.at(incrementor)->setCurrentOrientation(orientations.at(incrementor));

    for(const auto& zone : GetInstance().m_SensorZones)
        zone->removeAllSensors();
    
    // create sensors in sensor zones
    int count{0};
    for(auto it = orientations.begin() + size; it != orientations.end(); ++it)
    {
        for(const auto& zone : GetInstance().m_SensorZones)
        {
            auto worldPositions = zone->getWorldPositionOfPoints();
            auto itFound = std::find_if(worldPositions.begin(), worldPositions.end(),[&it, &zone](const osg::Vec3& worldPos){
                                        if((it->getMatrix().getTrans() - worldPos).length2() < 0.00001) //they are equal
                                        {
                                            zone->createSensor(it->getMatrix());
                                            return true;
                                        }
                                        else
                                            return false;
                                        });
        }
    }
   
}

void DataManager::UpdateUDPSensorPosition(int pos, const osg::Matrix& mat)
{
    GetInstance().m_UDPSensors.at(pos)->setMatrix(mat);
};

void DataManager::UpdateUDPZone(int pos, const osg::Matrix& mat, int nbrOfSensors)
{
    GetInstance().m_UDPSafetyZones.at(pos)->setPosition(mat);
    GetInstance().m_UDPSafetyZones.at(pos)->setCurrentNbrOfSensors(nbrOfSensors);

}; 

void DataManager::UpdateUDPObstacle(int pos, const osg::Matrix& mat)
{
    GetInstance().m_UDPObstacles.at(pos)->setMatrix(mat);
};



void DataManager::setOriginalZoneColor()
{
    for(const auto& zone : GetSafetyZones())
        zone->setOriginalColor();
}

void DataManager::setPreviousZoneColor()
{
    for(const auto& zone : GetSafetyZones())
        zone->setPreviousZoneColor();
}


