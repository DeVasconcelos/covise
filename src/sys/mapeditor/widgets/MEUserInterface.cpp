/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QMenu>
#include <QDockWidget>
#include <QLibrary>
#include <QToolBar>
#include <QStatusBar>
#include <QPushButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QSpinBox>
#include <QWhatsThis>
#include <QInputDialog>
#include <QProgressBar>
#include <QKeyEvent>
#include <QMimeData>
#include <QTimer>
#include <QScreen>

#include <messages/CRB_EXEC.h>
#include <config/coConfig.h>
#include <covise/covise_msg.h>
#include <util/coSpawnProgram.h>

#include "MEUserInterface.h"
#include "MEGraphicsView.h"
#include "MEMessageHandler.h"
#include "MEFileBrowser.h"
#include "MEModuleTree.h"
#include "MERegistry.h"
#include "color/MEColorMap.h"
#include "handler/MEMainHandler.h"
#include "handler/MENodeListHandler.h"
#include "handler/MEHostListHandler.h"
#include "handler/MEFavoriteListHandler.h"
#include "dataObjects/MEDataViewer.h"
#include "controlPanel/MEControlPanel.h"
#include "modulePanel/MEModulePanel.h"
#include "gridProxy/MEGridProxy.h"
#include "widgets/MEPreference.h"

#include "TUIApplication.h"

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

const int colorNo = 4;
static QColor msgColor[colorNo] = { Qt::black, Qt::darkGreen, Qt::blue, Qt::red };

MEUserInterface *MEUserInterface::m_singleton = nullptr;

/*!
    \class MEUserInterface
    \brief This class provides the main widgets for the MapEditor and
    handles events from menubar, toolbar ...

*/

MEUserInterface::MEUserInterface(MEMainHandler *handler)
    : QMainWindow()
    , m_graphicsView(MEGraphicsView::instance())
    , m_showDataViewerCfg(m_mapConfig.value("Session", "showDataViewer", false))
    , m_showTabletUICfg(m_mapConfig.value("Session", "showTabletUI", false))
    , m_showToolbarCfg(m_mapConfig.value("Session", "showToolbar", false))
    , m_showMessageAreaCfg(m_mapConfig.value("Session", "showMessageAre", false))
    , m_showControlPanelCfg(m_mapConfig.value("Session", "showControlPanel", false))
    , m_renderName("")
    , m_mainHandler(handler)
    , m_mapConfig(m_mainHandler->getConfig())
{
    assert(!m_singleton);
    m_singleton = this;
}

//!
MEUserInterface *MEUserInterface::instance()
//!
{
    assert(m_singleton);
    return m_singleton;
}

bool restoreSettings(QSettings &settings, QWidget* widget, const std::string &name)
{
    auto moduleParameter = settings.value(name.c_str());
    if(!moduleParameter.isValid())
        return false;
    widget->restoreGeometry(moduleParameter.toByteArray());
    return true;
}

void MEUserInterface::init()
{
    makeMainWidgets();
    createActions();
    m_graphicsView->init();
    createToolbar();
    createMenubar();
    // set the logo
    setWindowIcon(m_mainHandler->pm_logo);
    // try to figure out a reasonable window position and size

    if(!restoreSettings(MEMainHandler::instance()->getUserBehaviour(), this, "mainwindowGeo"))
    {
        move(QApplication::primaryScreen()->geometry().topLeft());
        resize(800, 550);
    }

    restoreSettings(MEMainHandler::instance()->getUserBehaviour(), MEModulePanel::instance(), "moduleParameter");
    restoreSettings(MEMainHandler::instance()->getUserBehaviour(), m_bottomDockWindow, "messageArea");
    restoreSettings(MEMainHandler::instance()->getUserBehaviour(), m_mainRight, "controlPanel");

    m_testScreenOffset = true; 
    
    // react on a selected developer mode inside the settings
    connect(m_mainHandler, SIGNAL(developerMode(bool)), this, SLOT(developerMode(bool)));

    m_foregroundCount = 0;
    bringApplicationToForeground();
}

// see Qt Creator: RunControl::bringApplicationToForegroundInternal()
void MEUserInterface::bringApplicationToForeground()
{
#ifdef Q_OS_MAC
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    if (SetFrontProcess(&psn) == procNotFound && m_foregroundCount < 15)
    {
        // somehow the mac/carbon api says
        // "-600 no eligible process with specified process id"
        // if we call SetFrontProcess too early
        ++m_foregroundCount;
        QTimer::singleShot(200, this, SLOT(bringApplicationToForeground()));
        return;
    }
#endif
}

//!
//! enable/diable the developermode (show dataviewer and snapshot)
//!
void MEUserInterface::developerMode(bool state)
{
    m_showDataViewer_a->setVisible(state);
    m_snapshot_a->setVisible(state);
}

//!
//! creates central tab widgets, parameter window and message area
//!

void MEUserInterface::makeMainWidgets()
{

    // make the main central widget
    m_main = new QWidget(this);

    //make the center widgets
    makeCenter();

    //make the module parameter window
    makeParameterWindow();

    //make the bottom widgets
    makeMessageArea();

    // Create an label and a message in the status bar
    // The message is updated when buttons are clicked etc.
    // Use statusbar also for Chat
    makeStatusBar();

    // set the central widget
    setCentralWidget(m_main);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    connect(m_graphicsView, SIGNAL(usingNode(const QString &)), m_moduleTree.get(), SLOT(moduleUseNotification(const QString &)));
    connect(m_mainHandler, SIGNAL(usingNode(const QString &)), m_moduleTree.get(), SLOT(moduleUseNotification(const QString &)));

    connect(m_moduleTree.get(), SIGNAL(showUsedNodes(const QString &, const QString &)),
            MENodeListHandler::instance(), SLOT(findUsedNodes2(const QString &, const QString &)));
    connect(m_moduleTree.get(), SIGNAL(showUsedCategory(const QString &)),
            MENodeListHandler::instance(), SLOT(findUsedNodes(const QString &)));

    connect(m_restoreListPB, SIGNAL(clicked()), m_moduleTree.get(), SLOT(restoreList()));
}

//!
//! create content for statusbar
//!

void MEUserInterface::makeStatusBar()
{
    //statusBar()->setFont( m_mainHandler->s_boldFont );

    QWidget *main = new QWidget(statusBar());
    QVBoxLayout *vbox = new QVBoxLayout(main);
    vbox->setSpacing(0);
    vbox->setContentsMargins(0, 1, 0, 1);

    // chat part
    m_chat = new QWidget();
    m_chat->hide();
    QHBoxLayout *hbox = new QHBoxLayout(m_chat);

    m_chatLineLabel = new QLabel("Chat:");
    hbox->addWidget(m_chatLineLabel, 0);
    vbox->setContentsMargins(1, 2, 1, 2);

    m_chatLine = new QLineEdit();
    m_chatLine->setFixedHeight(m_chatLine->sizeHint().height());
    m_chatLine->setModified(true);
    connect(m_chatLine, SIGNAL(returnPressed()), m_mainHandler, SLOT(chatCB()));
    hbox->addWidget(m_chatLine, 10);
    vbox->setContentsMargins(1, 2, 1, 2);

    // message info part
    m_info = new QWidget();
    m_info->hide();
    hbox = new QHBoxLayout(m_info);

    m_messageWindowPB = new QPushButton(QPixmap(m_mainHandler->pm_file), "");
    connect(m_messageWindowPB, SIGNAL(clicked()), this, SLOT(messagePBClicked()));
    hbox->addWidget(m_messageWindowPB, 0);

    m_messageWindowText = new QLabel();
    hbox->addWidget(m_messageWindowText, 10);

    vbox->addWidget(m_chat);
    vbox->addWidget(m_info);

    statusBar()->addWidget(main, 5);
}

