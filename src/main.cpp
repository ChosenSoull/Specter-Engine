#include <QApplication>
#include "UI/startupdialog.h"
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    StartupDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        std::cout << "Выбран проект: " << dialog.getSelectedProjectPath().toStdString() << std::endl;
    } else {
        std::cout << "Программа закрыта без выбора проекта" << std::endl;
    }

    return app.exec();
}