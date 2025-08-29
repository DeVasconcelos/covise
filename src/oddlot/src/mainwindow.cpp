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

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

 // Manager //
 //
#include "src/data/prototypemanager.hpp"
#include "src/data/signalmanager.hpp"
#include "src/wizards/wizardmanager.hpp"

// GUI //
//
#include "src/gui/tools/toolmanager.hpp"
#include "src/gui/osmimport.hpp"
#include "src/gui/parameters/parameterdockwidget.hpp"

// Graph //
//
#include "src/graph/editors/projecteditor.hpp"

// Qt //
//
#include <QtGui>
#include <QPushButton>
#include <QLabel>
#include <QUndoGroup>
#include <QUndoView>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QDockWidget>
#include <QToolBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>

// Utils //
//
#include "src/util/odd.hpp"

// tree //
//
#include "src/tree/signaltreewidget.hpp"

//################//
// Constructors   //
//################//

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , signalTree_(NULL)
{
    ui->setupUi(this);

    setWindowTitle(tr("ODD: OpenDRIVE Designer"));
    resize(1024, 768);

    setCorner(Qt::TopLeftCorner, Qt::TopDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::TopDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // ODD //
    //
    ODD::init(this);

    // Official Menus and Bars //
    //
    createMenus();
    createToolBars();
    createStatusBar();

    // Official Actions //
    //
    createActions();

    createMdiArea();
    createTree();
    createSettings();

    //createFileSettings();


    createUndo();
    createErrorMessageTab();
    createPrototypes();
    createTools();
    createParameterSettings();
    createSignals();
    createWizards();

    projectionSettings = new ProjectionSettings();
    importSettings = new ImportSettings();
    exportSettings = new ExportSettings();
    lodSettings = new LODSettings();
    apiSettings = new APISettings(this);
    oscSettings = new OSCSettings(covisedir_ + "/share/covise/catalogs/");

    //---------------------------------------//
#ifdef COVER_CONNECTION
    coverConnection = COVERConnection::instance();
#endif
    fileSettings = new FileSettings();

    createFileSettings();
    createCOVERConnectionButton();
    // Default //
    //
    emit(hasActiveProject(false));

}

MainWindow::~MainWindow()
{
    delete ui;
    delete signalTree_;

    ODD::kill();
}

//################//
// INIT FUNCTIONS //
//################//

/*! \brief Creates the official menus.
*
*/
void
MainWindow::createMenus()
{
    fileMenu_ = menuBar()->addMenu(tr("&File"));
    editMenu_ = menuBar()->addMenu(tr("&Edit"));
    wizardsMenu_ = menuBar()->addMenu(tr("&Wizards"));
    viewMenu_ = menuBar()->addMenu(tr("&View"));
    projectMenu_ = menuBar()->addMenu(tr("&Project"));

    menuBar()->addSeparator();
    helpMenu_ = menuBar()->addMenu(tr("&Help"));
}

/*! \brief Creates the official tool bars.
*
*/
void
MainWindow::createToolBars()
{
    fileToolBar_ = addToolBar(tr("File"));
}

/*! \brief Creates the status bar.
*
*/
void
MainWindow::createStatusBar()
{
    locationLabel_ = new QLabel(" [-99999.999, -99999.999] ");
    locationLabel_->setAlignment(Qt::AlignHCenter);
    locationLabel_->setMinimumSize(locationLabel_->sizeHint());
    statusBar()->addPermanentWidget(locationLabel_);
    updateStatusBarPos(QPointF(0.0, 0.0));
}

void
MainWindow::updateStatusBarPos(const QPointF &pos)
{
    locationLabel_->setText(QString(" [ %1").arg(pos.x(), 10, 'f', 3, ' ').append(", %2 ] ").arg(pos.y(), 10, 'f', 3, ' '));
}

void
MainWindow::createCOVERConnectionButton()
{
    coverButton = new QPushButton();
#ifdef COVER_CONNECTION
    updateCOVERConnectionIcon(*(coverConnection->getIconDisconnected()));
#endif
    coverButton->setIconSize(QSize(30, 30));
    connect(coverButton, SIGNAL(clicked()), this, SLOT(openCOVERSettings()));
    coverConnectionToolBar = new QToolBar();
    coverConnectionToolBar->addWidget(coverButton);
    coverConnectionToolBar->setAllowedAreas(Qt::TopToolBarArea);
    addToolBar(coverConnectionToolBar);
}

void
MainWindow::updateCOVERConnectionIcon(const QIcon &icon)
{
    coverButton->setIcon(icon);
}

/*! \brief Creates the official actions and inserts
* them into the menus and toolbars.
*
*/
void
MainWindow::createActions()
{
    // Files //
    //
    QAction *newAction = new QAction(tr("&New"), this);
    newAction->setShortcuts(QKeySequence::New);
    newAction->setStatusTip(tr("Create a new project file."));
    connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

    QAction *openAction = new QAction(tr("&Open"), this);
    openAction->setShortcuts(QKeySequence::Open);
    openAction->setStatusTip(tr("Open an existing file."));
    connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

    QMenu *openMenu = new QMenu("&Open", fileMenu_);
    openMenu->addAction(openAction);
    QAction *openXODRAction = new QAction(tr("Open Open&Drive File"), openMenu);
    openMenu->addAction(openXODRAction);
    connect(openXODRAction, SIGNAL(triggered()), this, SLOT(openXODR()));

    QAction *openXOSCAction = new QAction(tr("Open Open&Scenario File"), openMenu);
    openMenu->addAction(openXOSCAction);
    connect(openXOSCAction, SIGNAL(triggered()), this, SLOT(openXOSC()));

    QMenu *mergeMenu = new QMenu("&Merge", fileMenu_);
    QAction *openTileAction = new QAction(tr("&Merge Open&Drive file"), this);
    openTileAction->setShortcut(QKeySequence::AddTab);
    openTileAction->setStatusTip(tr("Merge an additonal File."));
    connect(openTileAction, SIGNAL(triggered()), this, SLOT(openTile()));
    connect(this, SIGNAL(hasActiveProject(bool)), openTileAction, SLOT(setEnabled(bool)));
    mergeMenu->addAction(openTileAction);

    QAction *mergeXOSCAction = new QAction(tr("Merge Open&Scenario File"), mergeMenu);
    mergeMenu->addAction(mergeXOSCAction);
    connect(mergeXOSCAction, SIGNAL(triggered()), this, SLOT(mergeXOSC()));
    connect(this, SIGNAL(hasActiveProject(bool)), mergeXOSCAction, SLOT(setEnabled(bool)));

    QAction *saveAction = new QAction(tr("&Save"), this);
    saveAction->setShortcuts(QKeySequence::Save);
    saveAction->setStatusTip(tr("Save the document to disk."));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));
    connect(this, SIGNAL(hasActiveProject(bool)), saveAction, SLOT(setEnabled(bool)));

    QMenu *saveMenu = new QMenu("&Save", fileMenu_);
    saveMenu->addAction(saveAction);
    QAction *saveXODRAction = new QAction(tr("Save As Open&Drive File"), saveMenu);
    saveMenu->addAction(saveXODRAction);
    connect(saveXODRAction, SIGNAL(triggered()), this, SLOT(saveAsXODR()));
    connect(this, SIGNAL(hasActiveProject(bool)), saveXODRAction, SLOT(setEnabled(bool)));

    QAction *saveXOSCAction = new QAction(tr("Save As Open&Scenario File"), saveMenu);
    saveMenu->addAction(saveXOSCAction);
    connect(saveXOSCAction, SIGNAL(triggered()), this, SLOT(saveAsXOSC()));
    connect(this, SIGNAL(hasActiveProject(bool)), saveXOSCAction, SLOT(setEnabled(bool)));

    QAction *saveAsAction = new QAction(tr("Save &As..."), saveMenu);
    saveMenu->addAction(saveAsAction);
    saveAsAction->setShortcuts(QKeySequence::SaveAs);
    saveAsAction->setStatusTip(tr("Save the document under a new name."));
    connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));
    connect(this, SIGNAL(hasActiveProject(bool)), saveAsAction, SLOT(setEnabled(bool)));



    QAction *fileSettingsAction = new QAction(tr("Settings"), fileMenu_);
    connect(fileSettingsAction, SIGNAL(triggered()), this, SLOT(changeFileSettings()));
    connect(this, SIGNAL(hasActiveProject(bool)), fileSettingsAction, SLOT(setEnabled(bool)));


    /*
    QMenu *settingsMenu = new QMenu("&Settings", fileMenu_);

    //QAction *projectionSettingsAction = new QAction(tr("Projection Settings"), fileMenu_);
    QAction *projectionSettingsAction = new QAction(tr("Projection Settings"), settingsMenu);
    settingsMenu->addAction(projectionSettingsAction);
    connect(projectionSettingsAction, SIGNAL(triggered()), this, SLOT(changeSettings()));
    connect(this, SIGNAL(hasActiveProject(bool)), projectionSettingsAction, SLOT(setEnabled(bool)));

    //QAction *lodSettingsAction = new QAction(tr("LOD Settings"), fileMenu_);
    QAction *lodSettingsAction = new QAction(tr("LOD Settings"), settingsMenu);
    settingsMenu->addAction(lodSettingsAction);
    connect(lodSettingsAction, SIGNAL(triggered()), this, SLOT(changeLODSettings()));
    connect(this, SIGNAL(hasActiveProject(bool)), lodSettingsAction, SLOT(setEnabled(bool)));


    //QAction *coverConnectionAction = new QAction(tr("COVERConnection"), fileMenu_);
    QAction *coverConnectionAction = new QAction(tr("COVERConnection"), settingsMenu);
    settingsMenu->addAction(coverConnectionAction);
    connect(coverConnectionAction,SIGNAL(triggered()),this, SLOT(changeCOVERConnection()));
    connect(this, SIGNAL(hasActiveProject(bool)), coverConnectionAction, SLOT(setEnabled(bool)));


    //QAction *OSCSettingsAction = new QAction(tr("OpenSCENARIO Settings"), fileMenu_);
    QAction *OSCSettingsAction = new QAction(tr("OpenSCENARIO Settings"), settingsMenu);
    settingsMenu->addAction(OSCSettingsAction);
    connect(OSCSettingsAction, SIGNAL(triggered()), this, SLOT(changeOSCSettings()));
    connect(this, SIGNAL(hasActiveProject(bool)), OSCSettingsAction, SLOT(setEnabled(bool)));

    //QAction *importSettingsAction = new QAction(tr("Import Settings"), fileMenu_);
    QAction *importSettingsAction = new QAction(tr("Import Settings"), settingsMenu);
    settingsMenu->addAction(importSettingsAction);
    connect(importSettingsAction, SIGNAL(triggered()), this, SLOT(changeImportSettings()));
    connect(this, SIGNAL(hasActiveProject(bool)), importSettingsAction, SLOT(setEnabled(bool)));

    //QAction *exportSettingsAction = new QAction(tr("Export Settings"), fileMenu_);
    QAction *exportSettingsAction = new QAction(tr("Export Settings"), settingsMenu);
    settingsMenu->addAction(exportSettingsAction);
    connect(exportSettingsAction, SIGNAL(triggered()), this, SLOT(changeExportSettings()));
    connect(this, SIGNAL(hasActiveProject(bool)), exportSettingsAction, SLOT(setEnabled(bool)));*/

    QMenu *exportMenu = new QMenu("E&xport", fileMenu_);
    QAction *exportSplineAction = new QAction(tr("Export &Spline"), exportMenu);
    exportMenu->addAction(exportSplineAction);
    connect(exportSplineAction, SIGNAL(triggered()), this, SLOT(exportSpline()));
    connect(this, SIGNAL(hasActiveProject(bool)), exportSplineAction, SLOT(setEnabled(bool)));

    QMenu *importMenu = new QMenu("Import", fileMenu_);
    QAction *importIntermapAction = new QAction(tr("Import Intermap Road"), exportMenu);
    importMenu->addAction(importIntermapAction);
    connect(importIntermapAction, SIGNAL(triggered()), this, SLOT(importIntermapRoad()));
    connect(this, SIGNAL(hasActiveProject(bool)), importIntermapAction, SLOT(setEnabled(bool)));
    /*
    QAction *importCSVAction = new QAction(tr("Import CSV file"), exportMenu);
    importMenu->addAction(importCSVAction);
    connect(importCSVAction, SIGNAL(triggered()), this, SLOT(importCSVRoad()));
    connect(this, SIGNAL(hasActiveProject(bool)), importCSVAction, SLOT(setEnabled(bool)));
    */
    QAction *importCSVRoadAction = new QAction(tr("Import CSV Road"), exportMenu);
    importMenu->addAction(importCSVRoadAction);
    connect(importCSVRoadAction, SIGNAL(triggered()), this, SLOT(importCSVRoad()));
    connect(this, SIGNAL(hasActiveProject(bool)), importCSVRoadAction, SLOT(setEnabled(bool)));
    QAction *importCSVSignAction = new QAction(tr("Import CSV Sign"), exportMenu);
    importMenu->addAction(importCSVSignAction);
    connect(importCSVSignAction, SIGNAL(triggered()), this, SLOT(importCSVSign()));
    connect(this, SIGNAL(hasActiveProject(bool)), importCSVSignAction, SLOT(setEnabled(bool)));
    QAction *importCarMakerAction = new QAction(tr("Import CarMaker Road file"), exportMenu);
    importMenu->addAction(importCarMakerAction);
    connect(importCarMakerAction, SIGNAL(triggered()), this, SLOT(importCarMakerRoad()));
    connect(this, SIGNAL(hasActiveProject(bool)), importCarMakerAction, SLOT(setEnabled(bool)));
    QAction *importOSMAction = new QAction(tr("Download OSM Data"), exportMenu);
    importMenu->addAction(importOSMAction);
    connect(importOSMAction, SIGNAL(triggered()), this, SLOT(importOSMRoad()));
    connect(this, SIGNAL(hasActiveProject(bool)), importOSMAction, SLOT(setEnabled(bool)));
    QAction *importOSMFileAction = new QAction(tr("Import OSM file"), exportMenu);
    importMenu->addAction(importOSMFileAction);
    connect(importOSMFileAction, SIGNAL(triggered()), this, SLOT(importOSMFile()));
    connect(this, SIGNAL(hasActiveProject(bool)), importOSMFileAction, SLOT(setEnabled(bool)));

    QAction *exitAction = new QAction(tr("&Exit"), this);
    exitAction->setShortcuts(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Exit program."));
    connect(exitAction, SIGNAL(triggered()), qApp, SLOT(closeAllWindows())); // ?!

    fileMenu_->addAction(newAction);
    fileMenu_->addMenu(openMenu);
    fileMenu_->addMenu(saveMenu);
    fileMenu_->addMenu(mergeMenu);


    //fileMenu_->addMenu(settingsMenu);
    fileMenu_->addAction(fileSettingsAction);

    fileMenu_->addSeparator();
    /*fileMenu_->addAction(projectionSettingsAction);
    fileMenu_->addAction(lodSettingsAction);

    fileMenu_->addAction(coverConnectionAction);
    fileMenu_->addAction(importSettingsAction);
    fileMenu_->addAction(exportSettingsAction);
    fileMenu_->addAction(OSCSettingsAction);*/
    fileMenu_->addMenu(importMenu);
    fileMenu_->addMenu(exportMenu);
    fileMenu_->addSeparator();
    fileMenu_->addAction(exitAction);

    fileToolBar_->addAction(newAction);
    fileToolBar_->addAction(openAction);
    fileToolBar_->addAction(saveAction);

    // About //
    //
    QAction *aboutAction = new QAction(tr("&About ODDLOT"), this);
    aboutAction->setStatusTip(tr("Show information about OpenDRIVE Designer."));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    helpMenu_->addAction(aboutAction);

    osmi = new OsmImport();

}

