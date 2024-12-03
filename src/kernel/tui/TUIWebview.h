/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef CO_TUI_WEBVIEW_H
#define CO_TUI_WEBVIEW_H



#include <QObject>
#include "TUIElement.h"

#ifdef USE_WEBENGINE
#include <QWebEngineView>
#include <QBoxLayout>
#endif


/** Basic Container
 * This class provides basic functionality and a
 * common interface to all Container elements.<BR>
 * The functionality implemented in this class represents a container
 * which arranges its children on top of each other.
 */
class TUIWebview : public QObject, public TUIElement
{
    Q_OBJECT

public:
    TUIWebview(int id, int type, QWidget *w, int parent, QString name); //(QObject* parent, const std::string& n, int pID)
    virtual ~TUIWebview();
    
    /// get the Element's classname
    const char *getClassName() const override;
    void setValue(TabletValue type, covise::TokenBuffer& tb) override;
public slots:
    void sendURL(bool);
#ifdef USE_WEBENGINE
    QWebEngineView* Webview;
    QHBoxLayout* WebviewLayout;
#endif
};
#endif
