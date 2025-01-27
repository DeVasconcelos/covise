/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

/************************************************************************
*									*
*          								*
*                            (C) 2001					*
*              Computer Centre University of Stuttgart			*
*                         Allmandring 30				*
*                       D-70550 Stuttgart				*
*                            Germany					*
*									*
*									*
*	File			vvMousePointer.cpp (Performer 2.0)	*
*									*
*	Description		Mouse support for COVER
*									*
*	Author		Uwe Woessner				*
*									*
*	Date			19.09.2001				*
*									*
*	Status			none					*
*									*
************************************************************************/

#include <math.h>
#include <OpenVRUI/vsg/mathUtils.h>
#include <OpenVRUI/sginterface/vruiButtons.h>
#include "vvViewer.h"
#include "vvPluginSupport.h"
#include "vvConfig.h"

#include "vvMousePointer.h"
#include "buttondevice.h"
#include "trackingbody.h"
#include "input.h"

using namespace vive;

/*______________________________________________________________________*/
vvMousePointer::vvMousePointer()
{
    if (vv->debugLevel(2))
        fprintf(stderr, "new vvMousePointer\n");

    buttons = Input::instance()->getButtons("Mouse");
    body = Input::instance()->getBody("Mouse");

    wheelCounter[0] = wheelCounter[1] = newWheelCounter[0] = newWheelCounter[1] = 0;

    mouseTime = vv->frameRealTime();
    mouseButtonTime = mouseTime - 1.0;
    mouseX = mouseY = 0;

    width = 1;
    height = 1;
    screenX = 0.;
    screenY = 0.;
    screenZ = 0.;
    screenH = 0.;
    screenP = 0.;
    screenR = 0.;

    if (vvConfig::instance()->numWindows() > 0 && vvConfig::instance()->numScreens() > 0)
    {
        width = vvConfig::instance()->screens[0].hsize;
        height = vvConfig::instance()->screens[0].vsize;
        if ((vvConfig::instance()->viewports[0].viewportXMax - vvConfig::instance()->viewports[0].viewportXMin) == 0)
        {
            xres = vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].sx;
            yres = vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].sy;
        }
        else
        {
            xres = (int)(vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].sx * (vvConfig::instance()->viewports[0].viewportXMax - vvConfig::instance()->viewports[0].viewportXMin));
            yres = (int)(vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].sy * (vvConfig::instance()->viewports[0].viewportYMax - vvConfig::instance()->viewports[0].viewportYMin));
        }

        //xres=vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].sx;
        //yres=vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].sy;
        xori = vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].ox;
        yori = vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].oy;

        //fprintf(stderr,"width: %f, height %f, xres %d , yres %d, xori %d, yori %d\n", width, height, xres,yres,xori,yori);

        screenX = vvConfig::instance()->screens[0].xyz[0];
        screenY = vvConfig::instance()->screens[0].xyz[1];
        screenZ = vvConfig::instance()->screens[0].xyz[2];
        screenH = vvConfig::instance()->screens[0].hpr[0];
        screenP = vvConfig::instance()->screens[0].hpr[1];
        screenR = vvConfig::instance()->screens[0].hpr[2];
    }
}

/*______________________________________________________________________*/
vvMousePointer::~vvMousePointer()
{
}

double vvMousePointer::eventTime() const
{
    return mouseTime;
}

void vvMousePointer::queueEvent(int type, int state, int code)
{
    MouseEvent me = { type, state, code };
    std::cerr << "queueEvent " << type << " " << state << " " << code << std::endl;
    eventQueue.push_back(me);
}

void vvMousePointer::processEvents()
{
    while (!eventQueue.empty())
    {
        MouseEvent me = eventQueue.front();
        eventQueue.pop_front();
        handleEvent(me.type, me.state, me.code, false);
     /*   if (me.type == osgGA::GUIEventAdapter::PUSH
            || me.type == osgGA::GUIEventAdapter::RELEASE
            || me.type == osgGA::GUIEventAdapter::SCROLL)
            break;*/
    }
}