void
MainWindow::createFileSettings()
{
    fileSettings->addTab(projectionSettings);
    fileSettings->addTab(exportSettings);
    fileSettings->addTab(importSettings);
    fileSettings->addTab(oscSettings);
    fileSettings->addTab(lodSettings);
    fileSettings->addTab(apiSettings);
#ifdef COVER_CONNECTION
    fileSettings->addTab(coverConnection);
#endif
}



/*! \brief Creates the MDI Area.
*
*/
void
MainWindow::createMdiArea()
{
    // MDI Area //
    //
    mdiArea_ = new QMdiArea();

    //setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);
    setCentralWidget(mdiArea_);
    connect(mdiArea_, SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(activateProject()));

    // Actions //
    //
    projectActionGroup = new QActionGroup(this);

    QAction *closeAction = new QAction(tr("Cl&ose"), this);
    closeAction->setStatusTip(tr("Close the active window."));
    connect(closeAction, SIGNAL(triggered()), mdiArea_, SLOT(closeActiveSubWindow()));
    connect(this, SIGNAL(hasActiveProject(bool)), closeAction, SLOT(setEnabled(bool)));

    QAction *closeAllAction = new QAction(tr("Close &All"), this);
    closeAllAction->setStatusTip(tr("Close all the windows."));
    connect(closeAllAction, SIGNAL(triggered()), mdiArea_, SLOT(closeAllSubWindows()));
    connect(this, SIGNAL(hasActiveProject(bool)), closeAllAction, SLOT(setEnabled(bool)));

    QAction *tileAction = new QAction(tr("&Tile"), this);
    tileAction->setStatusTip(tr("Tile the windows."));
    connect(tileAction, SIGNAL(triggered()), mdiArea_, SLOT(tileSubWindows()));
    connect(this, SIGNAL(hasActiveProject(bool)), tileAction, SLOT(setEnabled(bool)));

    QAction *cascadeAction = new QAction(tr("&Cascade"), this);
    cascadeAction->setStatusTip(tr("Cascade the windows."));
    connect(cascadeAction, SIGNAL(triggered()), mdiArea_, SLOT(cascadeSubWindows()));
    connect(this, SIGNAL(hasActiveProject(bool)), cascadeAction, SLOT(setEnabled(bool)));

    QAction *nextAction = new QAction(tr("Ne&xt"), this);
    nextAction->setShortcuts(QKeySequence::NextChild);
    nextAction->setStatusTip(tr("Move the focus to the next window."));
    connect(nextAction, SIGNAL(triggered()), mdiArea_, SLOT(activateNextSubWindow()));
    connect(this, SIGNAL(hasActiveProject(bool)), nextAction, SLOT(setEnabled(bool)));

    QAction *previousAction = new QAction(tr("Pre&vious"), this);
    previousAction->setShortcuts(QKeySequence::PreviousChild);
    previousAction->setStatusTip(tr("Move the focus to the previous window."));
    connect(previousAction, SIGNAL(triggered()), mdiArea_, SLOT(activatePreviousSubWindow()));
    connect(this, SIGNAL(hasActiveProject(bool)), previousAction, SLOT(setEnabled(bool)));

    projectMenu_->addAction(closeAction);
    projectMenu_->addAction(closeAllAction);
    projectMenu_->addSeparator();
    projectMenu_->addAction(tileAction);
    projectMenu_->addAction(cascadeAction);
    projectMenu_->addSeparator();
    projectMenu_->addAction(nextAction);
    projectMenu_->addAction(previousAction);
    projectMenu_->addSeparator();
}

