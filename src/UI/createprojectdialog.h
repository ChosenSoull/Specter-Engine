// createprojectdialog.h
#ifndef CREATEPROJECTDIALOG_H
#define CREATEPROJECTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QWidget>
#include <QScrollArea>
#include <QList>
#include <QSettings>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMouseEvent>
#include <QHBoxLayout>

// Кликабельная иконка для выбора изображения
class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent = nullptr) : QLabel(parent) {}
signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent* event) override {
        emit clicked();
        QLabel::mousePressEvent(event);
    }
};

// Кликабельное текстовое поле для выбора директории проекта
class ClickableLineEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit ClickableLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) { setReadOnly(true); }
signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent* event) override {
        emit clicked();
        QLineEdit::mousePressEvent(event);
    }
};

// Виджет для отображения информации о библиотеке
class LibraryItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit LibraryItemWidget(const QString &name, const QString &description, const QString &avatarPath, QWidget *parent = nullptr);
    bool isSelected() const;
    void setSelected(bool selected);
    QString getLibraryName() const;
private:
    QCheckBox* selectBox;
    QLabel* avatarLabel;
    QLabel* nameLabel;
    QLabel* descLabel;
};

// Виджет для выбора рендеринга с динамической нумерацией
class RenderOptionWidget : public QWidget {
    Q_OBJECT
public:
    explicit RenderOptionWidget(const QString &apiName, QWidget *parent = nullptr)
        : QWidget(parent), api(apiName), selected(false), order(0)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(5, 5, 5, 5);
        labelOrder = new QLabel("", this);
        labelOrder->setFixedWidth(20);
        labelApi = new QLabel(apiName, this);
        layout->addWidget(labelOrder);
        layout->addWidget(labelApi);
        // Всегда используем стандартную обводку
        setStyleSheet("border: 1px solid gray;");
    }

    int getOrder() const { return order; }
    void setOrder(int ord) {
         order = ord;
         labelOrder->setText(ord > 0 ? QString::number(ord) : "");
    }

    bool isSelected() const { return selected; }
    void setSelected(bool sel) {
         selected = sel;
         setStyleSheet("border: 1px solid gray;");
    }

    QString getApiName() const { return api; }

protected:
    void mousePressEvent(QMouseEvent *event) override {
         if (!selected) {
             emit clicked(this);
         }
         QWidget::mousePressEvent(event);
    }
    void mouseDoubleClickEvent(QMouseEvent *event) override {
         if (selected) {
             emit removed(this);
         }
         QWidget::mouseDoubleClickEvent(event);
    }

signals:
    void clicked(RenderOptionWidget* option);
    void removed(RenderOptionWidget* option);

private:
    QString api;
    bool selected;
    int order;
    QLabel *labelOrder;
    QLabel *labelApi;
};

// Диалог создания проекта
class CreateProjectDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateProjectDialog(QWidget* parent = nullptr);
    ~CreateProjectDialog();

    QString getProjectPath() const { return directoryLineEdit->text(); }
    QString getProjectName() const { return projectNameEdit->text(); }
    QString getDescription() const { return descriptionEdit->toPlainText(); }
    bool is3DEnabled() const;
    // Возвращает список выбранных API в порядке выбора
    QList<QString> getRenderAPIs() const;
    QList<LibraryItemWidget*> getSelectedLibraries() const;

private slots:
    void onLogoSelected();
    void onSelectProjectDirectory();
    void onCreateProject();
    void onRenderOptionClicked(RenderOptionWidget* option);
    void onRenderOptionRemoved(RenderOptionWidget* option);

private:
    // Левые виджеты (настройки проекта)
    ClickableLabel* logoPreview;
    ClickableLineEdit* directoryLineEdit;
    QLineEdit* projectNameEdit;
    QTextEdit* descriptionEdit;
    QCheckBox* renderMode3DCheck;

    // Виджеты выбора рендеринга с динамической нумерацией
    RenderOptionWidget* openglOption;
    RenderOptionWidget* vulkanOption;
    RenderOptionWidget* directxOption;

    QPushButton* createButton;

    // Правая панель – выбор библиотек
    QScrollArea* libsScrollArea;
    QWidget* libsContainer;
    QVBoxLayout* libsLayout;
    QList<LibraryItemWidget*> libraryItems;

    // Список выбранных рендер-виджетов (в порядке выбора)
    QList<RenderOptionWidget*> selectedRenderOptions;

    QString logoPath;

    void setupUI();
    void loadLibraries();
    void moveLogoToProject();
    void saveConfigFile();
};

#endif // CREATEPROJECTDIALOG_H
