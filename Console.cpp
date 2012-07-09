#include "Console.h"
#include <QString>
#include <QDateTime>

/////////////////////////////////////////////////////////////////////////////////////////////////////
Console::Console(QWidget *parent) :
	QTextEdit(parent)
{
	setCurrentCharFormat(QTextCharFormat());
	qRegisterMetaType<Console::InfoLevel>("Console::InfoLevel");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Console::addInfo(QString msg, Console::InfoLevel level)
{
	// html colors: http://www.w3schools.com/html/html_colornames.asp
	const static QString alertHtml = "<font color=\"Red\">";
	const static QString notifyHtml = "<font color=\"BlueViolet\">";
	const static QString infoHtml = "<font color=\"Black\">";
	const static QString endHtml = "</font><br>";

	QString prefix;
	switch(level)
	{
		case Info:
			prefix = infoHtml;
			break;

		case Notify:
			prefix = notifyHtml;
			break;

		case Error:
			prefix = alertHtml;
			break;

		default:
			prefix = infoHtml;
			break;
	}

	QString text = QDateTime::currentDateTime().toString("%1[hh:mm:ss] %2%3").arg(prefix).arg(msg).arg(endHtml);
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::End);
	setTextCursor(cursor);
	insertHtml(text);
	/*
	QScrollBar* scrollBar = _console->verticalScrollBar();
	scrollBar->setValue(scrollBar->maximum());*/
}
