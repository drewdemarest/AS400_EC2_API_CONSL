#ifndef INPUTSETTINGSTHREAD_H
#define INPUTSETTINGSTHREAD_H

#include <QObject>
#include <QtCore>
#include <QThread>
#include <iostream>

class InputSettingsThread : public QThread
{
    Q_OBJECT
    void run()
    {
        QString inputNewSettingsStr;
        QTextStream s(stdin);
        inputNewSettingsStr = s.readLine();

        if(inputNewSettingsStr == "y")
            emit result(true);

        if(inputNewSettingsStr == "n")
            emit result(false);

        if(!(inputNewSettingsStr == "y" || inputNewSettingsStr == "n"))
        {
            emit result(false);
            emit debugMessage("MySQL user input error: incorrect user input for using existing settings."
                              " Input not y or n. "
                              "Using existing settings.");
        }
        QTimer waitASec;
        waitASec.setSingleShot(true);
        waitASec.start(1000);
        while(waitASec.isActive())
            qApp->processEvents();
    }
signals:
    void result(bool inputNewSettings);
    void debugMessage(QString debug);
};

#endif // INPUTSETTINGSTHREAD_H
