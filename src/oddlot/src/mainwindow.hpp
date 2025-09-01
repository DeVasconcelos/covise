/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

 /**************************************************************************
 ** ODD: OpenDRIVE Designer
 **   Frank Naegele (c) 2010
 **   <mail@f-naegele.de>
 **   1/18/2010
 **
 **************************************************************************/

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QUndoView>
#include <QSettings>

#include <src/util/odd.hpp>

 // Project
 //
#include <src/gui/projectwidget.hpp>


class QMdiArea;
class QMdiSubWindow;

class QUndoGroup;
class QUndoView;

class QAction;
class QActionGroup;

class QMenu;

class QLabel;

class QGroupBox;

class ToolManager;
class ToolAction;

class ToolParameterSettings;
class Tool;
class ParameterDockWidget;

class PrototypeManager;
class SignalManager;
class SignalTreeWidget;
class WizardManager;
class OsmImport;
class COVERConnection;

#include "src/gui/projectionsettings.hpp"
#include "src/gui/importsettings.hpp"
#include "src/gui/exportsettings.hpp"
#include "src/gui/lodsettings.hpp"
#include "src/gui/apisettings.hpp"
#include "src/gui/oscsettings.hpp"
#include "src/cover/coverconnection.hpp"
#include "src/gui/filesettings.hpp"

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    class UndoView;

    Q_OBJECT

        //################//
        // FUNCTIONS      //
        //################//

public:

    explicit MainWindow(QWidget *parent = NULL);
    virtual ~MainWindow();

    // Project //
    //
    ProjectWidget *getActiveProject();
    ProjectWidget *getLastActiveProject();

    // Manager //
    //
    ToolManager *getToolManager() const
    {
        return toolManager_;
    }
    PrototypeManager *getPrototypeManager() const
    {
        return prototypeManager_;
    }
    SignalManager *getSignalManager() const
    {
        return signalManager_;
    }

    // Menus //
    //
    QMenu *getFileMenu() const
    {
        return fileMenu_;
    }
    QMenu *getEditMenu() const
    {
        return editMenu_;
    }
    QMenu *getWizardsMenu() const
    {
        return wizardsMenu_;
    }
    QMenu *getViewMenu() const
    {
        return viewMenu_;
    }
    QMenu *getProjectMenu() const
    {
        return projectMenu_;
    }
    QMenu *getHelpMenu() const
    {
        return helpMenu_;
    }

    const QString &getCovisedir()
    {
        return covisedir_;
    }

    // StatusBar //
    //
    void updateStatusBarPos(const QPointF &pos);

    // Undo //
    //
    QUndoGroup *getUndoGroup() const
    {
        return undoGroup_;
    }

    // ProjectTree //
    //
    void setProjectTree(QWidget *widget);

    // SignalTree //
   //
    void setSignalTree(QWidget *widget);

    SignalTreeWidget *getSignalTree()
    {
        return signalTree_;
    }

    ProjectionSettings *getProjectionSettings()
    {
        return projectionSettings;
    }

    FileSettings *getFileSettings()
    {
        return fileSettings;
    }

    ParameterDockWidget *getParameterDialog()
    {
        return parameterDialog_;
    }

    // ErrorMessageTree //
    //
    void setErrorMessageTree(QWidget *widget);

    void showDock(ODD::EditorId editor);
    void hideDock(ODD::EditorId editor);

    // ProjectSettings //
    //
    void setProjectSettings(QWidget *widget);

    // ProjectSignals //
    //
    void setProjectSignals(QWidget *widget);

    void open(QString fileName, ProjectWidget::FileType type = ProjectWidget::FileType::FT_All);
    void openTile(QString fileName);

    // add Catalog dock widgets when the project is openend
    QDockWidget *createCatalog(const QString &, QWidget *widget);

    void showParameterDialog(bool show, const QString &windowTitle = "", const QString &helpText = "");

    void updateCOVERConnectionIcon(const QIcon &icon);

    void hideParameterSettings();


    QSettings appSettings;