//!
//! creates the visual programming area and the module browser
//!

void MEUserInterface::makeCenter()
{

    // center layout
    QVBoxLayout *box = new QVBoxLayout(m_main);
    box->setSpacing(2);
    box->setContentsMargins(1, 10, 1, 5);

    // create a tabbed widgets showing different folders
    m_tabBar = new METabBar();
    m_tabWidgets = new METabWidget(m_main);
    m_tabWidgets->setFocusPolicy(Qt::NoFocus);
    connect(m_tabWidgets, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    m_tabWidgets->setTabBar(m_tabBar);
    box->addWidget(m_tabWidgets);

    // create the combined first folder for the visual programming area
    m_mainArea = new QSplitter();

    // create left widget
    QWidget *makeLeft = new QWidget();
    makeLeftContent(makeLeft);
    m_mainArea->addWidget(makeLeft);
    m_mainArea->setStretchFactor(0, 0);

    // create visual programming area
    m_mainArea->addWidget(m_graphicsView);
    m_mainArea->setStretchFactor(1, 1);

    // create the right side
    // containing a label & a scroll are with the control panel
    m_mainRight = new QWidget();
    makeRightContent(m_mainRight);
    m_mainArea->addWidget(m_mainRight);
    m_mainArea->setStretchFactor(2, 0);

    // add first tab
    m_tabWidgets->addTab(m_mainArea, "Visual Programming");

    // create dataviewer folder
    m_dataPanel = MEDataViewer::instance();

    // show the programming area by default
    m_tabWidgets->setCurrentWidget(m_mainArea);
}

/*!
   creates the left part of the window (module browser)
*/

void MEUserInterface::makeLeftContent(QWidget *w)
{
    // main layout
    QVBoxLayout *vb = new QVBoxLayout(w);
    vb->setSpacing(2);
    vb->setContentsMargins(2, 4, 2, 4);

    // box containig a filter input for module tree
    QHBoxLayout *hb = new QHBoxLayout();
    QLabel *label = new QLabel("Filter: ");
    label->setFont(m_mainHandler->s_boldFont);
    m_filterLine = new MEFilterLineEdit();
    connect(m_filterLine, SIGNAL(textChanged(const QString &)), this, SLOT(filterCB()));
    hb->addWidget(label);
    hb->addWidget(m_filterLine, 1);
    vb->addLayout(hb);

    // create a pushbutton ativated when a filtered module list is shown
    m_restoreListPB = new QPushButton("Show all modules", w);
    m_restoreListPB->hide();
    vb->addWidget(m_restoreListPB);

    // create a stacked widget containing
    // 1. the full module tree list
    // 2. the filtered module tree list
    m_widgetStack = new QStackedWidget(w);

    m_moduleTree = std::make_unique<MEModuleTree>(MEMainHandler::instance()->getUserBehaviour());
    m_moduleTree->setFocusPolicy(Qt::NoFocus);
    m_widgetStack->addWidget(m_moduleTree.get());

    m_filterTree = std::make_unique<MEModuleTree>(MEMainHandler::instance()->getUserBehaviour());
    m_filterTree->setFocusPolicy(Qt::NoFocus);
    m_widgetStack->addWidget(m_filterTree.get());

    m_widgetStack->setCurrentWidget(m_moduleTree.get());
    connect(m_filterLine, SIGNAL(returnPressed()), m_filterTree.get(), SLOT(executeVisibleModule()));

    vb->addWidget(m_widgetStack);
}

/*!
  creates the right part of the window (control panel)
*/

void MEUserInterface::makeRightContent(QWidget *w)
{
    // create main layout
    QVBoxLayout *vb = new QVBoxLayout(w);
    vb->setSpacing(2);
    vb->setContentsMargins(2, 2, 2, 2);

    // set a proper label for widget
    QLabel *l = new QLabel("Control Panel", w);
    l->setAlignment(Qt::AlignCenter);
    l->setAutoFillBackground(true);
    l->setBackgroundRole(QPalette::Button);
    vb->addWidget(l);

    // create a acroll area containing the control panel
    QScrollArea *area = new QScrollArea(w);
    area->setWidgetResizable(true);
    area->setWidget(MEControlPanel::instance());
    vb->addWidget(area);

    // set window size
    restoreSettings(MEMainHandler::instance()->getUserBehaviour(), w, "controlPanel");
}

//!
//! create a docking window on the bottom side which contains the message area
//!
void MEUserInterface::makeMessageArea()
{

    m_bottomDockWindow = new QDockWidget("Message Area", this);
    m_bottomDockWindow->setAllowedAreas(Qt::BottomDockWidgetArea);

    // create text window
    m_infoWindow = new QTextEdit(m_bottomDockWindow);
    m_infoWindow->setReadOnly(true);
    m_bottomDockWindow->setWidget(m_infoWindow);
    m_bottomDockWindow->hide();

    QAction *closeAction = new QAction("close", m_bottomDockWindow);
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, SIGNAL(triggered(bool)), m_bottomDockWindow, SLOT(close()));
    m_bottomDockWindow->addAction(closeAction);

    // set window position & size
    restoreSettings(MEMainHandler::instance()->getUserBehaviour(), m_bottomDockWindow, "messageArea");
    addDockWidget(Qt::BottomDockWidgetArea, m_bottomDockWindow, Qt::Horizontal);

    m_showMessageArea_a = m_bottomDockWindow->toggleViewAction();
    m_showMessageArea_a->setChecked(m_showMessageAreaCfg->value());
    if (m_showMessageArea_a->isChecked())
        m_bottomDockWindow->show();
}

//!
//! create a window for module parameters as a stand alone dialog widget
//!
void MEUserInterface::makeParameterWindow()
{
    MEModulePanel::instance()->init();
    restoreSettings(MEMainHandler::instance()->getUserBehaviour(), MEModulePanel::instance(), "moduleParameter");
}

#define addMyAction(handler, action, text, callback, pixmap, shortcut, tip) \
    action = new QAction(text, this);                                       \
    connect(action, SIGNAL(triggered()), handler, SLOT(callback()));        \
    if (pixmap)                                                             \
        action->setIcon(QPixmap(pixmap));                                   \
    if (shortcut != 0)                                                      \
        action->setShortcut(shortcut);                                      \
    if (!QString(tip).isEmpty())                                            \
        action->setToolTip(tip);

#define addShowAction(action, text, ocstate, callback)                  \
    action = new QAction(text, this);                                   \
    action->setCheckable(true);                                         \
    connect(action, SIGNAL(toggled(bool)), this, SLOT(callback(bool))); \
    action->setChecked(ocstate);

