#include "UI/startupdialog.h"
#include "UI/editorwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    StartupDialog startupDialog;
    startupDialog.show();

    // Подключаем сигнал projectSelected к открытию редактора
    QObject::connect(&startupDialog, &StartupDialog::projectSelected, [&startupDialog](const QString &path) {
        EditorWindow *editor = new EditorWindow(path);
        editor->show();
        // Закрываем StartupDialog после открытия редактора
        startupDialog.close();
    });

    return app.exec();
}