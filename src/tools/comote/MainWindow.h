/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

// MainWindow.h

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QtGlobal>
#include <qtutil/Qt5_15_deprecated.h>

#include "ui_MainWindow.h"

#include "MainWidget.h"
#include "NetworkDialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags{});
#else
    MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
#endif
    virtual ~MainWindow();

protected:
    virtual void closeEvent(QCloseEvent *closeEvent);

private slots:
    void message(int /*MainWidget::Message*/ type, int x, int y, int z, int w);

    void toggleFullscreen();
    void connectToHost();
    void disconnectFromHost();

private:
    Ui::MainWindowUi ui;

    MainWidget *mainWidget;
    NetworkDialog *networkDialog;

    bool canClose;
};
#endif
