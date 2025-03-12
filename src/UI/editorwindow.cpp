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
#include <QInputDialog>
#include <QToolButton>
#include <QHBoxLayout>
#include <QTreeWidgetItem>
#include <QProcess>
#include <QDockWidget>

EditorWindow::EditorWindow(const QString &projectPath, QWidget *parent)
    : QMainWindow(parent), projectPath(projectPath), codeEditorProcess(nullptr), placeholderVisible(false) {
    // Загружаем имя проекта из config.cfg
    QSettings config(projectPath + "/config.cfg", QSettings::IniFormat);
    config.beginGroup("Project");
    QString projectName = config.value("Name", "Unnamed Project").toString();
    config.endGroup();
    setWindowTitle(projectName + " - Specter Engine Editor");
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

    // Добавляем логотип в начало QMenuBar
    QWidget *logoWidget = new QWidget(this);
    QHBoxLayout *logoLayout = new QHBoxLayout(logoWidget);
    logoLayout->setContentsMargins(5, 0, 5, 0); // Минимальные отступы
    QLabel *logoLabel = new QLabel(logoWidget);
    QPixmap logo(":/resources/SpecterEngineLogo.png");
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(20, 20, Qt::KeepAspectRatio));
    } else {
        logoLabel->setText("L");
    }
    logoLayout->addWidget(logoLabel);
    logoLayout->addStretch(); // Растягиваем, чтобы логотип был слева
    logoWidget->setLayout(logoLayout);
    menuBar->setCornerWidget(logoWidget, Qt::TopLeftCorner); // Логотип в левом верхнем углу

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
    // Добавляем действие для переключения Placeholder
    placeholderAction = editMenu->addAction("Activate Placeholder", this, &EditorWindow::togglePlaceholder);

    // Build Menu
    QMenu *buildMenu = menuBar->addMenu("Build");
    buildMenu->addAction("Build Debug", this, &EditorWindow::buildDebug);
    buildMenu->addAction("Build Release", this, &EditorWindow::buildRelease);
    buildMenu->addAction("Run", this, &EditorWindow::runProject);

    // Settings Action
    QAction *settingsAction = new QAction("Settings", this);
    connect(settingsAction, &QAction::triggered, this, &EditorWindow::showSettings);
    menuBar->addAction(settingsAction);

    // Help Menu
    QMenu *helpMenu = menuBar->addMenu("Help");
    helpMenu->addAction("About", this, &EditorWindow::showAbout);
}

void EditorWindow::setupSceneView() {
    sceneViewWidget = new QWidget(this);
    sceneViewWidget->setStyleSheet("background-color: #333333;");
    setCentralWidget(sceneViewWidget);

    sceneLayout = new QVBoxLayout(sceneViewWidget);
    sceneLabel = new QLabel("Scene View (Viewport)", this);
    sceneLabel->setAlignment(Qt::AlignCenter);
    sceneLayout->addWidget(sceneLabel);

    // Создаём Placeholder без родителя, чтобы он не отображался автоматически
    placeholderWidget = new QLabel("3D/2D Scene Placeholder", nullptr);
    placeholderWidget->setAlignment(Qt::AlignCenter);
    placeholderWidget->hide(); // Скрываем по умолчанию
}

void EditorWindow::togglePlaceholder() {
    if (placeholderVisible) {
        // Деактивируем Placeholder
        sceneLayout->removeWidget(placeholderWidget);
        placeholderWidget->hide(); // Явно скрываем
        sceneLayout->addWidget(sceneLabel);
        sceneLabel->show(); // Явно показываем
        placeholderVisible = false;
        placeholderAction->setText("Activate Placeholder");
    } else {
        // Активируем Placeholder
        sceneLayout->removeWidget(sceneLabel);
        sceneLabel->hide(); // Явно скрываем
        sceneLayout->addWidget(placeholderWidget);
        placeholderWidget->show(); // Явно показываем
        placeholderVisible = true;
        placeholderAction->setText("Deactivate Placeholder");
    }
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

    // Контекстное меню для объектов
    hierarchyTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(hierarchyTree, &QTreeWidget::customContextMenuRequested, this, [this, hierarchyTree](const QPoint &pos) {
        QTreeWidgetItem *item = hierarchyTree->itemAt(pos);
        if (item) {
            QMenu contextMenu(this);
            contextMenu.addAction("Rename", this, [item]() {
                bool ok;
                QString newName = QInputDialog::getText(nullptr, "Rename Object", "Enter new name:", QLineEdit::Normal, item->text(0), &ok);
                if (ok && !newName.isEmpty()) {
                    item->setText(0, newName);
                }
            });
            contextMenu.addAction("Delete", this, [item]() {
                delete item;
            });
            contextMenu.exec(QCursor::pos());
        }
    });

    hierarchyDock->setWidget(hierarchyTree);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);
}

void EditorWindow::setupInspectorPanel() {
    inspectorDock = new QDockWidget("Inspector", this);
    QWidget *inspectorWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(inspectorWidget);

    QLabel *objectName = new QLabel("Selected Object: None", this);
    layout->addWidget(objectName);

    // Пример редактируемых свойств
    QLineEdit *positionX = new QLineEdit("0.0", this);
    positionX->setPlaceholderText("Position X");
    layout->addWidget(positionX);

    QLineEdit *positionY = new QLineEdit("0.0", this);
    positionY->setPlaceholderText("Position Y");
    layout->addWidget(positionY);

    QLineEdit *positionZ = new QLineEdit("0.0", this);
    positionZ->setPlaceholderText("Position Z");
    layout->addWidget(positionZ);

    QPushButton *addComponent = new QPushButton("Add Component", this);
    layout->addWidget(addComponent);

    layout->addStretch();
    inspectorDock->setWidget(inspectorWidget);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
}

void EditorWindow::setupAssetBrowser() {
    assetBrowserDock = new QDockWidget("Asset Browser", this);
    QWidget *assetBrowserWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(assetBrowserWidget);

    // Строка поиска
    QLineEdit *searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText("Search assets...");
    layout->addWidget(searchBar);

    // Список ресурсов с иконками
    QListWidget *assetList = new QListWidget(this);
    assetList->addItem(new QListWidgetItem(QIcon(":/resources/texture_icon.png"), "Texture1.png"));
    assetList->addItem(new QListWidgetItem(QIcon(":/resources/model_icon.png"), "Model1.obj"));

    layout->addWidget(assetList);
    assetBrowserDock->setWidget(assetBrowserWidget);
    addDockWidget(Qt::LeftDockWidgetArea, assetBrowserDock);
}

void EditorWindow::setupModulesPanel() {
    modulesDock = new QDockWidget("Modules", this);
    QListWidget *modulesList = new QListWidget(this);
    modulesList->addItem(new QListWidgetItem("Rendering Module"));
    modulesList->addItem(new QListWidgetItem("Physics Module"));
    modulesDock->setWidget(modulesList);
    addDockWidget(Qt::LeftDockWidgetArea, modulesDock);
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
        setWindowTitle(projectName + " - Specter Engine Editor");
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
    QString editorCommand = "code";
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