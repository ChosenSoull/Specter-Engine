#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QDockWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QStatusBar>
#include <QDialog>
#include <QVBoxLayout>
#include <QProcess>
#include <QDebug>

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Settings");
        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *label = new QLabel("Settings Placeholder", this);
        layout->addWidget(label);
    }
};

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("About Specter Game Engine");
        QVBoxLayout *layout = new QVBoxLayout(this);

        // Логотип
        QLabel *logoLabel = new QLabel(this);
        QPixmap logo(":/resources/SpecterEngineLogo.png");
        if (!logo.isNull()) {
            logoLabel->setPixmap(logo.scaled(100, 100, Qt::KeepAspectRatio));
            logoLabel->setAlignment(Qt::AlignCenter);
        } else {
            logoLabel->setText("Logo Missing");
        }
        layout->addWidget(logoLabel);

        // Информация
        QLabel *infoLabel = new QLabel("Specter Game Engine\nCopyright © 2025\nAuthor: ChosenSoul", this);
        infoLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(infoLabel);
    }
};

class EditorWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit EditorWindow(const QString &projectPath, QWidget *parent = nullptr);

private:
    void setupUI();
    void setupSceneView();
    void setupHierarchyPanel();
    void setupInspectorPanel();
    void setupAssetBrowser();
    void setupConsolePanel();
    void setupModulesPanel();
    void setupStatusBar();
    void setupMenuBar();

private slots:
    void openProject();
    void saveProject();
    void addToProject();
    void openCodeEditor();
    void buildDebug();
    void buildRelease();
    void runProject();
    void showSettings();
    void showAbout();

private:
    QString projectPath;
    QWidget *sceneViewWidget;
    QDockWidget *hierarchyDock;
    QDockWidget *inspectorDock;
    QDockWidget *assetBrowserDock;
    QDockWidget *consoleDock;
    QDockWidget *modulesDock;
    QStatusBar *statusBar;
    QProcess *codeEditorProcess;
};

#endif