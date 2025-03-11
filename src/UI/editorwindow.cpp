#include "editorwindow.h"
#include <QVBoxLayout>
#include <QSettings>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QScrollArea>
#include <QFileSystemModel>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

EditorWindow::EditorWindow(const QString &projectPath, QWidget *parent)
    : QMainWindow(parent), projectPath(projectPath), codeEditorProcess(nullptr) {
    // Загружаем имя проекта из config.cfg
    QSettings config(projectPath + "/config.cfg", QSettings::IniFormat);
    config.beginGroup("Project");
    QString projectName = config.value("Name", "Unnamed Project").toString();
    config.endGroup();
    setWindowTitle(projectName + " - Specter Game Engine Editor");
    resize(1200, 800);

    // Устанавливаем логотип окна
    QIcon windowIcon(":/resources/SpecterEngineLogo.png");
    if (!windowIcon.isNull()) {
        setWindowIcon(windowIcon);
    } else {
        qWarning() << "Window icon not found! Check path: /resources/SpecterEngineLogo.png";
    }

    // Тёмная тема, как в VSCode
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(30, 30, 30));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(43, 43, 43));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Highlight, QColor(77, 77, 77));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    setPalette(palette);

    setupUI();
}

void EditorWindow::setupUI() {
    // Меню
    setupMenuBar();

    // Центральный виджет (Сцена)
    setupSceneView();

    // Панели
    setupHierarchyPanel();
    setupInspectorPanel();
    setupAssetBrowser();
    setupConsolePanel();
    setupModulesPanel();

    // Статус-бар
    setupStatusBar();
}

void EditorWindow::setupMenuBar() {
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // Стилизация меню под VSCode
    menuBar->setStyleSheet("QMenuBar { background-color: #252526; color: #D4D4D4; }"
                           "QMenuBar::item { padding: 5px 10px; }"
                           "QMenuBar::item:selected { background-color: #333333; }"
                           "QMenu { background-color: #252526; color: #D4D4D4; }"
                           "QMenu::item { padding: 5px 20px; }"
                           "QMenu::item:selected { background-color: #333333; }");

    // File Menu
    QMenu *fileMenu = menuBar->addMenu("File");
    fileMenu->addAction("Open Project", this, &EditorWindow::openProject);
    fileMenu->addAction("Save Project", this, &EditorWindow::saveProject);
    fileMenu->addAction("Add to Project", this, &EditorWindow::addToProject);
    fileMenu->addSeparator();
    fileMenu->addAction("Close", this, &EditorWindow::close);

    // Scene Menu
    QMenu *sceneMenu = menuBar->addMenu("Scene");
    sceneMenu->addAction("New Scene");
    sceneMenu->addAction("Load Scene");
    sceneMenu->addAction("Save Scene");

    // Edit Menu
    QMenu *editMenu = menuBar->addMenu("Edit");
    editMenu->addAction("Open Code Editor", this, &EditorWindow::openCodeEditor);
    editMenu->addSeparator();
    editMenu->addAction("Settings", this, &EditorWindow::showSettings);

    // Build Menu
    QMenu *buildMenu = menuBar->addMenu("Build");
    buildMenu->addAction("Build Debug", this, &EditorWindow::buildDebug);
    buildMenu->addAction("Build Release", this, &EditorWindow::buildRelease);
    buildMenu->addAction("Run", this, &EditorWindow::runProject);

    // About Menu
    QMenu *helpMenu = menuBar->addMenu("Help");
    helpMenu->addAction("About", this, &EditorWindow::showAbout);
}

void EditorWindow::setupSceneView() {
    sceneViewWidget = new QWidget(this);
    sceneViewWidget->setStyleSheet("background-color: #333333;");
    setCentralWidget(sceneViewWidget);

    QVBoxLayout *layout = new QVBoxLayout(sceneViewWidget);
    QLabel *sceneLabel = new QLabel("Scene View (Viewport)", this);
    sceneLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(sceneLabel);

    QLabel *placeholder = new QLabel("3D/2D Scene Placeholder", this);
    placeholder->setAlignment(Qt::AlignCenter);
    layout->addWidget(placeholder);
}

void EditorWindow::setupHierarchyPanel() {
    hierarchyDock = new QDockWidget("Scene Hierarchy", this);
    QTreeWidget *hierarchyTree = new QTreeWidget(this);
    hierarchyTree->setHeaderHidden(true);
    QStringList headers;
    headers << "Objects";
    hierarchyTree->setHeaderLabels(headers);
    hierarchyTree->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Object1"));
    hierarchyTree->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Object2"));
    hierarchyDock->setWidget(hierarchyTree);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);
}