/*! \brief Creates the PrototypeManager and the application's prototypes.
*
* \note Some day there might be included "application prototypes",
* user created "installation prototypes" (in the settings file) and
* "file prototypes" saved per project.
*/
void
MainWindow::createPrototypes()
{
    // PrototypeManager //
    //
    prototypeManager_ = new PrototypeManager(this);
#ifdef WIN32
    char *pValue;
    size_t len;
    errno_t err = _dupenv_s(&pValue, &len, "ODDLOTDIR");
    if (err || pValue == NULL || strlen(pValue) == 0)
        err = _dupenv_s(&pValue, &len, "COVISEDIR");
    if (err)
        return;
    covisedir_ = pValue;
#else
    covisedir_ = getenv("ODDLOTDIR");
    if (covisedir_ == "")
        covisedir_ = getenv("COVISEDIR");
#endif
    if (prototypeManager_->loadPrototypes(covisedir_ + "/share/covise/prototypes/prototypes.odd"))
    {
        prototypeManager_->loadPrototypes(covisedir_ + "/share/covise/prototypes/TJunctionTown.odd");
    }
    else if (prototypeManager_->loadPrototypes(covisedir_ + "/src/oddlot/prototypes/prototypes.odd"))
    {
        prototypeManager_->loadPrototypes(covisedir_ + "/src/oddlot/prototypes/TJunctionTown.odd");
    }
    else if (prototypeManager_->loadPrototypes("prototypes/prototypes.odd"))
    {
        prototypeManager_->loadPrototypes("prototypes/TJunctionTown.odd");
    }
    else
    {
        fprintf(stderr, "Could not load prototypes.odd\n");
        exit(-1);
    }
}

