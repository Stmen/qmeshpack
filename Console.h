#ifndef CONSOLE_H
#define CONSOLE_H

#include <QTextEdit>

class Console : public QTextEdit
{
	Q_OBJECT

public:

	enum InfoLevel
	{
		Info = 0,
		Notify = 1,
		Error = 2
	};

	Q_ENUMS(InfoLevel)

	explicit Console(QWidget *parent = 0);
	inline virtual ~Console() {}
signals:
	
public slots:

	void addInfo(QString msg, Console::InfoLevel level = Info);
};

#endif // CONSOLE_H
