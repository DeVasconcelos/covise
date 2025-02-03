/*
  Steffen Frey
  Fachpraktikum Graphik-Programmierung 2007
  Institut fuer Visualisierung und Interaktive Systeme
  Universitaet Stuttgart
 */

#include "coVRDePeeUtility.h"

#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/FileUtils>
#include <osgDB/fstream>

bool coVRDePeeUtility::readFile(const char* fName, std::string& s)
{
  std::string foundFile = osgDB::findDataFile(fName);
  if (foundFile.empty()) return false;

  osgDB::ifstream is;//(fName);
  is.open(foundFile.c_str());
  if (is.fail())
    {
      std::cerr << "Could not open " << fName << " for reading.\n";
      return false;
    }
  char ch = is.get();
  while (!is.eof())
    {
      s += ch;
      ch = is.get();
    }
  is.close();
  return true;
}

std::string coVRDePeeUtility::toString(double d)
{
  std::stringstream ostr;
  ostr << d;
  return ostr.str();
}

osg::Program* coVRDePeeUtility::createProgram(std::string vs, std::string fs)
{
  //setup shader
  std::string vertSource;
  if(!readFile((char*)vs.c_str(), vertSource))
    {
      printf("shader source not found\n");
      return 0;
    }

  std::string fragSource;
  if(!readFile((char*)fs.c_str(), fragSource))
    {
      printf("shader source not found\n");
      return 0;
    }


  osg::Program* program = new osg::Program;
  program->addShader( new osg::Shader( osg::Shader::VERTEX, vertSource.c_str() ) );
  program->addShader( new osg::Shader( osg::Shader::FRAGMENT, fragSource.c_str() ) );
  return program;
}

double coVRDePeeUtility::getNoise(unsigned x, unsigned y, unsigned random)
{
  int n = x + y * 57 + random * 131;
  n = (n<<13) ^ n;
  double noise = (1.0f - ( (n * (n * n * 15731 + 789221) +
                1376312589)&0x7fffffff)* 0.000000000931322574615478515625f);
  return noise;
}

double coVRDePeeUtility::smoothNoise(unsigned width, unsigned height, unsigned x, unsigned y, unsigned char* noise)
{
  assert(noise);

  if(x==0 || x > width -2
     || y==0 || y > height -2)
    return noise[x + y*width];

  double corners = (noise[x-1 + (y-1) *width]
            +noise[x+1 + (y-1)*width]
            +noise[x-1 + (y+1) * width]
            +noise[x+1 + (y+1) * width]) / 16.0;
  double sides   = (noise[x-1 + y*width]
            +noise[x+1 + y*width]
            +noise[x + (y-1)*width]
            +noise[x + (y+1)*width]) / 8.0;
  double center  =  noise[x + y*width] / 4.0;

  return corners + sides + center;
}

osg::Texture2D* coVRDePeeUtility::newColorTexture2D(unsigned width, unsigned height, unsigned accuracy)
{
  osg::Texture2D* texture2D = new osg::Texture2D;

  texture2D->setTextureSize(width, height);
  if(accuracy == 32)
    {
      texture2D->setInternalFormat(GL_RGBA32F_ARB);
      texture2D->setSourceFormat(GL_RGBA);
    }
  else if(accuracy == 8)
    {
      texture2D->setInternalFormat(GL_RGBA);
    }
  texture2D->setSourceType(GL_FLOAT);
  texture2D->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
  texture2D->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
  return texture2D;
}

osg::Geode* coVRDePeeUtility::getCanvasQuad(unsigned width, unsigned height, double depth)
{
  vsg::vec3Array* vertices = new vsg::vec3Array;
  osg::Vec2Array* texCoords = new osg::Vec2Array;
  vertices->push_back(vsg::vec3(0,0,depth));
  texCoords->push_back(osg::Vec2(0,0));

  vertices->push_back(vsg::vec3(width,0,depth));
  texCoords->push_back(osg::Vec2(1,0));

  vertices->push_back(vsg::vec3(0,height,depth));
  texCoords->push_back(osg::Vec2(0,1));

  vertices->push_back(vsg::vec3(width,height,depth));
  texCoords->push_back(osg::Vec2(1,1));

  vsg::Node* quad = new vsg::Node;
  quad->setVertexArray(vertices);
  quad->setTexCoordArray(1,texCoords);

  quad->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP,0,vertices->size()));

  vsg::vec4Array* colors = new vsg::vec4Array;
  colors->push_back(vsg::vec4(1.0f,1.0f,1.0f,1.0f));
  quad->setColorArray(colors, osg::Array::BIND_OVERALL);

  osg::Geode* geode = new osg::Geode();
  geode->addDrawable(quad);

  return geode;

}
