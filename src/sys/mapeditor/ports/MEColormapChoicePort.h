/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef ME_COLORMAPCHOICEPORT_H
#define ME_COLORMAPCHOICEPORT_H

#include <QVector>
#include <QMap>

#include "ports/MEParameterPort.h"

#include <QStringList>

class QWidget;
class QComboBox;
class QVBoxLayout;

class MENode;
class MEColorMap;
class MELineEdit;
class MEColorRGBTable;
class MEComboBox;

//================================================
class MEColormapChoicePort : public MEParameterPort
//================================================
{

    Q_OBJECT

public:
    MEColormapChoicePort(MENode *node, QGraphicsScene *scene, const QString &pportname, const QString &paramtype, const QString &description);
    MEColormapChoicePort(MENode *node, QGraphicsScene *scene, const QString &portname, int paramtype, const QString &description, int porttype);

    ~MEColormapChoicePort();

    void defineParam(QString value, int apptype);
    void modifyParam(QStringList list, int noOfValues, int istart);
    void modifyParameter(QString value);
    void sendParamMessage();
    void moduleParameterRequest();
    void makeLayout(layoutType type, QWidget *);
    void restoreParam();
    void storeParam();

    MEColorMap *getColorMap()
    {
        return m_colorMap;
    }

private slots:

    void choiceCB(int);

private:
    int m_noOfChoices;
    unsigned int m_currentChoice, m_currentChoiceold;
    QStringList m_choiceValues, m_choiceValuesold;
    QVector<float *> m_values, m_valuesold;
    QVector<int> m_mapPoints, m_mapPointsOld;

    MEColorMap *m_colorMap;
    MEComboBox *m_comboBox[2];
    MEColorRGBTable *m_preview[2];

    QString makeColorMapValues();
};
#endif
