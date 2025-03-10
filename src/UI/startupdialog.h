#ifndef STARTUPDIALOG_H
#define STARTUPDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

class ProjectWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProjectWidget(const QString& name, const QString& path, int index, QWidget* parent = nullptr)
        : QWidget(parent), index_(index) {
        QHBoxLayout* layout = new QHBoxLayout(this);
        QLabel* iconLabel = new QLabel(this);
        QPixmap folderIcon(":/resources/folder_icon.png");
        if (!folderIcon.isNull()) {
            iconLabel->setPixmap(folderIcon.scaled(16, 16, Qt::KeepAspectRatio));
        } else {
            iconLabel->setText("[Icon]");
        }
        layout->addWidget(iconLabel);

        QLabel* textLabel = new QLabel(QString("%1\n%2").arg(name, path), this);
        textLabel->setStyleSheet("color: white;");
        textLabel->setToolTip(path);
        textLabel->setWordWrap(true);
        layout->addWidget(textLabel);
        layout->addStretch();

        setCursor(Qt::PointingHandCursor);
    }

    int index() const { return index_; }

signals:
    void clicked(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override {
        emit clicked(index_);
        QWidget::mousePressEvent(event);
    }

private:
    int index_;
};

class StartupDialog : public QDialog {
    Q_OBJECT

public:
    explicit StartupDialog(QWidget* parent = nullptr);
    ~StartupDialog();

    QString getSelectedProjectPath() const { return selectedProjectPath; }

private slots:
    void onCreateProjectClicked();
    void onOpenProjectClicked();
    void onProjectSelected(int index);

private:
    void loadRecentProjects();
    void saveRecentProjects();

    QPushButton* createButton;
    QPushButton* openButton;
    QString selectedProjectPath;
    QSettings* settings;

    // Для списка проектов
    QVBoxLayout* projectsLayout;
    QWidget* projectsWidget;
    QVector<ProjectWidget*> projectWidgets;
};

#endif