//!
//! create all actions for the menubar & toolbar
//!
void MEUserInterface::createActions()
{

    addMyAction(m_mainHandler, m_fileopen_a, "&Open...", openNet, ":/icons/fileopen32.png", QKeySequence::Open, "Load a new map");
    addMyAction(m_mainHandler, m_filenew_a, "&New", clearNet, ":/icons/filenew.png", QKeySequence::New, "Clear the visual programming area");
    addMyAction(m_mainHandler, m_filesave_a, "&Save", saveNet, ":/icons/filesave32.png", QKeySequence::Save, "Save the current map");
    addMyAction(m_mainHandler, m_filesaveas, "Save As...", saveAsNet, "", QKeySequence::SaveAs, "Save current map as new file");
    addMyAction(m_mainHandler, m_settings_a, "Se&ttings...", settingXML, ":/icons/settings.png", 0, "Configure the Map Editor");
    m_settings_a->setShortcuts(QList<QKeySequence>() << QKeySequence::Preferences << Qt::CTRL + Qt::Key_T);
    addMyAction(m_mainHandler, m_exec_a, "&Execute All", execNet, ":/icons/execall.png", 0, "Execute the whole pipeline");
    m_exec_a->setShortcuts(QList<QKeySequence>() << QKeySequence::Refresh << Qt::CTRL + Qt::Key_E);
    addMyAction(m_mainHandler, m_addpartner_a, "Manage &Partner...", addPartner, ":/icons/add_user.png", 0, "Add a partner (with userinterface)");
    addMyAction(m_mainHandler, m_undo_a, "Undo", undoAction, ":/icons/undo32.png", QKeySequence::Undo, "Undo last user action");
    addMyAction(m_mainHandler, m_deleteAll_a, "&Delete All", clearNet, "", 0, "Clear the visual programming area");
    addMyAction(this, m_gridproxy_a, "Grid Proxy...", gridProxy, "", 0, "");
    addMyAction(m_mainHandler, m_snapshot_a, "Snapshot", printCB, ":/icons/snapshot.png", 0, "Make a snapshot of the canvas");
    m_snapshot_a->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_P);

    addMyAction(m_mainHandler, m_about_qt_a, "About Qt...", aboutQt, "", 0, "");
    addMyAction(m_mainHandler, m_about_a, "About COVISE...", about, "", 0, "");
    addMyAction(m_mainHandler, m_reportbug_a, "Report Bug", reportbug, "", 0, "");
    addMyAction(m_mainHandler, m_tutorial_a, "Tutorial", tutorial, "", 0, "");
    addMyAction(m_mainHandler, m_usersguide_a, "User\'s Guide", usersguide, "", 0, "");
    addMyAction(m_mainHandler, m_moduleguide_a, "Module Reference Guide", moduleguide, "", 0, "");
    addMyAction(m_mainHandler, m_progguide_a, "Programming Guide", progguide, "", 0, "");

    addMyAction(m_graphicsView, m_layoutMap_a, "Layout", layoutMap, ":/icons/layout32.png", 0, "Automatically lay out modules");
    addMyAction(m_graphicsView, m_viewAll_a, "Fit to View", viewAll, ":/icons/viewall32.png", 0, "Fit pipeline to view");
    addMyAction(m_graphicsView, m_actionCopy, "Copy", copy, ":/icons/copy.png", QKeySequence::Copy, "Copy modules to the clipoard");
    addMyAction(m_graphicsView, m_actionCut, "Cut", cut, ":/icons/cut.png", QKeySequence::Cut, "Cut modules to the clipoard");
    addMyAction(m_graphicsView, m_actionPaste, "Paste", paste, ":/icons/paste.png", QKeySequence::Paste, "Paste modules from the clipoard");

    // action only used under covise

    addMyAction(m_mainHandler, m_master_a, "Become Session &Master", masterCB, ":/icons/master32.png", Qt::CTRL + Qt::Key_M, "Become the session master");
    //addMyAction(m_mainHandler, _setmirror,   "Set Mirror Hosts...",   setMirror,      0, 0, "");
    //addMyAction(m_mainHandler, _startmirror, "Start Mirroring Nodes", addMirrorNodes, 0, 0, "");
    //addMyAction(m_mainHandler, _delmirror,   "Remove Mirrored Nodes", delMirrorNodes, 0, 0, "");

    // special actions
    addMyAction(qApp, m_exit_a, "&Quit", closeAllWindows, ":/icons/quit.png", Qt::CTRL + Qt::Key_Q, "Close the Session");
    addMyAction(MENodeListHandler::instance(), m_selectAll_a, "Select All", selectAllNodes, "", QKeySequence::SelectAll, "");
    addMyAction(m_mainHandler, m_help_a, "", mapeditor, "", QKeySequence::HelpContents, "");

    // shortcuts for scaling the visual programming area

    addMyAction(m_graphicsView, m_view100_a, "Scale to 100%", scaleView100, "", Qt::CTRL + Qt::Key_1, "");
    addMyAction(m_graphicsView, m_view50_a, "Scale to 50%", scaleView50, "", Qt::CTRL + Qt::Key_2, "");
    this->addAction(m_view100_a);
    this->addAction(m_view50_a);

    // user can delete selected module nodes with "DEL" or "ENTF" or Backspace

    QList<QKeySequence> shortcuts;
    shortcuts << Qt::Key_Delete << Qt::Key_Backspace;
    addMyAction(m_mainHandler, m_keyDelete_a, "Delete", deleteSelectedNodes, "", 0, "");
    m_keyDelete_a->setShortcuts(shortcuts);
    m_whatsthis_a = QWhatsThis::createAction();

    // some toggled action

    m_execOnChange_a = new QAction("Execute on change", this);
    m_execOnChange_a->setIcon(QPixmap(":/icons/execonchange.png"));
    m_execOnChange_a->setCheckable(true);
    m_execOnChange_a->setChecked(false);
    connect(m_execOnChange_a, SIGNAL(toggled(bool)), MEModulePanel::instance(), SLOT(enableExecCB(bool)));
    connect(m_execOnChange_a, SIGNAL(toggled(bool)), m_mainHandler, SLOT(changeCB(bool)));

    bool ocstate = m_showControlPanelCfg->value();
    addShowAction(m_showControlPanel_a, "Control Panel", ocstate, showControlPanel);
    showControlPanel(ocstate);

    ocstate = m_showDataViewerCfg->value();
    addShowAction(m_showDataViewer_a, "Data Viewer", ocstate, showDataViewer);
    showDataViewer(ocstate);

    ocstate = m_showTabletUICfg->value();
    addShowAction(m_showTabletUI_a, "Tablet UI", ocstate, showTabletUI);
    showTabletUI(ocstate);

    // programming area is the main visible tab widget

    if (m_tabWidgets)
        m_tabWidgets->setCurrentWidget(m_mainArea);

    developerMode(m_mainHandler->isDeveloperMode());
}

