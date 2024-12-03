/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <assert.h>
#include <stdio.h>

#include "qtcolortriangle.h"
#include "TUIColorTriangle.h"
#include "TUIMain.h"
#include <net/tokenbuffer.h>

/// Constructor
TUIColorTriangle::TUIColorTriangle(int id, int type, QWidget *w, int parent, QString name)
    : TUIElement(id, type, w, parent, name)
{
    colorTriangle = createWidget<QtColorTriangle>(w);

    connect(colorTriangle, SIGNAL(colorChanged(const QColor &)), this, SLOT(changeColor(const QColor &)));
    connect(colorTriangle, SIGNAL(released(const QColor &)), this, SLOT(releaseColor(const QColor &)));
}

void TUIColorTriangle::changeColor(const QColor &col)
{
    red = ((float)col.red()) / 255.0;
    green = ((float)col.green()) / 255.0;
    blue = ((float)col.blue()) / 255.0;
    alpha = ((float)col.alpha()) / 255.0;
    covise::TokenBuffer tb;
    tb << ID;
    tb << TABLET_RGBA;
    tb << TABLET_PRESSED;
    tb << red;
    tb << green;
    tb << blue;
    tb << alpha;

    TUIMain::getInstance()->send(tb);
}

void TUIColorTriangle::releaseColor(const QColor &col)
{
    red = ((float)col.red()) / 255.0;
    green = ((float)col.green()) / 255.0;
    blue = ((float)col.blue()) / 255.0;
    alpha = ((float)col.alpha()) / 255.0;
    covise::TokenBuffer tb;
    tb << ID;
    tb << TABLET_RGBA;
    tb << TABLET_RELEASED;
    tb << red;
    tb << green;
    tb << blue;
    tb << alpha;

    TUIMain::getInstance()->send(tb);
}

void TUIColorTriangle::setValue(TabletValue type, covise::TokenBuffer &tb)
{

    if (type == TABLET_RED)
    {
        tb >> red;
    }
    else if (type == TABLET_GREEN)
    {
        tb >> green;
    }
    else if (type == TABLET_BLUE)
    {
        tb >> blue;
    }
    int r = (int)(red * 255);
    int g = (int)(green * 255);
    int b = (int)(blue * 255);
    int a = (int)(alpha * 255);

    /*if(r>255) r = 255;
   if(g>255) g = 255;
   if(b>255) b = 255;
   if(r<0) r = 0;
   if(g<0) g = 0;
   if(b<0) b = 0;*/

    QColor col(r, g, b, a);

    colorTriangle->setColor(col);

    TUIElement::setValue(type, tb);
}

const char *TUIColorTriangle::getClassName() const
{
    return "TUIColorTriangle";
}