void
MainWindow::createSignals()
{
    // SignalManager //
    //
    signalManager_ = new SignalManager(this);
#ifdef WIN32
    char *pValue = NULL;
    size_t len = 0;
    errno_t err = _dupenv_s(&pValue, &len, "ODDLOTDIR");
    if (err || pValue == NULL || strlen(pValue) == 0)
        err = _dupenv_s(&pValue, &len, "COVISEDIR");
    if (err)
        return;
    QString covisedir = pValue;
#else
    QString covisedir = getenv("ODDLOTDIR");
    if (covisedir == "")
        covisedir = getenv("COVISEDIR");
#endif
    if (!signalManager_->loadSignals(covisedir + "/src/oddlot/signs/signs.xml"))
    {
        if (!signalManager_->loadSignals(covisedir + "/share/covise/signs/signs.xml"))
        {
            fprintf(stderr, "Could not load signs.xml\n");
            exit(-1);
        }
    }

    // Dock Area //
    //
    signalsDock_ = new QDockWidget(tr("Signals & Objects"), this);
    signalsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, signalsDock_);
    tabifyDockWidget(treeDock_, signalsDock_);
    treeDock_->raise();

    // Show/Hide Action //
    //
    QAction *signalsDockToggleAction = signalsDock_->toggleViewAction();
    signalsDockToggleAction->setStatusTip(tr("Show/hide the Signals view."));
    viewMenu_->addAction(signalsDockToggleAction);

    // SignalTree //
    //
    signalTree_ = new SignalTreeWidget(signalManager_, this);
    setSignalTree(signalTree_);

    signalsDock_->hide();
}

/*! \brief Creates the ToolManager and the tool box on the left side.
*
*/

void
MainWindow::createTools()
{

    // ToolManager //
    //
    toolManager_ = new ToolManager(prototypeManager_, this);
    connect(toolManager_, SIGNAL(toolAction(ToolAction *)), this, SLOT(toolAction(ToolAction *)));
    connect(this, SIGNAL(hasActiveProject(bool)), toolManager_, SLOT(loadProjectEditor(bool)));

    ribbonToolDock_ = new QDockWidget(tr("Ribbon"), this);
    QWidget *titleWidget = new QWidget(this);
    ribbonToolDock_->setTitleBarWidget(titleWidget); // empty title bar
    ribbonToolDock_->setAllowedAreas(Qt::TopDockWidgetArea);
    addDockWidget(Qt::TopDockWidgetArea, ribbonToolDock_);

    // Show/Hide Action //
    //
    QAction *ribbonToolDockToggleAction = ribbonToolDock_->toggleViewAction();
    ribbonToolDockToggleAction->setStatusTip(tr("Show/hide the ribbon."));
    viewMenu_->addAction(ribbonToolDockToggleAction);

    dockAreaWidget_ = new QMainWindow;
    dockAreaWidget_->setWindowFlags(Qt::Widget);
    dockAreaWidget_->setMinimumSize(50,100);

    QWidget* RobbonContainer = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(RobbonContainer);
    layout->addWidget(toolManager_->getRibbonWidget());
    layout->addWidget(dockAreaWidget_);
    layout->setStretchFactor(dockAreaWidget_, 10);
    ribbonToolDock_->setWidget(RobbonContainer);

}