//!
//! create all stuff for the menubar
//!
void MEUserInterface::createMenubar()
{

    // File Menu
    //----------------------------------------------------------------
    QMenu *filePopup = menuBar()->addMenu("&File");
    m_openRecentMenu = new QMenu("Open Recent");
    m_openAutosaveMenu = new QMenu("Open Autosaved");

    filePopup->addAction(m_filenew_a);
    filePopup->addAction(m_fileopen_a);
    filePopup->addMenu(m_openRecentMenu);
    filePopup->addMenu(m_openAutosaveMenu);
    filePopup->addAction(m_filesave_a);
    filePopup->addAction(m_filesaveas);
    filePopup->addAction(filePopup->addSeparator());
    filePopup->addAction(m_settings_a);
    filePopup->addAction(m_snapshot_a);
    filePopup->addAction(filePopup->addSeparator());
    filePopup->addAction(m_exit_a);

    m_openRecentMenu->addActions(m_mainHandler->recentMapList);
    m_openAutosaveMenu->addActions(m_mainHandler->autosaveMapList);
    if (!m_mainHandler->autosaveMapList.isEmpty())
    {
        m_openAutosaveMenu->addSeparator();
        m_openAutosaveMenu->addAction(m_mainHandler->m_deleteAutosaved_a);
    }
    m_fileActionList.append(m_filenew_a);
    m_fileActionList.append(m_filesave_a);
    m_fileActionList.append(m_filesaveas);
    m_fileActionList.append(m_exit_a);

    // Edit Menu
    //----------------------------------------------------------------
    QMenu *edit = menuBar()->addMenu("&Edit");

    m_editActionList.append(m_actionCut);
    m_editActionList.append(m_actionCopy);
    m_editActionList.append(m_actionPaste);
    m_editActionList.append(m_keyDelete_a);
    m_editActionList.append(filePopup->addSeparator());
    m_editActionList.append(m_selectAll_a);
    m_editActionList.append(m_deleteAll_a);
    m_editActionList.append(filePopup->addSeparator());
    m_editActionList.append(m_undo_a);

    edit->addActions(m_editActionList);

    // View Menu
    //----------------------------------------------------------------
    QMenu *tools = menuBar()->addMenu("&View");

    tools->addAction(m_showToolbar_a);
    tools->addAction(m_showMessageArea_a);
    tools->addAction(m_showDataViewer_a);
    tools->addAction(m_showTabletUI_a);
    tools->addAction(m_showControlPanel_a);

    // Pipeline  Menu
    //----------------------------------------------------------------
    QMenu *pipe = menuBar()->addMenu("&Pipeline");

    m_pipeActionList.append(m_exec_a);
    m_pipeActionList.append(m_execOnChange_a);
    m_pipeActionList.append(pipe->addSeparator());
    m_pipeActionList.append(m_viewAll_a);
    m_pipeActionList.append(m_layoutMap_a);

    pipe->addActions(m_pipeActionList);

    // CSCW Menu
    //----------------------------------------------------------------
    QMenu *session = menuBar()->addMenu("&CSCW");

    m_sessionActionList.append(m_master_a);
    m_sessionActionList.append(session->addSeparator());
    m_sessionActionList.append(m_addpartner_a);
    m_sessionActionList.append(session->addSeparator());
//sessionActionList.append(_setmirror);
//sessionActionList.append(_startmirror);
//sessionActionList.append(_delmirror);
    session->addActions(m_sessionActionList);

#ifdef HAVE_GLOBUS
    // Globus Menu
    //----------------------------------------------------------------
    QMenu *globus = menuBar()->addMenu("&Globus");
    globus->addAction(m_gridproxy_a);
#endif

    // Help menu
    //----------------------------------------------------------------
    QMenu *help = menuBar()->addMenu(("&Help"));

    help->addAction(m_tutorial_a);
    help->addAction(m_usersguide_a);
    help->addAction(m_moduleguide_a);
    help->addAction(m_progguide_a);
    help->addSeparator();
    help->addAction(m_reportbug_a);
    help->addSeparator();
    help->addAction(m_about_qt_a);
    help->addAction(m_about_a);
}

//!
//! create all stuff for the toolbar
//!
void MEUserInterface::createToolbar()
{

    // create a toolbar

    m_toolBar = new METoolBar();
    m_toolBar->layout()->setSpacing(2);
    m_toolBar->layout()->setContentsMargins(0, 0, 0, 0);
    m_toolBar->setAcceptDrops(true);
    addToolBar(m_toolBar);

    bool ocstate = m_showToolbarCfg->value();
    m_showToolbar_a = m_toolBar->toggleViewAction();
    m_showToolbar_a->setChecked(ocstate);

    // fill the toolbar with actions

    m_toolBarActionList.append(m_fileopen_a);
    m_toolBarActionList.append(m_filesave_a);
    m_toolBarActionList.append(m_exec_a);
    m_toolBarActionList.append(m_execOnChange_a);
    m_toolBarActionList.append(m_undo_a);
    m_toolBarActionList.append(m_whatsthis_a);

    m_toolBar->addActions(m_toolBarActionList);
    m_comboSeparator = m_toolBar->addSeparator();

    m_toolBarActionList.append(m_layoutMap_a);
    m_toolBar->addAction(m_layoutMap_a);

    // insert a zoom in/out box

    m_toolBarActionList.append(m_viewAll_a);
    m_toolBar->addAction(m_viewAll_a);

    QLabel *l = new QLabel("No. of visible array: ", m_toolBar);
    l->setFont(m_mainHandler->s_boldFont);
    m_comboLabel_a = m_toolBar->addWidget(l);
    m_comboLabel_a->setVisible(false);

    QWidget *w = new QWidget();
    QHBoxLayout *hb1 = new QHBoxLayout(w);

    QStringList list;
    list << ""
         << "25%"
         << "50%"
         << "75%"
         << "100%"
         << "125%"
         << "150%"
         << "200%";

    m_scaleViewBox = new QComboBox();
    m_scaleViewBox->addItems(list);
    m_scaleViewBox->setToolTip("<p>Visual programming area zoom</p>");
    m_scaleViewBox->setFocusPolicy(Qt::NoFocus);

    updateScaleViewCB(1);

    connect(m_scaleViewBox, SIGNAL(textActivated(const QString &)), this, SLOT(scaleViewCB(const QString &)));
    connect(m_graphicsView, SIGNAL(factorChanged(qreal)), this, SLOT(updateScaleViewCB(qreal)));
    hb1->addWidget(m_scaleViewBox);

    // no of visible data array (shown if data viewer is shown)

    m_visibleArray = new QSpinBox();
    m_visibleArray->setRange(1, 10);
    m_visibleArray->setValue(3);
    m_visibleArray->hide();
    m_visibleArray->setToolTip("<p>Maximum number of visible data arrays</p>");

    if (m_dataPanel)
        connect(m_visibleArray, SIGNAL(valueChanged(int)), m_dataPanel, SLOT(spinCB(int)));
    hb1->addWidget(m_visibleArray);

    m_toolBar->addWidget(w);
    m_favSeparator = m_toolBar->addSeparator();

    // create a second part with favorite modules
    // fill in the favorites from mapeditor.xml

    const char *text = "<p>The favorite list contains often used modules.</p>"
                       "<p>Press on a name and drag the appearing icon to the <b>Visual Programming Area</b>. "
                       "Relase the icon and the corresponding module will be started</p>";

    l = new QLabel("Favorites: ", m_toolBar);
    l->setFont(m_mainHandler->s_boldFont);
    l->setWhatsThis(text);
    l->setToolTip("<p>Drop items from the module browser</p>");
    m_favoriteLabel_a = m_toolBar->addWidget(l);

    MEFavoriteListHandler::instance()->createFavorites();
}

