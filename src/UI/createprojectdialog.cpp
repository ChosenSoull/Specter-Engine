// createprojectdialog.cpp
#include "createprojectdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QPixmap>
#include <QScrollArea>
#include <QLabel>
#include <QFontMetrics>

// Реализация LibraryItemWidget
LibraryItemWidget::LibraryItemWidget(const QString &name, const QString &description, const QString &avatarPath, QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5,5,5,5);

    selectBox = new QCheckBox(this);
    mainLayout->addWidget(selectBox);

    avatarLabel = new QLabel(this);
    avatarLabel->setFixedSize(50,50);
    QPixmap avatarPix(avatarPath);
    if (!avatarPix.isNull()) {
        avatarLabel->setPixmap(avatarPix.scaled(50,50, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        avatarLabel->setText("No Img");
        avatarLabel->setAlignment(Qt::AlignCenter);
        avatarLabel->setStyleSheet("border: 1px solid gray;");
    }
    mainLayout->addWidget(avatarLabel);

    QVBoxLayout* textLayout = new QVBoxLayout();
    nameLabel = new QLabel(name, this);
    nameLabel->setStyleSheet("font-weight: bold;");
    textLayout->addWidget(nameLabel);

    descLabel = new QLabel(this);
    descLabel->setWordWrap(true);
    QFontMetrics fm(descLabel->font());
    QString elidedDesc = fm.elidedText(description, Qt::ElideRight, 200);
    descLabel->setText(elidedDesc);
    textLayout->addWidget(descLabel);

    mainLayout->addLayout(textLayout);
}

bool LibraryItemWidget::isSelected() const {
    return selectBox->isChecked();
}

void LibraryItemWidget::setSelected(bool selected) {
    selectBox->setChecked(selected);
}

QString LibraryItemWidget::getLibraryName() const {
    return nameLabel->text();
}

// Реализация CreateProjectDialog
CreateProjectDialog::CreateProjectDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Create New Project");
    setFixedSize(800, 400);
    setupUI();
    loadLibraries();
}

CreateProjectDialog::~CreateProjectDialog() {
}

void CreateProjectDialog::setupUI() {
    // Основной горизонтальный layout: слева – настройки проекта, справа – выбор библиотек
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // Левая панель
    QWidget* leftWidget = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);

    // Верхняя часть: логотип и поля для названия/директории проекта
    QHBoxLayout* topLayout = new QHBoxLayout();
    logoPreview = new ClickableLabel(this);
    logoPreview->setFixedSize(100, 100);
    logoPreview->setStyleSheet("border: 1px solid gray;");
    logoPreview->setAlignment(Qt::AlignCenter);
    logoPreview->setText("Click to select image");
    connect(logoPreview, &ClickableLabel::clicked, this, &CreateProjectDialog::onLogoSelected);
    topLayout->addWidget(logoPreview);

    QVBoxLayout* rightTopLayout = new QVBoxLayout();
    QLabel* nameLabel = new QLabel("Project Name:", this);
    projectNameEdit = new QLineEdit(this);
    rightTopLayout->addWidget(nameLabel);
    rightTopLayout->addWidget(projectNameEdit);

    QLabel* dirLabel = new QLabel("Project Directory:", this);
    directoryLineEdit = new ClickableLineEdit(this);
    directoryLineEdit->setPlaceholderText("Click to select project directory");
    connect(directoryLineEdit, &ClickableLineEdit::clicked, this, &CreateProjectDialog::onSelectProjectDirectory);
    rightTopLayout->addWidget(dirLabel);
    rightTopLayout->addWidget(directoryLineEdit);

    topLayout->addLayout(rightTopLayout);
    leftLayout->addLayout(topLayout);

    // Описание проекта
    QLabel* descLabel = new QLabel("Description:", this);
    descriptionEdit = new QTextEdit(this);
    descriptionEdit->setFixedHeight(80);
    leftLayout->addWidget(descLabel);
    leftLayout->addWidget(descriptionEdit);

    // Выбор 2D/3D
    renderMode3DCheck = new QCheckBox("Enable 3D (unchecked for 2D)", this);
    leftLayout->addWidget(renderMode3DCheck);

    // Группа для выбора рендеринга с динамической нумерацией
    QGroupBox* renderGroupBox = new QGroupBox("Render Options (Click to select order, double-click to remove)", this);
    QHBoxLayout* renderLayout = new QHBoxLayout(renderGroupBox);
    openglOption = new RenderOptionWidget("OpenGL", this);
    vulkanOption = new RenderOptionWidget("Vulkan", this);
    directxOption = new RenderOptionWidget("DirectX", this);