/*! \brief Creates the WizardManager.
*
*/
void
MainWindow::createWizards()
{
    wizardManager_ = new WizardManager(this);
}

/*! \brief Creates the tree view dock.
*/
void
MainWindow::createTree()
{
    // Dock Area //
    //
    treeDock_ = new QDockWidget(tr("Tree View"), this);
    treeDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, treeDock_);

    // Show/Hide Action //
    //
    QAction *treeDockToggleAction = treeDock_->toggleViewAction();
    treeDockToggleAction->setStatusTip(tr("Show/hide the tree view."));
    viewMenu_->addAction(treeDockToggleAction);


    // Tree Widget //
    //
    emptyTreeWidget_ = new QWidget();

    treeDock_->setWidget(emptyTreeWidget_);
}

/*! \brief Creates the settings view dock.
*/
void
MainWindow::createSettings()
{
    // Dock Area //
    //
    settingsDock_ = new QDockWidget(tr("Settings View"), this);
    settingsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    settingsDock_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    settingsDock_->setFixedWidth(200);
    settingsDock_->setMinimumHeight(152);

    // settingsDock_->setFeatures(settingsDock_->features() | QDockWidget::DockWidgetVerticalTitleBar);
    addDockWidget(Qt::RightDockWidgetArea, settingsDock_);

    // Show/Hide Action //
    //
    QAction *settingsDockToggleAction = settingsDock_->toggleViewAction();
    settingsDockToggleAction->setStatusTip(tr("Show/hide the settings view."));
    viewMenu_->addAction(settingsDockToggleAction);

    // Settings Widget //
    //
    emptySettingsWidget_ = new QWidget();
    settingsDock_->setWidget(emptySettingsWidget_);

    connect(settingsDock_, SIGNAL(topLevelChanged(bool)), this, SLOT(settingsDockParentChanged(bool)));

}

void
MainWindow::createParameterSettings()
{

    parameterDialog_ = new ParameterDockWidget(tr("ParameterSettings"), this);
    parameterDialog_->setMinimumWidth(200);
    parameterDialog_->setMinimumHeight(152);

    dockAreaWidget_->addDockWidget(Qt::RightDockWidgetArea, parameterDialog_);

    // Show/Hide Action //
    //
    QAction *parameterDialogToggleAction = parameterDialog_->toggleViewAction();
    parameterDialogToggleAction->setStatusTip(tr("Show/hide the parameter view."));
    viewMenu_->addAction(parameterDialogToggleAction);

}


void
MainWindow::showParameterDialog(bool show, const QString &windowTitle, const QString &helpText)
{
    if (show)
    {
        parameterDialog_->setVisibility(true, helpText, windowTitle);
    }
    else
    {
        parameterDialog_->setVisibility(false, "", "");
    }
}

/*! \brief Creates the undo group and view.
*/
void
MainWindow::createUndo()
{
    // Dock Area //
    //
    undoDock_ = new QDockWidget(tr("Undo History"), this);
    undoDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, undoDock_);
    tabifyDockWidget(undoDock_, treeDock_);

    // Show/Hide Action //
    //
    QAction *undoDockToggleAction = undoDock_->toggleViewAction();
    undoDockToggleAction->setStatusTip(tr("Show/hide the undo history."));
    viewMenu_->addAction(undoDockToggleAction);

    // Undo Group //
    //
    undoGroup_ = new QUndoGroup(this);

    // Undo/Redo Action //
    //
    QAction *undoAction = undoGroup_->createUndoAction(this, tr("&Undo"));
    undoAction->setShortcuts(QKeySequence::Undo);

    QAction *redoAction = undoGroup_->createRedoAction(this, tr("&Redo"));
    redoAction->setShortcuts(QKeySequence::Redo);

    editMenu_->addAction(undoAction);
    editMenu_->addAction(redoAction);

    // Undo View //
    //
    undoView_ = new UndoView(undoGroup_, this);
    undoDock_->setWidget(undoView_);
}

void 
MainWindow::hideParameterSettings()
{
    ProjectWidget *projectWidget = getActiveProject();
    if (projectWidget)
    {
        projectWidget->getProjectEditor()->reject();
    }
}


/*! \brief Creates the view for error messages of OpenSCENARIO object settings.
*/
void
MainWindow::createErrorMessageTab()
{
    // Dock Area //
    //
    errorDock_ = new QDockWidget(tr("Error messages"), this);
    errorDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, errorDock_);
    tabifyDockWidget(undoDock_, errorDock_);

    // Show/Hide Action //
    //
    QAction *errorDockToggleAction = errorDock_->toggleViewAction();
    errorDockToggleAction->setStatusTip(tr("Show/hide error messages."));
    viewMenu_->addAction(errorDockToggleAction);

    // Settings Widget //
   //
    emptyMessageWidget_ = new QWidget();

    // connect(errorDock_, SIGNAL(topLevelChanged(bool)), this, SLOT(settingsDockParentChanged(bool)));
}

/*! \brief Creates the tree view dock.
*/
QDockWidget *
MainWindow::createCatalog(const QString &name, QWidget *widget)
{
    // Dock Area //
    //
    QDockWidget *catalogDock = new QDockWidget(name, this);
    catalogDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    catalogDock->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    catalogDock->setFixedWidth(200);
    catalogsDock_.prepend(catalogDock);

    addDockWidget(Qt::RightDockWidgetArea, catalogDock);
    tabifyDockWidget(treeDock_, catalogDock);
    catalogDock->raise();

    // Show/Hide Action //
    //
    QAction *catalogDockToggleAction = catalogDock->toggleViewAction();
    catalogDockToggleAction->setStatusTip(tr("Show/hide the tree view."));
    viewMenu_->addAction(catalogDockToggleAction);


    // Catalog Widget //
    //
    catalogDock->setWidget(widget);

    return catalogDock;
}


//################//
// SLOTS          //
//################//

/*! \brief Triggers the creation of a new project.
*
* The project will be empty.
*/
void
MainWindow::newFile()
{
    ProjectWidget *project = getActiveProject();
    if (project)
    {
        project->setProjectActive(false);
    }

    project = createProject();

    project->newFile(createProjectFilename());
    //   project->show();
    project->showMaximized();
    return;
}

