/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */


#include <QHBoxLayout>

#include "MELineEdit.h"
#include "MEVectorPort.h"
#include "handler/MEMainHandler.h"
#include "nodes/MENode.h"
#include "widgets/MEUserInterface.h"
#include <qtutil/Qt5_15_deprecated.h>
//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
MEVectorPort::MEVectorPort(MENode *node, QGraphicsScene *scene,
                           const QString &portname,
                           const QString &paramtype,
                           const QString &description)
    : MEParameterPort(node, scene, portname, paramtype, description)
{
    m_vector.clear();
    m_vectorold.clear();
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
MEVectorPort::MEVectorPort(MENode *node, QGraphicsScene *scene,
                           const QString &portname,
                           int paramtype,
                           const QString &description,
                           int porttype)
    : MEParameterPort(node, scene, portname, paramtype, description, porttype)
{
    m_vector.clear();
    m_vectorold.clear();
}

//------------------------------------------------------------------------
MEVectorPort::~MEVectorPort()
//------------------------------------------------------------------------
{
}

QString MEVectorPort::asString(int idx) const
{
    return toString(m_vector.at(idx).toDouble());
}

//------------------------------------------------------------------------
// restore saved parameters
// after the user has pressed cancel in module parameter window
//------------------------------------------------------------------------
void MEVectorPort::restoreParam()
{
    m_vector.clear();

    for (int k = 0; k < m_nVect; k++)
    {
        QVariant v(m_vectorold.at(k));
        m_vector.append(v);
    }

    sendParamMessage();
}

//------------------------------------------------------------------------
//save current value for further use
//------------------------------------------------------------------------
void MEVectorPort::storeParam()
{
    m_vectorold.clear();

    for (int k = 0; k < m_nVect; k++)
    {
        QVariant v(m_vector.at(k));
        m_vectorold.append(v);
    }
}

//------------------------------------------------------------------------
// module has requested parameter
//------------------------------------------------------------------------
void MEVectorPort::moduleParameterRequest()
{
    sendParamMessage();
}

//------------------------------------------------------------------------
// define one parameter of a module,	init from controller
//------------------------------------------------------------------------
void MEVectorPort::defineParam(QString value, int apptype)
{
    QStringList list = value.split(" ", SplitBehaviorFlags::SkipEmptyParts);
    m_nVect = list.count();

    for (int i = 0; i < m_nVect; i++)
    {
        QVariant v(list[i]);
        m_vector.append(v);
    }

    MEParameterPort::defineParam(value, apptype);
}

//------------------------------------------------------------------------
// modify one parameter of a module,	update param  from controller
//------------------------------------------------------------------------
void MEVectorPort::modifyParam(QStringList list, int noOfValues, int istart)
{
    Q_UNUSED(noOfValues);

    if (list.count() > istart + m_nVect - 1)
    {
        for (int k = 0; k < m_nVect; k++)
        {
            QVariant v(list[istart + k]);
            m_vector.append(v);
        }

        // modify module & control line content
        if (!m_vectorList[MODULE].isEmpty())
        {
            for (int j = 0; j < m_nVect; j++)
                m_vectorList[MODULE].at(j)->setText(asString(j));
        }

        if (!m_vectorList[CONTROL].isEmpty())
        {
            for (int j = 0; j < m_nVect; j++)
                m_vectorList[CONTROL].at(j)->setText(asString(j));
        }
    }

    else
    {
        QString msg = "MEParameterPort::modifyParam: " + node->getNodeTitle() + ": Parameter type " + parameterType + " has wrong number of values";
        MEUserInterface::instance()->printMessage(msg);
    }
}

//------------------------------------------------------------------------
// modify one parameter of a module,	update param  from controller
//------------------------------------------------------------------------
void MEVectorPort::modifyParameter(QString lvalue)
{
    lvalue = lvalue.trimmed();

    QStringList list = QString(lvalue).split(" ");

    if ((list.count() == m_nVect) || (list.count() == m_nVect + 1))
    {
        m_vector.clear();

        for (int k = 0; k < m_nVect; k++)
        {
            QVariant v(list[k]);
            m_vector.append(v);
        }

        // modify module & control line content
        if (!m_vectorList[MODULE].isEmpty())
        {
            for (int j = 0; j < m_nVect; j++)
                m_vectorList[MODULE].at(j)->setText(asString(j));
        }

        if (!m_vectorList[CONTROL].isEmpty())
        {
            for (int j = 0; j < m_nVect; j++)
                m_vectorList[CONTROL].at(j)->setText(asString(j));
        }
    }

    else
    {
        QString msg = "MEParameterPort::modifyParam: " + node->getNodeTitle() + ": Parameter type " + parameterType + " has wrong number of values";
        MEUserInterface::instance()->printMessage(msg);
    }
}

//------------------------------------------------------------------------
// make the layout for the module parameter widget
//------------------------------------------------------------------------
void MEVectorPort::makeLayout(layoutType type, QWidget *w)
{

    QHBoxLayout *hBox = new QHBoxLayout(w);
    hBox->setContentsMargins(2, 2, 2, 2);
    hBox->setSpacing(2);

    // values

    for (int i = 0; i < m_nVect; i++)
    {
        MELineEdit *le = new MELineEdit();
        le->setMinimumWidth(MEMainHandler::instance()->getSliderWidth());
        le->setText(asString(i));

        connect(le, SIGNAL(contentChanged(const QString &)), this, SLOT(textCB(const QString &)));
        connect(le, SIGNAL(focusChanged(bool)), this, SLOT(setFocusCB(bool)));
        m_vectorList[type].append(le);
        hBox->addWidget(le, 2);
    }
}

void MEVectorPort::sendParamMessage()
//------------------------------------------------------------------------
/* send a PARAM message to controller				                        */
/* key	    ______	    keyword for message			                  	*/
//------------------------------------------------------------------------
{
    QString buffer;

    for (int k = 0; k < m_nVect; k++)
    {
        buffer.append(asString(k));
        buffer.append(" ");
    }

    MEParameterPort::sendParamMessage(buffer);
}

//------------------------------------------------------------------------
// unmap parameter from control panel
// remove extended parts for filebrowser and colormap
// close extended parts
// reset parent
//------------------------------------------------------------------------
void MEVectorPort::removeFromControlPanel()
{
    MEParameterPort::removeFromControlPanel();
    m_vectorList[CONTROL].clear();
}