private:
    // Init functions //
    //
    void createMenus();
    void createToolBars();
    void createStatusBar();

    void createActions();



    void createFileSettings();

    void createCOVERConnectionButton();


    void createMdiArea();
    void createPrototypes();
    void createSignals();
    void createTools();
    void createUndo();
    void createTree();
    void createSettings();
    void createWizards();
    void createErrorMessageTab();
    void createParameterSettings();

    ProjectionSettings *projectionSettings;

    COVERConnection *coverConnection;

    OSCSettings *oscSettings;
    ImportSettings *importSettings;
    ExportSettings *exportSettings;
    LODSettings* lodSettings;
    APISettings* apiSettings;

    FileSettings *fileSettings;

    // Program Settings //
    //
    // void      readSettings();
    // void      writeSettings();

    // Project //
    //
    ProjectWidget *createProject();
    const QString  createProjectFilename();
    QMdiSubWindow *findProject(const QString &fileName);

    //################//
    // EVENTS         //
    //################//

protected:
    virtual void changeEvent(QEvent *event);
    virtual void closeEvent(QCloseEvent *event);

    //################//
    // SIGNALS        //
    //################//

signals:
    void hasActiveProject(bool);
    void activated();

    //################//
    // SLOTS          //
    //################//

public slots:

    // Project Handling //
    //
    void activateProject();

    // Tools //
    //
    void toolAction(ToolAction *);

    void settingsDockParentChanged(bool);

private slots:

    // Menu Slots //
    //
    void newFile();
    void open();
    void openXODR();
    void openXOSC();
    void mergeXOSC();
    void save();
    void openTile();
    void saveAs();
    void saveAsXODR();
    void saveAsXOSC();
    void exportSpline();
    void changeSettings();
    void changeOSCSettings();
    void changeImportSettings();
    void changeExportSettings();
    void importIntermapRoad();
    void importCarMakerRoad();
    void importCSVRoad();
    void importCSVSign();
    void importOSMRoad();
    void importOSMFile();
    void about();
    void openRecentFile();
    void changeLODSettings();

    void changeFileSettings();

    void changeCOVERConnection();


    void openCOVERSettings();


    //################//
    // PROPERTIES     //
    //################//

private:
    // Main GUI //
    //
    Ui::MainWindow *ui;
    OsmImport *osmi;

    // Official Menus //
    //
    QMenu *fileMenu_;
    QMenu *editMenu_;
    QMenu *wizardsMenu_;
    QMenu *viewMenu_;
    QMenu *projectMenu_;
    QMenu *helpMenu_;

    // Official ToolBars //
    //
    QToolBar *fileToolBar_;

    // Manager //
    //
    ToolManager *toolManager_;
    PrototypeManager *prototypeManager_;
    WizardManager *wizardManager_;
    SignalManager *signalManager_;

    // UI elements //
    //
    QMdiArea *mdiArea_;

    QDockWidget *undoDock_;
    QUndoGroup *undoGroup_;
    UndoView *undoView_;

    QDockWidget *errorDock_;
    QWidget *emptyMessageWidget_;

    QDockWidget *ribbonToolDock_;

    QDockWidget *treeDock_;
    QWidget *emptyTreeWidget_;

    QDockWidget *signalsDock_;
    QWidget *emptySignalsWidget_;
    SignalTreeWidget *signalTree_;

    ParameterDockWidget *parameterDialog_;

    QList<QDockWidget *> catalogsDock_;
    QDockWidget *settingsDock_;
    QWidget *emptySettingsWidget_;

    QMainWindow* dockAreaWidget_=nullptr;
    // StatusBar //
    //
    QLabel *locationLabel_;

    // Project Menu //
    //
    QActionGroup *projectActionGroup;

    // Covise Directory Path //
    //
    QString covisedir_;

    // COVERConnection Button //
    //
    QPushButton *coverButton;

    QToolBar *coverConnectionToolBar;


    class UndoView : public QUndoView
    {
    public:

        explicit UndoView(QUndoGroup *group, QWidget *parent = NULL);
        virtual ~UndoView() {};

    protected:
        virtual void mouseReleaseEvent(QMouseEvent *event);

    private:
        MainWindow *mainWindow_;
    };
};



#endif // MAINWINDOW_HPP