/*! \brief Triggers the creation of a new project.
*
* Tries to open a file. On success, the new project will be
* displayed. On failure, the new project will be deleted again.
* If the file has already been opened, its window will be
* activated.
*/
void
MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {
        // Check if file has already been opened... //
        //
        QMdiSubWindow *existing = findProject(fileName);
        if (existing)
        {
            mdiArea_->setActiveSubWindow(existing);
            return;
        }

        // ... otherwise create new project //
        //

        // [workaround_0] //
        // project->close() did not reliably delete the mdi subwindow when the file could not be loaded.
        // This workaround adds the project widget only after a successful loading.
        // original code:
        // ProjectWidget* project = createProject();
        ProjectWidget *project = new ProjectWidget(this);
        // [workaround_0]

        if (project->loadFile(fileName))
        {
            statusBar()->showMessage(tr("File has been loaded."), 0);

            // [workaround_0]
            mdiArea_->addSubWindow(project);
            project->showMaximized();
            QAction *projectMenuAction = project->getProjectMenuAction();
            projectMenu_->addAction(projectMenuAction);
            projectActionGroup->addAction(projectMenuAction);
            connect(projectMenuAction, SIGNAL(triggered()), project, SLOT(show()));
            connect(projectMenuAction, SIGNAL(triggered()), project, SLOT(setFocus()));
            // [workaround_0]

            project->show();
        }
        else
        {
            //           project->close();
            delete project;
        }
    }
    return;
}

/*! \brief Triggers the creation of a new project.
*
* Tries to open a file. On success, the new project will be
* displayed. On failure, the new project will be deleted again.
* If the file has already been opened, its window will be
* activated.
*/
void
MainWindow::open(QString fileName, ProjectWidget::FileType type)
{
    if (!fileName.isEmpty())
    {
        // Check if file has already been opened... //
        //
        QMdiSubWindow *existing = findProject(fileName);
        if (existing)
        {
            mdiArea_->setActiveSubWindow(existing);
            return;
        }

        // ... otherwise create new project //
        //

        // [workaround_0] //
        // project->close() did not reliably delete the mdi subwindow when the file could not be loaded.
        // This workaround adds the project widget only after a successful loading.
        // original code:
        // ProjectWidget* project = createProject();
        ProjectWidget *project = new ProjectWidget(this);
        // [workaround_0]

        if (project->loadFile(fileName, type))
        {
            statusBar()->showMessage(tr("File has been loaded."), 0);

            // [workaround_0]
            mdiArea_->addSubWindow(project);
            project->showMaximized();
            QAction *projectMenuAction = project->getProjectMenuAction();
            projectMenu_->addAction(projectMenuAction);
            projectActionGroup->addAction(projectMenuAction);
            connect(projectMenuAction, SIGNAL(triggered()), project, SLOT(show()));
            connect(projectMenuAction, SIGNAL(triggered()), project, SLOT(setFocus()));
            // [workaround_0]

            project->show();
        }
        else
        {
            //   project->close();
            delete project;
        }
    }
    return;
}

void
MainWindow::openXODR()
{
    QString fileName = QFileDialog::getOpenFileName(this, "", "", tr("xodr files (*.xodr)"));
    if (!fileName.isEmpty())
    {
        open(fileName, ProjectWidget::FileType::FT_OpenDrive);
    }
}

void
MainWindow::openXOSC()
{
    QString fileName = QFileDialog::getOpenFileName(this, "", "", tr("xosc files (*.xosc)"));
    if (!fileName.isEmpty())
    {
        open(fileName, ProjectWidget::FileType::FT_OpenScenario);
    }
}


/*! \brief Opens an additional tile and adds to the project
*
*
*/
void
MainWindow::openTile()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {
        openTile(fileName);
    }
    return;
}

/*! \brief Open an additional tile and add to the project
*
*
*/
void
MainWindow::openTile(QString fileName)
{
    ProjectWidget *active = getActiveProject();
    if (active)
    {
        active->loadTile(fileName);
    }
    else
    {
        open(fileName);
    }
    return;
}

void
MainWindow::mergeXOSC()
{
    QString fileName = QFileDialog::getOpenFileName(this, "", "", tr("xosc files (*.xosc)"));
    if (!fileName.isEmpty())
    {
        if (getActiveProject())
        {
            getActiveProject()->loadFile(fileName, ProjectWidget::FileType::FT_OpenScenario);
        }
        else
        {
            open(fileName, ProjectWidget::FileType::FT_OpenScenario);
        }
    }
}

/*! \brief Tells the active project to save itself.
*
*/
void
MainWindow::save()
{
    if (getActiveProject() && getActiveProject()->save())
    {
        statusBar()->showMessage(tr("File has been saved."), 0);
    }
    return;
}

/*! \brief Tells the active project to save itself.
*/
void
MainWindow::saveAs()
{
    if (getActiveProject() && getActiveProject()->saveAs())
    {
        statusBar()->showMessage(tr("File has been saved."), 0);
    }
    return;
}

void
MainWindow::saveAsXODR()
{
    ProjectWidget *project = getActiveProject();
    if (project)
    {
        QString fileName = QFileDialog::getSaveFileName(this, "", "", tr("xodr files (*.xodr)"));
        if (!fileName.isEmpty())
        {
            if (project->saveFile(fileName, ProjectWidget::FileType::FT_OpenDrive))
            {
                statusBar()->showMessage(tr("File has been saved."), 0);
            }
        }
    }
}

void
MainWindow::saveAsXOSC()
{
    ProjectWidget *project = getActiveProject();
    if (project)
    {
        QString fileName = QFileDialog::getSaveFileName(this, "", "", tr("xosc files (*.xosc)"));
        if (!fileName.isEmpty())
        {
            if (project->saveFile(fileName, ProjectWidget::FileType::FT_OpenScenario))
            {
                statusBar()->showMessage(tr("File has been saved."), 0);
            }
        }
    }
}

/*! \brief Tells the active project to save itself.
*
*/
void
MainWindow::exportSpline()
{
    if (getActiveProject() && getActiveProject()->exportSpline())
    {
        statusBar()->showMessage(tr("File has been exported."), 0);
    }
    return;
}