//!
//! store the session parameter (window positions, size ...)
//!
void MEUserInterface::storeSessionParam(bool store)
{


    // store values for data viewer, colormap, control panel & registry
    *m_showDataViewerCfg = m_showDataViewer_a->isChecked();
    *m_showControlPanelCfg = m_showControlPanel_a->isChecked();
    *m_showTabletUICfg = m_showTabletUI_a->isChecked();
    *m_showToolbarCfg = m_showTabletUI_a->isChecked();
    *m_showMessageAreaCfg = m_showMessageArea_a->isChecked();
    if (store)
    {
        MEMainHandler::instance()->getUserBehaviour().setValue("mainwindowGeo", saveGeometry());
        // module parameter window
        if (MEModulePanel::instance())
        {
            MEMainHandler::instance()->getUserBehaviour().setValue("moduleParameter", MEModulePanel::instance()->saveGeometry());

        }

        MEMainHandler::instance()->getUserBehaviour().setValue("messageArea", m_bottomDockWindow->saveGeometry());
        MEMainHandler::instance()->getUserBehaviour().setValue("controlPanel", m_mainRight->saveGeometry());
    }
}

//!
//! delete all selected nodes
//!
void MEUserInterface::deleteSelectedNodes()
{
    if (m_tabWidgets->count() == 1 || m_tabWidgets->currentWidget() == m_mainArea)
        m_graphicsView->deleteNodesCB();

    else if (m_tabWidgets->currentWidget() == m_colorMap)
        m_colorMap->removeMarker();
}

//!
//! a new filter for module is given
//!
void MEUserInterface::filterCB()
{
    QString text = m_filterLine->text().trimmed();
    if (!text.isEmpty())
    {
        m_filterTree->showMatchingItems(text);
        MENodeListHandler::instance()->showMatchingNodes(text);
    }
    else
    {
        switchModuleTree(m_moduleTree.get(), false);
    }
}

//!
//! show all matching canvas nodes and module in the tree for a given user search string
//!
void MEUserInterface::showMatchingNodes()
{
    bool ok = false;
    QString text = QInputDialog::getText(this,
                                         (m_mainHandler->framework + "::Search modules and/or categories"),
                                         ("Please enter the search string"),
                                         QLineEdit::Normal, QString(), &ok);

    text = text.trimmed();
    if (ok && !text.isEmpty())
    {
        m_filterTree->showMatchingItems(text);
        MENodeListHandler::instance()->showMatchingNodes(text);
    }
}

//!
//! create main filebrowser
//!
void MEUserInterface::openBrowser(const QString &title, const QString &mapName, int mode)
{

    if (m_mainBrowser == NULL)
    {
        m_mainBrowser = new MEFileBrowser(this);
        if (mapName.isEmpty())
            m_mainBrowser->lookupFile("", "net", MEFileBrowser::FB_OPEN);
        else
            m_mainBrowser->lookupFile("", mapName, MEFileBrowser::FB_OPEN);
    }
    m_mainBrowser->setWindowTitle(title);
    m_mainBrowser->setNetType(mode);
    m_mainBrowser->raise();
    m_mainBrowser->show();
}

//!
//! activate callback from status line
//!
void MEUserInterface::messagePBClicked()
{
    if (m_errorNumber != 0)
    {
        m_messageWindowPB->setText("no new messages");
        QPalette palette;
        palette.setBrush(m_messageWindowPB->foregroundRole(), Qt::black);
        m_messageWindowPB->setPalette(palette);
    }
    showMessageArea(true);

    m_errorLevel = m_errorNumber = 0;
}

void MEUserInterface::closeEvent(QCloseEvent *ce)
{
    m_mainHandler->closeApplication(ce);
}

//!
//! delete all nodes and reset depending stuff
//!
void MEUserInterface::reset()
{
    m_graphicsView->reset();
    m_infoWindow->clear();
    m_willStartRenderer = false;

/*if(m_miniUserInterface)
   {
   }*/

    m_renderer = NULL;
    if (MEDataViewer::instance())
        MEDataViewer::instance()->reset();
}

void MEUserInterface::quit()
{
    m_moduleTree->storeModuleHistory();
}


//!
//! show current map name
//!
void MEUserInterface::showMapName(const QString &text)
{
    m_miniUserInterface->statusBar()->showMessage(text);
}

//!
//! change Main Browser content
//!
void MEUserInterface::updateMainBrowser(const QStringList &list)
{
    if (m_mainBrowser)
        m_mainBrowser->updateTree(list);
}

//!
//! change Main Browser content
//!
void MEUserInterface::lookupResult(const QString &text, const QString &filename, QString &type)
{
    if (m_mainBrowser)
        m_mainBrowser->lookupResult(text, filename, type);
}

//!
//! add a new host
//!
void MEUserInterface::addHostToModuleTree(MEHost *nptr)
{
    m_moduleTree->addHostList(nptr);
}

//!
//! open the chat window
//!
void MEUserInterface::openChatWindow()
{
    if (!m_chat->isVisible())
        m_chat->show();
}

//!
//! show chat message
//!
void MEUserInterface::writeChatContent(const QStringList &list)
{
    if (list.count() < 3)
        return;

    QString name = MEHostListHandler::instance()->getIPAddress(list[1]);
    MEHost *host = MEHostListHandler::instance()->getHost(name);
    QColor color(Qt::black);
    if (host)
    {
        color = host->getColor();
        int hue, s, v;
        color.getHsv(&hue, &s, &v);
        color.setHsv(hue, 255, v);
    }
    QString buffer = "Chat:: ";
    buffer.append(list[2].section('>', -1, -1));
    writeInfoMessage(buffer, color);
}

//!
//! show no. of unread messages & last message
//!
void MEUserInterface::writeInfoMessage(const QString &text, const QColor &color)
{
    // append text to message area text widget
    m_infoWindow->setTextColor(color);
    static bool first = true;
    if (m_errorNumber == 0 && !first)
        m_infoWindow->append("---");
    first = false;
    m_infoWindow->append(text);

    // set new message number
    m_errorNumber++;

    // get highest error level for color
    QColor c = color;
    for (int i = 0; i < colorNo; i++)
    {
        if (color == msgColor[i])
        {
            m_errorLevel = qMax(m_errorLevel, i);
            c = msgColor[m_errorLevel];
            break;
        }
    }
    statusBar()->clearMessage();

    //! show current number of unread messages
    //! show last error message
    QPalette palette;
    palette.setBrush(m_messageWindowPB->foregroundRole(), c);
    m_messageWindowPB->setText(QString::number(m_errorNumber) + " message" + (m_errorNumber == 1 ? "" : "s"));
    m_messageWindowPB->setPalette(palette);

    palette.setBrush(m_messageWindowText->foregroundRole(), c);
    m_messageWindowText->setPalette(palette);
    m_messageWindowText->setText(text);
    if (!m_bottomDockWindow->isVisible())
        m_info->show();
}

