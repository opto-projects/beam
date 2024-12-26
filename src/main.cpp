#include "app/AppSettings.h"
#include "app/HelpSystem.h"
#include "windows/PlotWindow.h"

#include "helpers/OriTheme.h"
#include "tools/OriDebug.h"
#include "tools/OriHelpWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QTranslator>

#ifndef Q_OS_WIN
#include <iostream>
#endif

void style() {
    qApp->setStyleSheet(
        //reduce separator width
        "QMainWindow::separator {width: 1px} "
        //adjust menu style
        "QMenu::separator {height: 1px; margin-left: 6px; margin-right: 6px; background: rgba(155, 155, 155, 255);}"
    );

    //set style and color palette
    qApp->setStyle("Fusion");
    QPalette dark;

    dark.setColor(QPalette::Text, QColor(255, 255, 255));
    dark.setColor(QPalette::WindowText, QColor(255, 255, 255));
    dark.setColor(QPalette::Window, QColor(50, 50, 50));
    dark.setColor(QPalette::Button, QColor(50, 50, 50));
    dark.setColor(QPalette::Base, QColor(25, 25, 25));
    dark.setColor(QPalette::AlternateBase, QColor(50, 50, 50));
    dark.setColor(QPalette::ToolTipBase, QColor(200, 200, 200));
    dark.setColor(QPalette::ToolTipText, QColor(50, 50, 50));
    dark.setColor(QPalette::ButtonText, QColor(255, 255, 255));
    dark.setColor(QPalette::BrightText, QColor(255, 50, 50));
    dark.setColor(QPalette::Link, QColor(40, 130, 220));
    dark.setColor(QPalette::Disabled, QPalette::Text, QColor(99, 99, 99));
    dark.setColor(QPalette::Disabled, QPalette::WindowText, QColor(99, 99, 99));
    dark.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(99, 99, 99));
    dark.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    dark.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(99, 99, 99));
    qApp->setPalette(dark);

}

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("Laser Beam Inspector ");
    app.setOrganizationName("orion-project.org");
    app.setApplicationVersion(HelpSystem::appVersion());


    QCommandLineParser parser;
    auto optionHelp = parser.addHelpOption();
    auto optionVersion = parser.addVersionOption();
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription("Camera based beam profiler");
    QCommandLineOption optionDevMode("dev"); optionDevMode.setFlags(QCommandLineOption::HiddenFromHelp);
    QCommandLineOption optionConsole("console"); optionConsole.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOptions({optionDevMode, optionConsole});

    if (!parser.parse(QApplication::arguments()))
    {
#ifdef Q_OS_WIN
        QMessageBox::critical(nullptr, app.applicationName(), parser.errorText());
#else
        std::cerr << qPrintable(parser.errorText()) << std::endl;
#endif
        return 1;
    }

    // These will quite the app
    if (parser.isSet(optionHelp))
        parser.showHelp();
    if (parser.isSet(optionVersion))
        parser.showVersion();

    // It's only useful on Windows where there is no
    // direct way to use the console for GUI applications.
    if (parser.isSet(optionConsole) || AppSettings::instance().useConsole)
        Ori::Debug::installMessageHandler();

    // Load application settings before any command start
    AppSettings::instance().isDevMode = 1;// parser.isSet(optionDevMode);
    Ori::HelpWindow::isDevMode = 1;// parser.isSet(optionDevMode);

    QString translationFolder = QCoreApplication::applicationDirPath() + "/translation";
    QTranslator mTranslator;
    mTranslator.load(
        "zh_CN", translationFolder);
    QApplication::instance()->installTranslator(&mTranslator);

    // Call `setStyleSheet` after setting loaded
    // to be able to apply custom colors.
    app.setStyleSheet(Ori::Theme::makeStyleSheet(Ori::Theme::loadRawStyleSheet()));

    style();

    PlotWindow w;
    w.show();

    return app.exec();
}
