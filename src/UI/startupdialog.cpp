#include "startupdialog.h"
#include "createprojectdialog.h" // Добавляем заголовок нового диалога
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QStyleFactory>
#include <QDebug>
#include <QIcon>
#include <QMouseEvent>

StartupDialog::StartupDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Specter Game Engine");
    setFixedSize(450, 350); // Увеличиваем ширину для размещения кнопок горизонтально

    // Установка иконки для окна
    QIcon windowIcon(":/resources/SpecterEngineLogo.png");
    if (!windowIcon.isNull()) {
        setWindowIcon(windowIcon);
    } else {
        qWarning() << "Иконка приложения не найдена! Проверьте путь: /resources/SpecterEngineLogo.png";
    }

    // Тёмная тема
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    qApp->setPalette(darkPalette);

    // Настройки для хранения проектов
    settings = new QSettings("../recent_projects.ini", QSettings::IniFormat, this);

    // Главный макет (вертикальный)
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Горизонтальный макет для списка и кнопок
    QHBoxLayout* contentLayout = new QHBoxLayout();

    // Список последних проектов (в виде текста с иконкой)
    projectsWidget = new QWidget(this);
    projectsLayout = new QVBoxLayout(projectsWidget);
    projectsLayout->setAlignment(Qt::AlignTop);
    contentLayout->addWidget(projectsWidget);

    // Горизонтальный макет для кнопок
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    createButton = new QPushButton(this);
    openButton = new QPushButton(this);

    // Настройка кнопки "Create Project" с иконкой
    QVBoxLayout* createButtonLayout = new QVBoxLayout(createButton);
    QLabel* createIconLabel = new QLabel(this);
    QPixmap createIcon(":/resources/create_icon.png");
    if (!createIcon.isNull()) {
        createIconLabel->setPixmap(createIcon.scaled(120, 120, Qt::KeepAspectRatio));
        createIconLabel->setAlignment(Qt::AlignCenter);
    } else {
        createIconLabel->setText("Icon missing");
    }
    QLabel* createTextLabel = new QLabel("Create Project", this);
    createTextLabel->setAlignment(Qt::AlignCenter);
    createButtonLayout->addWidget(createIconLabel, 1); // Добавляем растяжение для иконки
    createButtonLayout->addWidget(createTextLabel);
    createButton->setFixedSize(200, 250);

    // Настройка кнопки "Open Project" с иконкой
    QVBoxLayout* openButtonLayout = new QVBoxLayout(openButton);
    QLabel* openIconLabel = new QLabel(this);
    QPixmap openIcon(":/resources/open_icon.png");
    if (!openIcon.isNull()) {
        openIconLabel->setPixmap(openIcon.scaled(120, 120, Qt::KeepAspectRatio));
        openIconLabel->setAlignment(Qt::AlignCenter);
    } else {
        openIconLabel->setText("Icon missing");
    }
    QLabel* openTextLabel = new QLabel("Open Project", this);
    openTextLabel->setAlignment(Qt::AlignCenter);
    openButtonLayout->addWidget(openIconLabel, 1); // Добавляем растяжение для иконки
    openButtonLayout->addWidget(openTextLabel);
    openButton->setFixedSize(200, 250);

    connect(createButton, &QPushButton::clicked, this, &StartupDialog::onCreateProjectClicked);
    connect(openButton, &QPushButton::clicked, this, &StartupDialog::onOpenProjectClicked);

    buttonLayout->addStretch(); // Добавляем растяжение слева
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(openButton);
    buttonLayout->addStretch(); // Добавляем растяжение справа

    contentLayout->addLayout(buttonLayout);
    mainLayout->addLayout(contentLayout);

    // Загрузка последних проектов
    loadRecentProjects();
}

StartupDialog::~StartupDialog() {
    saveRecentProjects();
}

void StartupDialog::onCreateProjectClicked() {
    CreateProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        selectedProjectPath = dialog.getProjectPath();
        if (!selectedProjectPath.isEmpty()) {
            loadRecentProjects();
            accept();
        }
    }
}

void StartupDialog::onOpenProjectClicked() {
    QString dir = QFileDialog::getOpenFileName(this, "Open Project", "", "Project Files (*.proj)");
    if (!dir.isEmpty()) {
        selectedProjectPath = dir;
        loadRecentProjects();
        accept();
    }
}

void StartupDialog::onProjectSelected(int index) {
    selectedProjectPath = projectWidgets[index]->findChild<QLabel*>()->toolTip();
    accept();
}

void StartupDialog::loadRecentProjects() {
    // Очищаем текущий список
    for (auto* widget : projectWidgets) {
        projectsLayout->removeWidget(widget);
        delete widget;
    }
    projectWidgets.clear();

    QStringList projects = settings->value("recent_projects").toStringList();
    if (projects.isEmpty()) {
        projectsWidget->hide(); // Скрываем виджет, если проектов нет
        return;
    }

    projectsWidget->show(); // Показываем виджет, если проекты есть

    for (int i = 0; i < projects.size(); ++i) {
        QStringList parts = projects[i].split("|");
        if (parts.size() != 2) continue;

        QString name = parts[0];
        QString path = parts[1];

        // Создаём кастомный виджет для проекта
        ProjectWidget* projectWidget = new ProjectWidget(name, path, i, this);
        projectsLayout->addWidget(projectWidget);
        projectWidgets.append(projectWidget);

        // Подключаем сигнал клика
        connect(projectWidget, &ProjectWidget::clicked, this, &StartupDialog::onProjectSelected);
    }
}

void StartupDialog::saveRecentProjects() {
    QStringList projects;
    for (const auto* widget : projectWidgets) {
        QLabel* label = widget->findChild<QLabel*>();
        QString name = label->text().split("\n")[0];
        QString path = label->toolTip();
        projects << name + "|" + path;
    }
    settings->setValue("recent_projects", projects);
}