//!
//! print an internal information into the message area
//!
void MEUserInterface::printMessage(const QString &text)
{
    if (statusBar())
        statusBar()->showMessage("Map Editor: " + text);

    if (m_infoWindow)
    {
        QString tmp = text;
        tmp.prepend("<font color=\"magenta\"> <b>Map Editor:</b> </font>");
        writeInfoMessage(tmp);
    }
}

//!
//! change Module Browser content when 'hide unused modules' setting has changed
//!
void MEUserInterface::hideUnusedModulesHasChanged()
{
    m_moduleTree->changeBrowserItems();
}

//!
//! reset status bar if a host was deleted
//!
void MEUserInterface::resetStatusBar()
{
    m_chat->hide();
    showMessageArea(false);
}

//!
//! show the message area
//!
void MEUserInterface::showMessageArea(bool state)
{
    if (state)
    {
        m_bottomDockWindow->resize(500, 200);
        m_bottomDockWindow->show();
        m_info->hide();
        m_showMessageArea_a->setChecked(true);
    }

    else
    {
        m_bottomDockWindow->hide();
        m_info->hide();
        m_showMessageArea_a->setChecked(false);
    }
}

//!
//! reset menu and toolbar if a host was deleted
//!
void MEUserInterface::setCollabItems(int activeHosts, bool master)
{
    m_master_a->setEnabled(!master);

    // collaborative mode
    if (activeHosts > 1)
    {
        m_toolBar->insertAction(m_whatsthis_a, m_master_a);
    }

    // local mode
    else
        m_toolBar->removeAction(m_master_a);
}

//!
//!
//!
QMenu *MEUserInterface::createPopupMenu()
{
    return NULL;
}

//!
//! show the control panel
//!
void MEUserInterface::showControlPanel(bool state)
{
    if (m_mainRight)
    {
        if (state)
            m_mainRight->show();

        else
            m_mainRight->hide();
    }
}

//!
//! show the tabletUI
//!
void MEUserInterface::showTabletUI(bool state)
{
    // don't block port if tablet ui is disabled
    if (state)
        state = m_showTabletUI_a->isChecked();

    if (m_tablet)
    {
        if (state)
        {
            if (!m_mainHandler->cfg_TabletUITabs->value())
            {
                if (m_tabWidgets->indexOf(m_tablet) == -1)
                {
                    m_tabWidgets->addTab(m_tablet, "Tablet UI");
                    m_tabWidgets->setTabToolTip(m_tabWidgets->indexOf(m_tablet), "This is the new beautiful OpenCOVER user interface");
                }
            }
            if (m_tabWidgets->indexOf(m_tablet) >= 0)
                m_tabWidgets->setCurrentWidget(m_tablet);
        }

        else
        {
            if (m_tabWidgets->indexOf(m_tablet) != -1)
                m_tabWidgets->removeTab(m_tabWidgets->indexOf(m_tablet));
            delete m_tablet;
            m_tablet = NULL;
        }
    }
}

//!
//! show the data object browser
//!
void MEUserInterface::showDataViewer(bool state)
{
    if (state)
    {
        if (m_tabWidgets->indexOf(m_dataPanel) == -1)
        {
            m_tabWidgets->addTab(m_dataPanel, "Data Viewer");
            m_tabWidgets->setTabToolTip(m_tabWidgets->indexOf(m_dataPanel), "Examine your data objects");
        }
        m_tabWidgets->setCurrentWidget(m_dataPanel);
    }

    else
    {
        if (m_tabWidgets->indexOf(m_dataPanel) != -1)
            m_tabWidgets->removeTab(m_tabWidgets->indexOf(m_dataPanel));
    }
}

//!
//! switch the module tree between full & filtered
//!
void MEUserInterface::switchModuleTree(MEModuleTree *tree, bool show)
{
    if (show)
        m_restoreListPB->show();
    else
        m_restoreListPB->hide();

    m_widgetStack->setCurrentWidget(tree);
}

//!
//! clear module filter and show all modules
//!
void MEUserInterface::resetModuleFilter()
{
    m_filterLine->setText("");
    m_moduleTree->restoreList();
    switchModuleTree(m_moduleTree.get(), false);
}

//!
//! user wants to delete the  render tab
//!
void MEUserInterface::removeRenderer()
{
    MEMessageHandler::instance()->sendMessage(covise::COVISE_MESSAGE_UI, "DEL_REQ\n" + m_renderName + "\n" + m_renderInstance + "\n" + m_renderHost);
}

//!
void MEUserInterface::gridProxy()
//!
{
    if (m_gridProxyBox == NULL)
        m_gridProxyBox = new MEGridProxy();

    m_gridProxyBox->show();
}

//!
//! restart the renderer outside the mapeditor
//!
void MEUserInterface::restartRenderer()
{
/*tabWidgets->removePage(renderer);
   int xx = m_graphicsView->childX(renderNode);
   int yy = m_graphicsView->childY(renderNode);
   requestNode(renderNode->getName(), renderNode->getIPAddress(), xx, yy, renderNode, MEUserInterface::MOVE);
   lastFolderClosed();*/
}

//!
//! enable execution button if one module is on the canvas
//!
void MEUserInterface::enableExecution(bool state)
{
    m_exec_a->setEnabled(state);
}

//!
//! enable undo button
//!
void MEUserInterface::enableUndo(bool state)
{
    if (m_mainHandler->isMaster())
        m_undo_a->setEnabled(state);
}

//!
//! dis/enabley cop, cut, paste, delete
//!
void MEUserInterface::changeEditItems(bool state)
{
    if (m_mainHandler->isMaster())
    {
        m_actionCopy->setEnabled(state);
        m_actionCut->setEnabled(state);
        m_keyDelete_a->setEnabled(state);
    }
}

//!
//! tab index changed
//!
void MEUserInterface::tabChanged(int index)
{
    bool data = m_tabWidgets->widget(index) == m_dataPanel;
    bool tui = m_tabWidgets->widget(index) == m_tablet;
    bool net = m_tabWidgets->widget(index) == m_mainArea;

    if (m_comboLabel_a)
        m_comboLabel_a->setVisible(data);
    if (m_visibleArray)
        data ? m_visibleArray->show() : m_visibleArray->hide();
    if (m_scaleViewBox)
        net ? m_scaleViewBox->show() : m_scaleViewBox->hide();
    if (m_viewAll_a)
        m_viewAll_a->setVisible(net);
    if (m_layoutMap_a)
        m_layoutMap_a->setVisible(net);
    MEFavoriteListHandler::instance()->setVisible(net);
    if (m_comboSeparator)
        m_comboSeparator->setVisible(!tui);
    if (m_favSeparator)
        m_favSeparator->setVisible(net);
}

//!
//! show the current scaling factor
//!
void MEUserInterface::updateScaleViewCB(qreal factor)
{
    int value = int(factor * 100.);
    QString tmp = QString::number(value) + "%";
    m_scaleViewBox->setItemText(0, tmp);
    m_scaleViewBox->setCurrentIndex(0);
    m_scaleViewBox->update();
}