void EditorWindow::setupInspectorPanel() {
    inspectorDock = new QDockWidget("Inspector", this);
    QWidget *inspectorWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(inspectorWidget);

    QLabel *objectName = new QLabel("Selected Object: None", this);
    layout->addWidget(objectName);

    QCheckBox *transformCheck = new QCheckBox("Transform", this);
    layout->addWidget(transformCheck);

    QLineEdit *positionX = new QLineEdit("0.0", this);
    layout->addWidget(positionX);

    QPushButton *addComponent = new QPushButton("Add Component", this);
    layout->addWidget(addComponent);

    layout->addStretch();
    inspectorDock->setWidget(inspectorWidget);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
}

void EditorWindow::setupAssetBrowser() {
    assetBrowserDock = new QDockWidget("Asset Browser", this);
    QListWidget *assetList = new QListWidget(this);
    assetList->addItem(new QListWidgetItem("Texture1.png"));
    assetList->addItem(new QListWidgetItem("Model1.obj"));
    assetBrowserDock->setWidget(assetList);
    addDockWidget(Qt::LeftDockWidgetArea, assetBrowserDock);
    tabifyDockWidget(assetBrowserDock, hierarchyDock);
}

void EditorWindow::setupConsolePanel() {
    consoleDock = new QDockWidget("Console", this);
    QTextEdit *consoleText = new QTextEdit(this);
    consoleText->setReadOnly(true);
    consoleText->append("Log: Engine started.");
    consoleDock->setWidget(consoleText);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
}

void EditorWindow::setupModulesPanel() {
    modulesDock = new QDockWidget("Modules", this);
    QListWidget *modulesList = new QListWidget(this);
    modulesList->addItem(new QListWidgetItem("Rendering Module"));
    modulesList->addItem(new QListWidgetItem("Physics Module"));
    modulesDock->setWidget(modulesList);
    addDockWidget(Qt::LeftDockWidgetArea, modulesDock);
    tabifyDockWidget(modulesDock, hierarchyDock);
}

void EditorWindow::setupStatusBar() {
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");
    statusBar->setStyleSheet("QStatusBar { background-color: #252526; color: #D4D4D4; }");
}

void EditorWindow::openProject() {
    QString dir = QFileDialog::getExistingDirectory(this, "Open Project Directory");
    if (!dir.isEmpty()) {
        QFile configFile(dir + "/config.cfg");
        if (!configFile.exists()) {
            QMessageBox::warning(this, "Error", "Selected directory does not contain a valid project (missing config.cfg)!");
            return;
        }
        projectPath = dir;
        QSettings config(projectPath + "/config.cfg", QSettings::IniFormat);
        config.beginGroup("Project");
        QString projectName = config.value("Name", "Unnamed Project").toString();
        config.endGroup();
        setWindowTitle(projectName + " - Specter Game Engine Editor");
    }
}

void EditorWindow::saveProject() {
    QMessageBox::information(this, "Save Project", "Project saved successfully!");
}

void EditorWindow::addToProject() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Add Files to Project", "", "All Files (*.*)");
    if (!files.isEmpty()) {
        QMessageBox::information(this, "Add to Project", "Files added: " + files.join(", "));
    }
}

void EditorWindow::openCodeEditor() {
    if (!codeEditorProcess) {
        codeEditorProcess = new QProcess(this);
    }
    // Запускаем VSCode в директории проекта (можно заменить на другой редактор)
    QString editorCommand = "code"; // Предполагается, что VSCode установлен и доступен в PATH
    QStringList arguments;
    arguments << projectPath;
    codeEditorProcess->start(editorCommand, arguments);
    if (!codeEditorProcess->waitForStarted()) {
        QMessageBox::warning(this, "Error", "Failed to open code editor! Ensure 'code' command is available.");
    }
}

void EditorWindow::buildDebug() {
    QMessageBox::information(this, "Build Debug", "Building project in Debug mode...");
}

void EditorWindow::buildRelease() {
    QMessageBox::information(this, "Build Release", "Building project in Release mode...");
}

void EditorWindow::runProject() {
    QMessageBox::information(this, "Run Project", "Running project...");
}

void EditorWindow::showSettings() {
    SettingsDialog dialog(this);
    dialog.exec();
}

void EditorWindow::showAbout() {
    AboutDialog dialog(this);
    dialog.exec();
}