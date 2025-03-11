#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTermWidget>
#include <QKeyEvent>
#include <QUrl>

class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    ~TerminalWidget();

private slots:
    void onTermKeyPressed(const QKeyEvent *key);
    void onUrlActivated(const QUrl &url, bool fromContextMenu);
    void onFinished();

private:
    void detectShell();
    void startShell(const QString &shellPath);

private:
    QTermWidget *terminal;
};

#endif // TERMINALWIDGET_H