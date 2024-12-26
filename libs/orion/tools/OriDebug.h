#ifndef ORI_DEBUG_H
#define ORI_DEBUG_H

#include <QApplication>
#include <QEvent>
#include <QTextEdit>
#include <QPointer>
#include <QVBoxLayout>

namespace Ori {
namespace Debug {

QString messageType(QtMsgType type)
{
    switch (type)
    {
    case QtDebugMsg: return "DEBUG";
    case QtWarningMsg: return "WARNING";
    case QtCriticalMsg: return "CRITICAL";
    case QtFatalMsg: return "FATAL";
    default: return QString();
    }
}

class LogMsgEvent : public QEvent
{
public:
    LogMsgEvent(const QString& text) : QEvent(QEvent::User), text(text) {}
    QString text;
};

class ConsoleWindow : public QWidget
{
public:
    ConsoleWindow() : QWidget()
    {
        setWindowFlag(Qt::Tool, true);
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowTitle(qApp->applicationName() + " Log");

        _log = new QTextEdit;
        _log->setReadOnly(true);
    #ifdef Q_OS_WIN
        _log->setFont(QFont("Courier", 9));
    #else
        _log->setFont(QFont("Monospace", 10));
    #endif

        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(3, 3, 3, 3);
        layout->addWidget(_log);
    }

    void append(const QString& msg)
    {
        qApp->sendEvent(this, new LogMsgEvent(msg));
    }

protected:
    bool event(QEvent *event) override
    {
        if (auto e = dynamic_cast<LogMsgEvent*>(event); e) {
            _log->append(e->text);
            return true;
        }
        return QWidget::event(event);
    }

private:
    QTextEdit *_log;
};

ConsoleWindow* consoleWindow()
{
    static QPointer<ConsoleWindow> console;
    if (!console)
    {
        console = new ConsoleWindow;
        console->resize(800, 300);
    }
    console->show();
    return console;
}

QString sanitizeHtml(const QString& msg)
{
    return QString(msg).replace("<", "&lt;").replace(">", "&gt;").replace("\n", "<br>");
}

bool mayInoreMessage(const QString& message)
{
    static QStringList ignoredMessages({
        // message sometimes appears when switching between windows on Xfce
        QString::fromLatin1("QXcbWindow: Unhandled client message: \"_GTK_LOAD_ICONTHEMES\""),
    });

    return ignoredMessages.contains(message);
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (mayInoreMessage(message)) return;

    QString msg = QString("<p><b>%1</b>: %2").arg(messageType(type), sanitizeHtml(message));

    // Messages inside of Qt-code has no context filled
    // but function info already built into the message text
    if (context.file)
        msg += QString("<br><font color=gray>(%1:%2, %3</font>")
            .arg(context.file).arg(context.line).arg(context.function);

    consoleWindow()->append(msg);
}
#else
void messageHandler(QtMsgType type, const char* msg)
{
    QString message = QString::fromUtf8(msg);

    if (mayInoreMessage(message)) return;

    consoleWindow()->append(QString("<p><b>%1</b>: %2<br>").arg(messageType(type)).arg(sanitizeHtml(message)));
}
#endif

void installMessageHandler()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    qInstallMessageHandler(messageHandler);
#else
    qInstallMsgHandler(messageHandler);
#endif
}

} // namespace Debug
} // namespace Ori


#endif // ORI_DEBUG_H
