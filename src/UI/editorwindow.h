#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QDockWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QLabel>
#include <QProcess>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Settings");
        resize(400, 300);
    }
};

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("About Specter Engine");
        resize(400, 300);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Specter Engine Editor\nVersion 1.0", this));
    }
};

class EditorWindow : public QMainWindow {
    Q_OBJECT

public:
    EditorWindow(const QString &projectPath, QWidget *parent = nullptr);

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
    void togglePlaceholder();

private:
    void setupUI();
    void setupMenuBar();
    void setupSceneView();
    void setupHierarchyPanel();
    void setupInspectorPanel();
    void setupAssetBrowser();
    void setupModulesPanel();
    void setupStatusBar();

    QString projectPath;
    QProcess *codeEditorProcess;

    // Док-виджеты
    QDockWidget *hierarchyDock;
    QDockWidget *inspectorDock;
    QDockWidget *assetBrowserDock;
    QDockWidget *modulesDock;

    // Центральный виджет (Сцена)
    QWidget *sceneViewWidget;
    QVBoxLayout *sceneLayout; // Для управления содержимым сцены
    QLabel *sceneLabel; // Метка сцены
    QLabel *placeholderWidget; // Placeholder
    bool placeholderVisible; // Флаг состояния Placeholder
    QAction *placeholderAction; // Действие для переключения Placeholder

    // Статус-бар
    QStatusBar *statusBar;
};

#endif // EDITORWINDOW_H