void vvMousePointer::handleEvent(int type, int state, int code, bool queue)
{
    mouseTime = vv->frameRealTime();

/*    if (queue && !eventQueue.empty())
    {
        queueEvent(type, state, code);
        return;
    }

    switch(type)
    {
    case osgGA::GUIEventAdapter::DRAG:
        mouseX = state;
        mouseY = code;
        break;
    case osgGA::GUIEventAdapter::MOVE:
        mouseX = state;
        mouseY = code;
        break;
    case osgGA::GUIEventAdapter::SCROLL:
        if (!buttonPressed)
        {
            if (state == osgGA::GUIEventAdapter::SCROLL_UP)
                ++newWheelCounter[0];
            else if (state == osgGA::GUIEventAdapter::SCROLL_DOWN)
                --newWheelCounter[0];
            else if (state == osgGA::GUIEventAdapter::SCROLL_RIGHT)
                ++newWheelCounter[1];
            else if (state == osgGA::GUIEventAdapter::SCROLL_LEFT)
                --newWheelCounter[1];
        }
        break;
    case osgGA::GUIEventAdapter::PUSH:
        buttonPressed = bool(state);
        if (mouseTime == mouseButtonTime)
            queueEvent(type, state, code);
        else
        {
            buttons->setButtonState(state, true);
            mouseButtonTime = vv->frameRealTime();
        }
        break;
    case osgGA::GUIEventAdapter::RELEASE:
        if (mouseTime == mouseButtonTime)
            queueEvent(type, state, code);
        else
        {
            buttons->setButtonState(state, true);
            mouseButtonTime = vv->frameRealTime();
        }
        buttonPressed = bool(state);
        break;
    case osgGA::GUIEventAdapter::DOUBLECLICK:
        handleEvent(osgGA::GUIEventAdapter::PUSH, state, code, queue);
        handleEvent(osgGA::GUIEventAdapter::RELEASE, state, code, true);
        break;
    }*/
}

