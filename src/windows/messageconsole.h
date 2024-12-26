

#ifndef MESSAGECONSOLE_H
#define MESSAGECONSOLE_H

#include <QTextEdit>
#include <QGridLayout>
#include <QDateTime>
#include <QDockWidget>
#include <QWidget>
#include <QMenu>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QSize>

#define MAX_MESSAGES 512

struct MessageConsoleParams {
	bool newestMessageAtBottom;
	int preferredHeight;
};

class MessageConsole : public QWidget
{
	Q_OBJECT
public:
	explicit MessageConsole(QWidget *parent = nullptr);
	~MessageConsole();

	QDockWidget* getDock(){return this->dock;}
	void setParams(MessageConsoleParams params);
	MessageConsoleParams getParams(){return this->params;}
	QSize sizeHint() const override;


private:
	QTextEdit* textEdit;
	QGridLayout* gridLayout;
	QDockWidget* dock;
	QPoint mousePos;
	QVector<QString> messages;
	int messagesIndex;
	MessageConsoleParams params;

	QString addStringToMessageBuffer(QString message);
	void contextMenuEvent(QContextMenuEvent* event) override;
	void refreshMessages();

protected:
	void resizeEvent(QResizeEvent* event) override;

signals:
	void error(QString);
	void info(QString);

public slots:
	void insertNewMessagesAtBottom(bool enable);
	void displayInfo(QString info);
	void displayError(QString error);
};

#endif // MESSAGECONSOLE_H
