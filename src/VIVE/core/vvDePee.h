/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */
/*
  Steffen Frey
  Fachpraktikum Graphik-Programmierung 2007
  Institut fuer Visualisierung und Interaktive Systeme
  Universitaet Stuttgart
 */

#ifndef _coVRDePee_H_
#define _coVRDePee_H_

#include <vsg/nodes/Node.h>
#include <osg/Camera>
#include <vsg/nodes/Group.h>
#include <osg/Texture2D>
#include <osgText/Text>
#include <string>
#include <stack>

#include "coVRDePeePass.h"

namespace vive
{
/*!
  The coVRDePee class is main class for setting up and managing depth peeling. 
  A coVRDePee object can be seen as a virtual node, that has one parent and one child. The rendering of every child and subchil of this child is managed by the the coVRDePee node. Besides that, it handles a head up display.
 */
class VVCORE_EXPORT coVRDePee : public osg::Referenced
{
public:
  /*!
    The constructor is initialized by giving it a parent and child node (subgraph), as well as the width and height in pixels of the output window. Additionally a subgraph can be added whose children aren't depth peeled but combined with de depth peeled scene
   */
  coVRDePee(vsg::Group* parent, vsg::Node* subgraph, unsigned width=100, unsigned height=100);
  /*!
    Takes care of clean removal of coVRDePee
   */
  ~coVRDePee();
  
  /*!
    The head up display shows information like internal status and current frames per second. This function needs to be called in the rendering loop to keep the information updated.
   */
  bool updateHUDText();

  /*!
    Sets whether sketchiness is activated or deactivated
   */
  void setSketchy(bool sketchy);
  
  /*!
    If sketchiness is enabled, sets whether a crayon should be used
   */
  void setCrayon(bool crayon);
  
  /*!
    Sets whether color display is activated or deactivated
   */
  void setColored(bool colored);
  
  /*!
    Sets whether edges are displayed or not
   */
  void setEdgy(bool edgy);

  /*!
    Sets how sketchy lines and colors should be displayed (standard is 1.0)
   */
  void setSketchiness(double sketchiness);
  
  /*!
    Set the pointer to the double variable containing the current fps for displaying it on the head up display
   */
  void setFPS(double* fps);

  /*!
    Add a depth peeling pass and adjust the render passes accordingly
   */
  bool addcoVRDePeePass();

  /*!
    Remove a depth peeling pass and adjust the render passes accordingly
   */
  bool remcoVRDePeePass();
  
private:
  /*!
    Create a map. This is a function for convenience and calls either 
    createNoiseMap(), createEdgeMap() or createNormalDepthColorMap().
    Apart from NOISE_MAP, for every texture generation 
    one rendering pass is needed.
    The boolean first is used to indicate whether this rendering pass
    belongs to the first depth peeling pass.
   */
  bool createMap(MapMode mapMode, bool first=false);

  /*!
    Creates a two dimensional noise map and initalizes _noiseMap with it
   */
  bool createNoiseMap();

  /*!
    Depending on the chosen MapMode, it either creates a new rendering
    pass for creaeting a normal, depth or color map. The created rendering
    pass is added to the current depth peeling pass.
   */
  bool createNormalDepthColorMap(MapMode mapMode, bool first);
  
  /*!
    Create an edge map. A previous depth and normal rendering pass in this 
    depth peeling pass is required for that.
   */
  bool createEdgeMap(bool first);
  
  /*!
    Creates the final rendering pass for depth peeling. Color and edge map are
    added up here and sketchiness is applied.
   */
  bool createFinal();
  
  /*!
    Create the rendering pass for the head up display
   */
  bool createHUD();

  /*
    Returns the number of rendering passes of the depth peeling object
   */
  unsigned int getNumberOfRenderPasses();
  
  /* Util methods
  */
  /*!
  Reads a file and returns a string
  */
  bool readFile(const char* fName, std::string& s);

  /*!
  Converts a number to a string
  */
  std::string toString(double d);

  /*!
  Create a osg shader program consisting of a vertex shader and a 
  fragment shader
  */
  osg::Program* createProgram(std::string vs, std::string fs);

  /*!
  This is a random generator to generate noise patterns.
  The returned values range from -1 to 1
  */
  double getNoise(unsigned x, unsigned y, unsigned random);

  /*!
  Returns a smoothed noise version of the value that is read from the noise
  texture
  */
  double smoothNoise(unsigned width, unsigned height, unsigned x, unsigned y, unsigned char* noise);

  /*!
  Creates a two dimensional color texture and apply some standard settings
  */
  osg::Texture2D* newColorTexture2D(unsigned width, unsigned height, unsigned accuracy);

  /*!
  Get a quad with screen size in order to show a texture full screen
  */
  osg::Geode* getCanvasQuad(unsigned width, unsigned height, double depth=-1);


  unsigned _texWidth;
  unsigned _texHeight;
  unsigned _width;
  unsigned _height;
  
  vsg::ref_ptr<vsg::Group> _parent;
  vsg::ref_ptr<vsg::Node> _subgraph;
  vsg::ref_ptr<osg::Texture2D> _noiseMap;
  vsg::ref_ptr<osg::Texture2D> _normalDepthMap0;
  vsg::ref_ptr<osg::Texture2D> _normalDepthMap1;
  
  vsg::ref_ptr<osg::Texture2D> _edgeMap;
  
  vsg::ref_ptr<osg::Texture2D> _colorMap;
  
  vsg::ref_ptr<osg::Geode> _quadGeode;

  osgText::Text* _hudText;
  double* _fps;
  
  std::vector<coVRDePeePass*> _coVRDePeePasses;
  
  osg::Uniform* _sketchy;
  osg::Uniform* _colored;
  osg::Uniform* _edgy;
  osg::Uniform* _sketchiness;
  
  bool _isSketchy;
  bool _isColored;
  bool _isEdgy;
  bool _isCrayon;

  osg::Camera* _colorCamera;

  //shader programs
  vsg::ref_ptr<osg::Program> _normalDepthMapProgram;
  vsg::ref_ptr<osg::Program> _colorMapProgram;
  vsg::ref_ptr<osg::Program> _edgeMapProgram;

  bool _renderToFirst;
};
}

#endif