void
MainWindow::changeSettings()
{
    if (getActiveProject())
    {
        projectionSettings->show();
    }
    return;
}

void
MainWindow::changeOSCSettings()
{
    if (getActiveProject())
    {
        oscSettings->show();
    }
    return;
}

void
MainWindow::changeImportSettings()
{
    if (getActiveProject())
    {
        importSettings->show();
    }
    return;
}

void
MainWindow::changeExportSettings()
{
    if (getActiveProject())
    {
        exportSettings->show();
    }
    return;
}

void
MainWindow::changeCOVERConnection()
{
    if (getActiveProject())
    {
#ifdef COVER_CONNECTION
        coverConnection->show();
#endif
    }
    return;
}

void
MainWindow::openCOVERSettings()
{
    if (getActiveProject())
    {
#ifdef COVER_CONNECTION
        /*if(coverConnection->getConnected())
        {
            fileSettings->show();
            fileSettings->getTabWidget()->setCurrentWidget(coverConnection);
        }*/
        coverConnection->setConnected(!(coverConnection->getConnected()));
#endif
    }
}

void
MainWindow::changeLODSettings()
{
    if (getActiveProject())
    {
        lodSettings->show();
    }
    return;
}


void
MainWindow::changeFileSettings()
{
    if (getActiveProject())
    {
        fileSettings->show();
    }
}


/*! \brief load Intermap xyz files.
*
*/
void
MainWindow::importIntermapRoad()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {

        ProjectWidget *project = getActiveProject();
        if (project == NULL)
        {
            project = createProject();
        }

        if (project->importIntermapFile(fileName))
        {
            statusBar()->showMessage(tr("File has been imported."), 0);
            project->show();
        }
    }
    return;
}

/*! \brief load CSV Road file.
*
*/
void
MainWindow::importCSVRoad()
{
    ProjectWidget *project = getActiveProject();
    if (project == NULL)
    {
        project = createProject();
        project->newFile(createProjectFilename());
    }
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {


        if (project->importCSVRoadFile(fileName))
        {
            statusBar()->showMessage(tr("File has been imported."), 0);
            project->show();
        }
    }
    return;
}

/*! \brief load CSV Sign file.
*
*/
void
MainWindow::importCSVSign()
{
    ProjectWidget *project = getActiveProject();
    if (project == NULL)
    {
        project = createProject();
        project->newFile(createProjectFilename());
    }
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {


        if (project->importCSVSignFile(fileName))
        {
            statusBar()->showMessage(tr("File has been imported."), 0);
            project->show();
        }
    }
    return;
}

/*! \brief load CarMaker Road file.
*
*/
void
MainWindow::importCarMakerRoad()
{
    ProjectWidget *project = getActiveProject();
    if (project == NULL)
    {
        project = createProject();
        project->newFile(createProjectFilename());
    }
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {


        if (project->importCarMakerFile(fileName))
        {
            statusBar()->showMessage(tr("File has been imported."), 0);
            project->show();
        }
    }
    return;
}

/*! \brief load OpenStreetMap OSM file.
*
*/
void
MainWindow::importOSMFile()
{
    ProjectWidget *project = getActiveProject();
    if (project == NULL)
    {
        project = createProject();
        project->newFile(createProjectFilename());
    }
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {
        osmi->setProject(project);
        if (osmi->importOSMFile(fileName))
        {
            statusBar()->showMessage(tr("File has been imported."), 0);
            project->show();
        }
    }
    return;
}
/*! \brief load OSM file.
*
*/
void
MainWindow::importOSMRoad()
{
    ProjectWidget *project = getActiveProject();
    if (project == NULL)
    {
        project = createProject();
        project->newFile(createProjectFilename());
    }
    osmi->setProject(project);
    osmi->exec();

    /*if (project->importOSMFile())
        {
            statusBar()->showMessage(tr("File has been imported."), 2000);
            project->show();
        }*/
    return;
}


/*! \brief Displays an "about" message box.
*/
void
MainWindow::about()
{
    QMessageBox::about(this, tr("About ODDLOT"), tr("The <b>oddlot: OpenDRIVE Designer</b> was initially created by Frank Naegele and is now further developed by the HLRS, University of Stuttgart.<br> Main contributors are Jutta Sauer, Uwe W&ouml;ssner<br> Since 2015, oddlot is now open source (LGPL2+) and available as part of the COVISE/OpenCOVER package on GitHub.<br> Credits: Train icon by Hannah Strobel from thenounproject.com<br> Tram icon by Arthur Lacote from thenounproject.com<br> Truck icon by Simon Child from thenounproject.com<br> RV icon by Yi Chen from thenounproject.com"));
    return;
}

/*! \brief \todo .
*/
void
MainWindow::openRecentFile()
{
    // TODO
    return;
}


/*! \brief Routes a ToolAction to the active project.
*
*/
void
MainWindow::toolAction(ToolAction *toolAction)
{
    if (getActiveProject())
    {
        getActiveProject()->toolAction(toolAction);
    }
}

void
MainWindow::settingsDockParentChanged(bool docked)
{
    if (docked)
    {
        settingsDock_->setMaximumWidth(400);
    }
    else
    {
        settingsDock_->setFixedWidth(200);
    }
}

//###########//
// GUI SETUP //
//###########//

/*! \brief Creates and adds a new project to the MDI area.
*
* This method is called by new.
* \note This function is not called by open(). See [workaround_0].
*/
ProjectWidget *
MainWindow::createProject()
{
    // New Project //
    //
    ProjectWidget *project = new ProjectWidget(this);
    mdiArea_->addSubWindow(project);
    // project->showMaximized();
    project->setWindowState(windowState() & Qt::WindowMaximized);

    // Project Menu Action //
    //
    QAction *projectMenuAction = project->getProjectMenuAction();
    projectMenu_->addAction(projectMenuAction);
    projectActionGroup->addAction(projectMenuAction);

    // setFocus calls QMdiArea::subWindowActivated(QMdiSubWindow * window)
    connect(projectMenuAction, SIGNAL(triggered()), project, SLOT(show()));
    connect(projectMenuAction, SIGNAL(triggered()), project, SLOT(setFocus()));

    // save current editor and tool in toolmanager
// toolManager_->addProjectEditingState(project);
 //   toolManager_->resendCurrentTool(project);
// toolManager_->resendStandardTool(project);

    return project;
}