#ifdef Q_OS_LINUX
    directxOption->setEnabled(false);
    directxOption->setStyleSheet("color: gray; border: 1px solid gray;");
#endif

    renderLayout->addWidget(openglOption);
    renderLayout->addWidget(vulkanOption);
    renderLayout->addWidget(directxOption);

    // Подключение сигналов для обработки кликов и двойных кликов
    connect(openglOption, &RenderOptionWidget::clicked, this, &CreateProjectDialog::onRenderOptionClicked);
    connect(vulkanOption, &RenderOptionWidget::clicked, this, &CreateProjectDialog::onRenderOptionClicked);
    connect(directxOption, &RenderOptionWidget::clicked, this, &CreateProjectDialog::onRenderOptionClicked);
    connect(openglOption, &RenderOptionWidget::removed, this, &CreateProjectDialog::onRenderOptionRemoved);
    connect(vulkanOption, &RenderOptionWidget::removed, this, &CreateProjectDialog::onRenderOptionRemoved);
    connect(directxOption, &RenderOptionWidget::removed, this, &CreateProjectDialog::onRenderOptionRemoved);

    leftLayout->addWidget(renderGroupBox);

    // Кнопка создания проекта
    createButton = new QPushButton("Create Project", this);
    connect(createButton, &QPushButton::clicked, this, &CreateProjectDialog::onCreateProject);
    leftLayout->addWidget(createButton);

    leftLayout->addStretch();
    mainLayout->addWidget(leftWidget, 2); // левая панель – 2/3 ширины

    // Правая панель – выбор библиотек
    QWidget* rightWidget = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    QLabel* libsLabel = new QLabel("Select Libraries:", this);
    rightLayout->addWidget(libsLabel);
    libsScrollArea = new QScrollArea(this);
    libsScrollArea->setWidgetResizable(true);
    libsContainer = new QWidget();
    libsLayout = new QVBoxLayout(libsContainer);
    libsContainer->setLayout(libsLayout);
    libsScrollArea->setWidget(libsContainer);
    rightLayout->addWidget(libsScrollArea);
    rightLayout->addStretch();
    mainLayout->addWidget(rightWidget, 3); // правая панель – 1/3 ширины
}

void CreateProjectDialog::loadLibraries() {
    // Каталоги для поиска библиотек
    QStringList libDirs = {"libs/standart", "libs/custom"};
    for (const QString &libDirPath : libDirs) {
        QDir libDir(libDirPath);
        if (!libDir.exists())
            continue;
        // Ищем файлы .cfg
        QStringList cfgFiles = libDir.entryList(QStringList() << "*.cfg", QDir::Files);
        for (const QString &cfgFile : cfgFiles) {
            QString cfgFilePath = libDir.absoluteFilePath(cfgFile);
            QSettings libSettings(cfgFilePath, QSettings::IniFormat);
            libSettings.beginGroup("Library");
            QString libName = libSettings.value("Name").toString();
            QString libDesc = libSettings.value("Description").toString();
            QString avatarRelPath = libSettings.value("Avatar").toString();
            libSettings.endGroup();
            // Формируем полный путь к аватарке
            QString avatarPath = libDir.absoluteFilePath(avatarRelPath);
            // Создаём виджет библиотеки и добавляем в layout
            LibraryItemWidget* item = new LibraryItemWidget(libName, libDesc, avatarPath, this);
            libsLayout->addWidget(item);
            libraryItems.append(item);
        }
    }
    libsLayout->addStretch();
}

void CreateProjectDialog::onLogoSelected() {
    logoPath = QFileDialog::getOpenFileName(this, "Select Logo", "", "Images (*.png *.jpg *.bmp)");
    if (!logoPath.isEmpty()) {
        QPixmap pixmap(logoPath);
        if (!pixmap.isNull()) {
            logoPreview->setPixmap(pixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            logoPreview->setText("Invalid Image");
        }
    }
}

void CreateProjectDialog::onSelectProjectDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Project Directory");
    if (!dir.isEmpty()) {
        directoryLineEdit->setText(dir);
    }
}

bool CreateProjectDialog::is3DEnabled() const {
    return renderMode3DCheck->isChecked();
}