//!
//! user has scaled the graphics view
//!
void MEUserInterface::scaleViewCB(const QString &text)
{
    QString value = text.section('%', 0, 0);
    qreal factor = value.toFloat() / 100.;
    m_graphicsView->resetTransform();
    m_graphicsView->scaleView(factor);
}

//!
//! change status of execute button
//!
void MEUserInterface::changeExecButton(bool state)
{
    m_execOnChange_a->setChecked(state);
}

//!
//! establish a minimal mapeditor if started with --minigui
//!
void MEUserInterface::setMiniGUI(bool state)
{
    m_miniGUI = state;

    // show mini GUI
    if (m_miniGUI)
    {
        hide();

        //define a new top dialog window & layout
        m_miniUserInterface = new QMainWindow();
        m_miniUserInterface->setWindowTitle(m_mainHandler->framework + " Control Panel");

        // set execution status to m_executeOnChange
        m_execOnChange_a->setChecked(true);
        m_mainHandler->changeCB(true);

        // set the logo
        m_miniUserInterface->setWindowIcon(m_mainHandler->pm_logo);
        m_miniUserInterface->move(600, 10);

        // define a toolbar
        m_miniToolbar = new METoolBar();
        m_miniUserInterface->addToolBar(m_miniToolbar);
        QAction *m_exit_a = new QAction("&Quit", this);
        m_exit_a->setIcon(QPixmap(":/icons/quit.png"));
        m_exit_a->setShortcut(Qt::CTRL + Qt::Key_Q);
        m_exit_a->setToolTip("Close the Session");
        connect(m_exit_a, SIGNAL(triggered()), this, SLOT(close()));
        m_miniToolbar->addAction(m_exit_a);
        m_miniToolbar->addAction(m_fileopen_a);
        m_miniToolbar->addAction(m_filesave_a);
        m_miniToolbar->addAction(m_exec_a);

        // define the central widget
        // reparent control panel window
        QWidget *main = new QWidget(m_miniUserInterface);
        QVBoxLayout *hb = new QVBoxLayout(main);
        MEControlPanel::instance()->setParent(main);
        hb->addWidget(MEControlPanel::instance(), 1);

        // show mini GUI
        m_miniUserInterface->setCentralWidget(main);
        m_miniUserInterface->resize(200, 200);
        m_miniUserInterface->show();
        m_miniUserInterface->raise();
    }

    else
        show();
}

//!
//! enable/disable menu, toolbars ... for the change of master state
//!
void MEUserInterface::switchMasterState(bool state)
{
    if (m_openRecentMenu == NULL)
        return;
    // reset items in menubar & toolbar
    m_openRecentMenu->setEnabled(state);
    m_openAutosaveMenu->setEnabled(state);
    for (int i = 0; i < m_fileActionList.count(); i++)
        m_fileActionList.at(i)->setEnabled(state);

    for (int i = 0; i < m_sessionActionList.count(); i++)
        m_sessionActionList.at(i)->setEnabled(state);

    for (int i = 0; i < m_editActionList.count(); i++)
        m_editActionList.at(i)->setEnabled(state);

    for (int i = 0; i < m_pipeActionList.count(); i++)
        m_pipeActionList.at(i)->setEnabled(state);

    for (int i = 0; i < m_toolBarActionList.count(); i++)
        m_toolBarActionList.at(i)->setEnabled(state);

    MEFavoriteListHandler::instance()->setEnabled(state);

    // allow quit if not initial host & no module is used
    if (state)
        m_exit_a->setEnabled(true);
    else
        m_exit_a->setEnabled(m_mainHandler->canQuitSession());

    // allow some icons in slave mode too
    m_viewAll_a->setEnabled(true);
    m_whatsthis_a->setEnabled(true);

    // master icon
    m_master_a->setEnabled(!state);

    // disable cut, copy, delete when starting
    if (MENodeListHandler::instance()->isListEmpty())
    {
        m_actionCopy->setEnabled(false);
        m_actionCut->setEnabled(false);
        m_keyDelete_a->setEnabled(false);
        m_undo_a->setEnabled(false);
    }

    // reset tab folder widgets
    m_filterTree->setEnabled(state);
    MEControlPanel::instance()->setMasterState(state);
    m_graphicsView->setMasterState(state);

    if (m_colorMap)
        m_colorMap->setEnabled(state);

    MEModulePanel::instance()->setMasterState(state);
}

//!
//! remove tablet after loosing the connection, called from TUIApplication
//!
void MEUserInterface::removeTabletUI()
{
    if (m_tablet)
    {
        m_tabWidgets->removeTab(m_tabWidgets->indexOf(m_tablet));
        delete m_tablet;
        m_tablet = NULL;
        m_showTabletUI_a->setEnabled(false);
    }
}

//!
//! answer to request from OpenCOVER
//!
void MEUserInterface::activateTabletUI()
{
    const char *text = "<p>The <b>Tablet Userinterface</b> contains items for navigation with the OpenCOVER.</p>";

    if (m_tabletUIisDead)
        return;

    // don't block port if tablet ui is disabled
    if (!m_showTabletUI_a->isChecked())
        return;

    if (m_miniGUI)
    {
        // create tablet widget if no exists
        if (!m_tablet)
        {
            QDockWidget *dw = new QDockWidget("Tablet User Interface", m_miniUserInterface);
            m_miniUserInterface->addDockWidget(Qt::RightDockWidgetArea, dw, Qt::Vertical);
            dw->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
            m_tablet = new TUIMainWindow(dw);
            dw->setWidget(m_tablet);
            m_tablet->show();
        }
    }

    else
    {
        // create tablet widget if no exists
        if (!m_tablet)
        {
            if (m_mainHandler->cfg_TabletUITabs->value())
                m_tablet = new TUIMainWindow(m_tabWidgets, m_tabWidgets);
            else
                m_tablet = new TUIMainWindow(m_tabWidgets);
            m_tablet->setWhatsThis(text);
        }
    }

    // retry to connect to server & show tabletUI in tab widgets
    if (m_tablet && !m_tablet->serverRunning())
    {
        int ierr = m_tablet->openServer();
        if (!m_miniGUI)
        {
            if (ierr != 0)
            {
                printMessage("Can't open socket connection for TabletUI");
                m_tablet = NULL;
                m_showTabletUI_a->setEnabled(false);
            }

            else
            {
                m_showTabletUI_a->setEnabled(true);
                printMessage("Opening socket connection for TabletUI");
                if (!m_mainHandler->cfg_TabletUITabs->value())
                {
                    if (m_showTabletUI_a->isChecked())
                    {
                        m_tabWidgets->addTab(m_tablet, "Tablet UI");
                        m_tabWidgets->setTabToolTip(m_tabWidgets->indexOf(m_tablet), "This is the new beautiful OpenCOVER user interface");
                    }
                }
            }
        }
    }
}

//!
//! this is the stuff for integrate an ViNCE Renderer inside the user interface
//!

