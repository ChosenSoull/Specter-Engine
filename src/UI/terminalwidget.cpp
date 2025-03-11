#include "terminalwidget.h"
#include <QApplication>
#include <QDesktopServices>
#include <QProcessEnvironment>
#include <QKeySequence>

TerminalWidget::TerminalWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Создаем терминал
    terminal = new QTermWidget(0, this);

    // Настраиваем шрифт терминала
    QFont font = QApplication::font();
    font.setFamily("Monospace");
    font.setPointSize(14);
    terminal->setTerminalFont(font);
    terminal->setBlinkingCursor(true); // Мерцающий курсор
    terminal->setMargin(10); // Внутренние отступы 10 пикселей

    // Задаем цветовую схему
    terminal->setColorScheme("Tango");

    // Добавляем терминал в компоновку
    layout->addWidget(terminal);
    setLayout(layout);

    // Подключаем обработку событий
    connect(terminal, &QTermWidget::termKeyPressed, this, &TerminalWidget::onTermKeyPressed);
    connect(terminal, &QTermWidget::urlActivated, this, &TerminalWidget::onUrlActivated);
    connect(terminal, &QTermWidget::finished, this, &TerminalWidget::onFinished);

    // Определяем текущую оболочку
    detectShell();
}

TerminalWidget::~TerminalWidget() {
    if (terminal) {
        delete terminal;
    }
}

void TerminalWidget::detectShell() {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString shellPath = env.value("SHELL", "/bin/bash");

    if (!shellPath.isEmpty()) {
        startShell(shellPath);
    } else {
        startShell("/bin/bash");
    }
}

void TerminalWidget::startShell(const QString &shellPath) {
    terminal->setShellProgram(shellPath);
    terminal->setEnvironment(QProcess::systemEnvironment());
    terminal->startShellProgram();
}

void TerminalWidget::onTermKeyPressed(const QKeyEvent *key) {
    if (key->matches(QKeySequence::Copy)) {
        terminal->copyClipboard(); // Обработка Ctrl+C
    }
}

void TerminalWidget::onUrlActivated(const QUrl &url, bool fromContextMenu) {
    if (QApplication::keyboardModifiers() & Qt::ControlModifier || fromContextMenu) {
        QDesktopServices::openUrl(url);
    }
}

void TerminalWidget::onFinished() {
    // Можно добавить действия при завершении терминала, например, закрытие виджета
    emit finished(); // Если нужно уведомить родительский объект
}