// Возвращает выбранные API в порядке выбора (например, {"OpenGL", "Vulkan"})
QList<QString> CreateProjectDialog::getRenderAPIs() const {
    QList<QString> apis;
    for (RenderOptionWidget* option : selectedRenderOptions) {
        apis.append(option->getApiName());
    }
    return apis;
}

QList<LibraryItemWidget*> CreateProjectDialog::getSelectedLibraries() const {
    QList<LibraryItemWidget*> selected;
    for (LibraryItemWidget* item : libraryItems) {
        if (item->isSelected()) {
            selected.append(item);
        }
    }
    return selected;
}

void CreateProjectDialog::onRenderOptionClicked(RenderOptionWidget* option) {

    if (option->isSelected()) {
        // Если опция уже выбрана, то сбрасываем её
        option->setSelected(false);
        option->setOrder(0);
        selectedRenderOptions.removeAll(option);

        // После удаления опции перенумеровываем оставшиеся элементы
        for (int i = 0; i < selectedRenderOptions.size(); ++i) {
            selectedRenderOptions[i]->setOrder(i + 1);
        }
    } else {
        // Если количество выбранных опций достигло максимума, удаляем первую
        if (selectedRenderOptions.size() >= 2) {
            RenderOptionWidget* removedOption = selectedRenderOptions.takeFirst();
            removedOption->setSelected(false);
            removedOption->setOrder(0);
        }

        // Добавляем новую опцию
        option->setSelected(true);
        selectedRenderOptions.append(option);

        // Перенумеровываем все опции, чтобы они шли по порядку
        for (int i = 0; i < selectedRenderOptions.size(); ++i) {
            selectedRenderOptions[i]->setOrder(i + 1);
        }
    }
}

void CreateProjectDialog::onRenderOptionRemoved(RenderOptionWidget* option) {
    // Убираем опцию из списка выбранных
    selectedRenderOptions.removeAll(option);

    // Сбрасываем её состояние
    option->setSelected(false);
    option->setOrder(0); // Сбрасываем номер на 0

    // Перенумеровываем оставшиеся опции
    for (int i = 0; i < selectedRenderOptions.size(); ++i) {
        selectedRenderOptions[i]->setOrder(i + 1);
    }
}

void CreateProjectDialog::onCreateProject() {
    if (projectNameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Project name cannot be empty!");
        return;
    }
    if (directoryLineEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Project directory must be selected!");
        return;
    }
    QDir dir(directoryLineEdit->text());
    if (!dir.exists()) {
        if (!QDir().mkpath(directoryLineEdit->text())) {
            QMessageBox::warning(this, "Error", "Failed to create project directory!");
            return;
        }
    }
    // Перемещаем логотип в директорию проекта
    moveLogoToProject();
    // Сохраняем конфигурационный файл (.cfg) в выбранной директории
    saveConfigFile();
    accept();
}

void CreateProjectDialog::moveLogoToProject() {
    if (!logoPath.isEmpty()) {
        QFileInfo info(logoPath);
        QString extension = info.suffix();
        QString newLogoPath = directoryLineEdit->text() + "/files/logo." + extension;
        QDir dir(directoryLineEdit->text());
        if (!dir.exists("files")) {
            dir.mkdir("files");
        }
        QFile::rename(logoPath, newLogoPath);
    }
}

void CreateProjectDialog::saveConfigFile() {
    QString configFilePath = directoryLineEdit->text() + "/config.cfg";
    QSettings config(configFilePath, QSettings::IniFormat);
    config.beginGroup("Project");
    config.setValue("Name", projectNameEdit->text());
    config.setValue("Description", descriptionEdit->toPlainText());
    config.setValue("RenderMode", is3DEnabled() ? "3D" : "2D");
    config.endGroup();

    // Сохраняем выбранные API согласно порядку (Render1, Render2, Render3)
    QList<QString> apis = getRenderAPIs();
    for (int i = 0; i < apis.size(); ++i) {
         config.beginGroup(QString("Render%1").arg(i+1));
         config.setValue("API", apis[i]);
         config.endGroup();
    }

    // Сохраняем выбранные библиотеки (их названия)
    QStringList selectedLibs;
    for (LibraryItemWidget* item : libraryItems) {
        if (item->isSelected()) {
            selectedLibs.append(item->getLibraryName());
        }
    }
    config.beginGroup("Libraries");
    config.setValue("Selected", selectedLibs);
    config.endGroup();
}