void MEUserInterface::startRenderer(const covise::CRB_EXEC &arguments)
{

    m_willStartRenderer = true;
    if (m_renderer)
        return;

    // store render parameter
    m_renderName = arguments.name();
    m_renderHost = arguments.controllerIp();
    m_renderInstance = arguments.moduleId();

    // create argumets for starting
    // ignore first parameter
    auto args = covise::getCmdArgs(arguments);
    auto argV = covise::cmdArgsToCharVec(args);
    size_t argc = argV.size() - 1;
    auto argv = argV.data() + 1;
    
    // get sgrender lib
    QLibrary *lib = new QLibrary(m_mainHandler->getLibraryName());
    lib->unload();
    if (!lib->isLoaded())
    {
        qWarning() << "MEUserInterface::startRenderer err: lib not found";
        return;
    }

    // look if VPA already exists in tab widgets

    getRendererFunction = (VinceRendererWidgetCreate)lib->resolve("vinceRendererWidgetCreate");
    if (!getRendererFunction)
    {
        qWarning() << "MEUserInterface::startRenderer err: could not resolve create function";
        return;
    }
    m_renderer = (QWidget *)getRendererFunction(m_tabWidgets, argc, argv);
    m_tabWidgets->addTab(m_renderer, "ViNCE Renderer");
    m_tabWidgets->setTabToolTip(m_tabWidgets->indexOf(m_renderer), "This is your desktop renderer");

    // show current page
    m_tabWidgets->setCurrentWidget(m_renderer);

// send message to m_mainHandler, that a render tab inside the mapeditor is available
    QString tmp = "RENDERER_IMBEDDED_ACTIVE\n" + m_mainHandler->localIP + "\n" + m_mainHandler->localUser + "\nTRUE";
    MEMessageHandler::instance()->sendMessage(covise::COVISE_MESSAGE_UI, tmp);

    qDebug() << "________________MEUserInterface::startRenderer info: start renderer ";
    m_renderer->show();
    qApp->processEvents();
}

//!
//! renderer was deleted
//!
void MEUserInterface::stopRenderer(const QString &name, const QString &number, const QString &host)
{
    // reset variable
    if (m_renderer)
    {
        if (!m_renderName.isEmpty())
        {
            if (name == m_renderName && number == m_renderInstance && host == m_renderHost)
            {
                m_tabWidgets->removeTab(m_tabWidgets->indexOf(m_renderer));
                m_renderer = NULL;
            }

// send message to controller, that a render tab inside the mapeditor is availablee
            QString tmp = "RENDERER_IMBEDDED_ACTIVE\n" + m_mainHandler->localIP + "\n" + m_mainHandler->localUser + "\nFALSE";
            MEMessageHandler::instance()->sendMessage(covise::COVISE_MESSAGE_UI, tmp);

            m_willStartRenderer = false;
            qDebug() << "________________MEUserInterface::startRenderer info: stop renderer ";
        }
    }
}

//!
//! show the colormap editor
//!
void MEUserInterface::showColorMap(bool state)
{
    if (state)
    {
        if (m_tabWidgets->indexOf(m_colorMap) == -1)
        {
            m_tabWidgets->addTab(m_colorMap, "ColorMap Editor");
            m_tabWidgets->setTabToolTip(m_tabWidgets->indexOf(m_colorMap), "Build or modify a colormap");
        }

        m_tabWidgets->setCurrentWidget(m_colorMap);
    }

    else
    {
        if (m_tabWidgets->indexOf(m_colorMap) != -1)
            m_tabWidgets->removeTab(m_tabWidgets->indexOf(m_colorMap));
    }
}

//!
//! show the registry
//!
void MEUserInterface::showRegistry(bool state)
{
    if (state)
    {
        if (m_tabWidgets->indexOf(MERegistry::instance()) == -1)
        {
            m_tabWidgets->addTab(MERegistry::instance(), "Registry");
            m_tabWidgets->setTabToolTip(m_tabWidgets->indexOf(MERegistry::instance()), "Have a look into the registry");
        }

        m_tabWidgets->setCurrentWidget(MERegistry::instance());
    }

    else
    {
        if (m_tabWidgets->indexOf(MERegistry::instance()) != -1)
            m_tabWidgets->removeTab(m_tabWidgets->indexOf(MERegistry::instance()));
    }
}

void MEUserInterface::clearOpenAutosaveMenu()
{
    m_openAutosaveMenu->clear();
}

void MEUserInterface::insertIntoOpenRecentMenu(QAction *ac)
{
    m_openRecentMenu->insertAction(NULL, ac);
}

METabBar::METabBar(QWidget *parent)
    : QTabBar(parent)
{
}

// dont't show any tab if only VPA is inserted
QSize METabBar::tabSizeHint(int index) const
{
    if (count() == 1)
    {
        QSize size(0, 0);
        return size;
    }

    else
    {
        QSize size = QTabBar::tabSizeHint(index);
        return size;
    }
}

METabWidget::METabWidget(QWidget *parent)
    : QTabWidget(parent)
{
}

void METabWidget::setTabBar(METabBar *tb)
{
    QTabWidget::setTabBar(tb);
}

MEFilterLineEdit::MEFilterLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
}

bool MEFilterLineEdit::event(QEvent *ev)
{
    bool ignore = false;
    switch (ev->type())
    {
    case QEvent::ShortcutOverride:
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(ev);
        if (ke->matches(QKeySequence::Cut)
            || ke->matches(QKeySequence::Copy)
            || ke->matches(QKeySequence::Paste)
            || ke->matches(QKeySequence::SelectAll))
            ignore = true;
    }
    break;
    default:
        break;
    }

    if (ignore)
        return false;

    return QLineEdit::event(ev);
}

void MEFilterLineEdit::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Escape)
    {
        clear();
        ev->accept();
    }
    else
    {
        QLineEdit::keyPressEvent(ev);
    }
}

METoolBar::METoolBar(QWidget *parent)
    : QToolBar(parent)
{
    setWindowTitle("Toolbar");
    setIconSize(QSize(24, 24));
}

//------------------------------------------------------------------------
// is this drag allowed ?
//------------------------------------------------------------------------
void METoolBar::dragEnterEvent(QDragEnterEvent *event)
{
    // don't allow drops from outside the application
    if (event->source() == NULL)
        return;

    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") || event->mimeData()->hasText())
        event->accept();
}

//------------------------------------------------------------------------
// is this drag allowed ?
//------------------------------------------------------------------------
void METoolBar::dragMoveEvent(QDragMoveEvent *event)
{
    // don't allow drops from outside the application
    if (event->source() == NULL)
        return;

    else if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") || event->mimeData()->hasText())
        event->accept();
}

//------------------------------------------------------------------------
// is this drag allowed ?
//------------------------------------------------------------------------
void METoolBar::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

//------------------------------------------------------------------------
// reread the dragged module information
// append modulename to favorite list
//------------------------------------------------------------------------
void METoolBar::dropEvent(QDropEvent *event)
{

    // from module tree
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))
    {
        QByteArray encodedData = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        while (!stream.atEnd())
        {
            QString text;
            stream >> text;
            QString combiname = text.section(':', 3, 3) + ":" + text.section(':', 2, 2);
            MEFavoriteListHandler::instance()->addFavorite(combiname);
        }
        event->accept();
    }

    else if (event->mimeData()->hasText())
    {
        QString text = event->mimeData()->text();
        QString combiname = text.section(':', 3, 3) + ":" + text.section(':', 2, 2);
        MEFavoriteListHandler::instance()->addFavorite(combiname);
        event->accept();
    }
}