/*! \brief Activates the current project.
*/
void
MainWindow::activateProject()
{
    ProjectWidget *project = getActiveProject();

    if (!project)
    {
        emit(hasActiveProject(false));
    }
    else
    {
        emit(hasActiveProject(true));

        // Set the current project marked in the project menu //
        //
        project->getProjectMenuAction()->setChecked(true);

        // Deactivate last project //
        //
        ProjectWidget *lastProject = getLastActiveProject();
        if (lastProject)
        {
            lastProject->setEditor(ODD::ENO_EDITOR);
            lastProject->setProjectActive(false);
        }

        // Tell project that it has been activated //
        //
        project->setProjectActive(true);

        // Pass currently selected tools //
        //
        toolManager_->resendCurrentTool(project);

        // Pass selected project to signal treewidget //
        //
        if (signalTree_)
        {
            signalTree_->setActiveProject(project);
        }
    }
}

/*! \brief Returns the currently active ProjectWidget.
*
*/
ProjectWidget *
MainWindow::getActiveProject()
{

    // If there is an active SubWindow, return it as a ProjectWidget //
    //
    QMdiSubWindow *activeSubWindow = mdiArea_->currentSubWindow();
    if (activeSubWindow)
    {
        return qobject_cast<ProjectWidget *>(activeSubWindow->widget());
    }
    else
    {
        return NULL;
    }
}

/*! \brief Returns the last active ProjectWidget.
*
*/
ProjectWidget *
MainWindow::getLastActiveProject()
{
    QList<QMdiSubWindow *> windowList = mdiArea_->subWindowList(QMdiArea::WindowOrder::ActivationHistoryOrder);

    if (windowList.size() > 1)
    {
        QMdiSubWindow *lastActiveWindow = windowList.at(windowList.size() - 2);
        return qobject_cast<ProjectWidget *>(lastActiveWindow->widget());
    }

    return NULL;
}

const QString
MainWindow::createProjectFilename()
{
    // Create a unique name by counting up numbers.
    static int documentNumber = 0;
    QString fileName;
    do
    {
        ++documentNumber;
        fileName = tr("untitled%1").arg(documentNumber);
    } while (findProject(fileName + ".xodr"));

    return fileName;
}

/*! \brief Returns the project subwindow of a already opened file (if possible).
*/
QMdiSubWindow *
MainWindow::findProject(const QString &fileName)
{
    // CanonicalFilePath //
    //
    // Absolute path without symbolic links or redundant "." or ".." elements.
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();
    if (canonicalFilePath.length() == 0) // file not found
        canonicalFilePath = fileName;

    // Search for project with that file name //
    //
    foreach(QMdiSubWindow * window, mdiArea_->subWindowList())
    {
        ProjectWidget *project = qobject_cast<ProjectWidget *>(window->widget());
        if (project->getFileName() == canonicalFilePath)
        {
            return window;
        }
    }

    // None found //
    //
    return NULL;
}

/*! \brief Set the widget of the Tree View.
*
* If NULL is passed, an empty widget will be displayed.
*/
void
MainWindow::setProjectTree(QWidget *widget)
{
    if (widget)
    {
        treeDock_->setWidget(widget);
    }
    else
    {
        treeDock_->setWidget(emptyTreeWidget_);
    }
}

/*! \brief Set the widget of the Tree View.
*
* If NULL is passed, an empty widget will be displayed.
*/
void
MainWindow::setSignalTree(QWidget *widget)
{
    if (widget)
    {
        signalsDock_->setWidget(widget);
    }
    else
    {
        signalsDock_->setWidget(emptyTreeWidget_);
    }
}

/*! \brief Set the widget of the Tree View.
*
* If NULL is passed, an empty widget will be displayed.
*/
void
MainWindow::setErrorMessageTree(QWidget *widget)
{
    if (widget)
    {
        errorDock_->setWidget(widget);
    }
    else
    {
        errorDock_->setWidget(emptyMessageWidget_);
    }
}

void
MainWindow::showDock(ODD::EditorId editor)
{
    switch (editor)
    {
    case ODD::EOS:
        if (!catalogsDock_.empty())
        {
            for (int i = 0; i < catalogsDock_.size(); i++)
            {
                catalogsDock_.at(i)->show();
            }
            catalogsDock_.first()->raise();
        }
        break;
    case ODD::ESG:
        signalsDock_->show();
        signalsDock_->raise();
        break;
    default:
        treeDock_->raise();
    }
}

void
MainWindow::hideDock(ODD::EditorId editor)
{
    switch (editor)
    {
    case ODD::EOS:
        if (!catalogsDock_.empty())
        {
            for (int i = 0; i < catalogsDock_.size(); i++)
            {
                catalogsDock_.at(i)->hide();
            }
        }
        break;
    case ODD::ESG:
        signalsDock_->hide();
        break;
    default:
        // make compiler happy
        break;
    }
}

/*! \brief Set the widget of the Settings View.
*
* If NULL is passed, an empty widget will be displayed.
*/
void
MainWindow::setProjectSettings(QWidget *widget)
{
    if (widget)
    {
        settingsDock_->setWidget(widget);
    }
    else
    {
        settingsDock_->setWidget(emptySettingsWidget_);
    }
}


//################//
// EVENTS         //
//################//

void
MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type())
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
    mdiArea_->closeAllSubWindows();
    if (mdiArea_->currentSubWindow())
    {
        event->ignore();
    }
    else
    {
        // TODO
        //writeSettings();
        event->accept();
    }
}

//################//
// Constructors   //
//################//
MainWindow::UndoView::UndoView(QUndoGroup *group, QWidget *parent)
    :QUndoView(group, parent) 
{
    mainWindow_ = dynamic_cast<MainWindow *>(parent);
}

void
MainWindow::UndoView::mouseReleaseEvent(QMouseEvent *event)
{
   mainWindow_->hideParameterSettings();
}