/*______________________________________________________________________*/
void
vvMousePointer::update()
{
    if (vv->debugLevel(5))
        fprintf(stderr, "vvMousePointer::update\n");

    if (vvConfig::instance()->numWindows() <= 0 || vvConfig::instance()->numScreens() <= 0)
        return;

    unsigned state = buttons->getButtonState();
    state &= ~(vrui::vruiButtons::WHEEL);
    buttons->setButtonState(state);

    processEvents();

    wheelCounter[0] = newWheelCounter[0];
    wheelCounter[1] = newWheelCounter[1];
    newWheelCounter[0] = newWheelCounter[1] = 0;

    static int oldWidth = -1, oldHeight = -1;
    int currentW, currentH;
 /*   const osg::GraphicsContext::Traits* traits = NULL;
    if (vvConfig::instance()->windows[0].window)
        traits = vvConfig::instance()->windows[0].window->getTraits();
    if (!traits)
        return;

    currentW = traits->width;
    currentH = traits->height;
    if (oldWidth != currentW || oldHeight != currentH)
    {
        vvConfig::instance()->windows[0].sx = currentW;
        vvConfig::instance()->windows[0].sy = currentH;
        oldWidth = currentW;
        oldHeight = currentH;
        if ((vvConfig::instance()->viewports[0].viewportXMax - vvConfig::instance()->viewports[0].viewportXMin) == 0)
        {
            xres = vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].sx;
            yres = vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].sy;
        }
        else
        {
            xres = (int)(vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].sx * (vvConfig::instance()->viewports[0].viewportXMax - vvConfig::instance()->viewports[0].viewportXMin));
            yres = (int)(vvConfig::instance()->windows[vvConfig::instance()->viewports[0].window].sy * (vvConfig::instance()->viewports[0].viewportYMax - vvConfig::instance()->viewports[0].viewportYMin));
        }
    }

    float mx = x();
    float my = y();
    float wc = 0.0;
    // mouse coordinates are relative to original window size, even if window has been resized... so don´t use current window size here.
    // but physical size might have been adjusted, if aspect ratio changed
    width = vvConfig::instance()->screens[0].hsize;
    height = vvConfig::instance()->screens[0].vsize;
    if(vvConfig::instance()->channels[0].stereoMode == osg::DisplaySettings::HORIZONTAL_SPLIT)
    {
        mx *=2;
        if(mx > xres)
            mx -= xres;
    }
    if(vvConfig::instance()->channels[0].stereoMode == osg::DisplaySettings::VERTICAL_SPLIT)
    {
        my *=2;
        if(my > yres)
            my -= yres;
    }

    vsg::vec3 mouse2D;
    vsg::vec3 mouse3D;
    vsg::dmat4 transMat;

    mouse2D[0] = ((mx - (xres / 2.0)) / xres) * width;
    if(mouse2D[0] > width/2.0) // work around for twho viewports in one window
    {
        mouse2D[0] -= width;
    }
    mouse2D[1] = 0;
    mouse2D[2] = (my / yres - (0.5)) * height;
    if(mouse2D[2] > height/2.0)// work around for twho viewports in one window
    {
        mouse2D[2] -= height;
    }

    MAKE_EULER_MAT(transMat, screenH, screenP, screenR);
    mouse3D = transMat.preMult(mouse2D);
    transMat= vsg::translate(screenX, screenY, screenZ);
    mouse3D = transMat.preMult(mouse3D);
    transMat = rotate(vvConfig::instance()->worldAngle(), osg::X_AXIS);
    mouse3D = transMat.preMult(mouse3D);
    //cerr << mx << " , " << my << endl;
    //cerr << " 3D:" << mouse3D[0] << " , " << mouse3D[1] << " , " << mouse3D[2] << " , "<< endl;
    //cerr << " 2D:" << mouse2D[0] << " , " << mouse2D[1] << " , " << mouse2D[2] << " , "<< endl;

    vsg::vec3 YPos(0, 1, 0);
    vsg::vec3 direction;
    vsg::vec3 viewerPos = vvViewer::instance()->getViewerPos();

    // if orthographic projection: project viewerPos onto screenplane-normal passing through mouse3D
    if (vvConfig::instance()->orthographic())
    {
        vsg::vec3 normal;
        MAKE_EULER_MAT(transMat, screenH, screenP, screenR);
        normal = transMat.preMult(vsg::vec3(0.0, 1.0, 0.0));
        transMat = rotate(vvConfig::instance()->worldAngle(), osg::X_AXIS);
        normal = transMat.preMult(normal);
        normal.normalize();
        vsg::vec3 projectionOntoNormal = normal * (normal * (viewerPos - mouse3D));
        viewerPos = mouse3D + projectionOntoNormal;
    }

    direction = mouse3D - viewerPos;
    direction.normalize();
    vsg::dmat4 mat;
    mat = rotate(YPos, direction);
    mat.rotate(osg::inDegrees(90.0), vsg::vec3(0.0, 1.0, 0.0));
    vsg::dmat4 tmp;
    tmp= vsg::translate(direction[0] * wc, direction[1] * wc, direction[2] * wc);
    mat.postMult(tmp);

    tmp= vsg::translate(viewerPos[0], viewerPos[1], viewerPos[2]);
    mat.postMult(tmp);
    body->setMat(mat);*/

    //cerr << " VP:" << viewerPos[0] << " , "<< viewerPos[1] << " , "<< viewerPos[2] << " , "<< endl;
    //mouse3D /=10; // umrechnung in cm (scheisse!!!!!!)
    //matrix.makeTrans(mouse3D[0],mouse3D[1],mouse3D[2]);
}

float vvMousePointer::x() const
{
    return mouseX;
}

float vvMousePointer::y() const
{
    return mouseY;
}

float vvMousePointer::winWidth() const
{
    return (float)xres;
}

float vvMousePointer::winHeight() const
{
    return (float)yres;
}

float vvMousePointer::screenWidth() const
{
    return width;
}

float vvMousePointer::screenHeight() const
{
    return height;
}

const vsg::dmat4 &vvMousePointer::getMatrix() const
{
    return body->getMat();
}

#if 0
void vvMousePointer::setMatrix(const vsg::dmat4 &mat)
{

    matrix = mat;
}
#endif

int vvMousePointer::wheel(size_t num) const
{

    if (num >= 2)
        return 0;

    return wheelCounter[num];
}

unsigned int vvMousePointer::buttonState() const
{

    return buttons->getButtonState();
}
