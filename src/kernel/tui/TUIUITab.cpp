/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include "TUIUITab.h"

#include <sstream>
#include <iostream>
#include <QLayout>
#include <net/tokenbuffer.h>

TUIUITab::TUIUITab(int id, int type, QWidget *w, int parent, QString name)
    : TUITab(id, type, w, parent, name)
    , uiWidget(0)
{
    std::cerr << "TUIUITab::<init> info: creating..." << std::endl;
}

TUIUITab::~TUIUITab()
{
    std::cerr << "TUIUITab::<dest> info: destroying..." << std::endl;
}

const char *TUIUITab::getClassName() const
{
    return "TUIUITab";
}

void TUIUITab::setValue(TabletValue type, covise::TokenBuffer &tb)
{
    if (type == TABLET_UI_USE_DESCRIPTION)
    {
        int size;
        ushort *data;
        tb >> size;
        data = (ushort *)tb.getBinary(size);

        uiDescription = QString::fromUtf16(data);

        delete uiWidget;
        uiWidget = new TUIUIWidget(uiDescription, this);
        widget()->layout()->addWidget(uiWidget);

        connect(this->uiWidget, SIGNAL(command(QString, QString)), this, SLOT(sendCommand(QString, QString)));

        //std::cerr << "TUIUITab::setValue info: ui " << std::endl << qPrintable(this->uiDescription) << std::endl;
    }
    else if (type == TABLET_UI_COMMAND)
    {
        std::string target;
        tb >> target;

        int size;
        ushort *data;
        tb >> size;
        data = (ushort *)tb.getBinary(size);

        QString command = QString::fromUtf16(data);

        //std::cerr << "TUIUITab::setValue info: command = " << qPrintable(command) << std::endl;
        this->uiWidget->processMessage(command);
    }

    TUITab::setValue(type, tb);
}

void TUIUITab::sendCommand(const QString &target, const QString &command)
{
    //std::cerr << "TUIUITab::sendCommand info: sending command (" <<
    //             qPrintable(target) << ") " << qPrintable(command) << std::endl;

    covise::TokenBuffer tb;
    unsigned long commandSize = (unsigned long)(command.size() + 1) * 2;

    tb << ID;
    tb << TABLET_UI_COMMAND;
    tb << target.toLocal8Bit().data();
    tb << (uint64_t)commandSize;
    tb.addBinary((const char *)command.utf16(), (command.size() + 1) * 2);
    TUIMain::getInstance()->send(tb);
}
