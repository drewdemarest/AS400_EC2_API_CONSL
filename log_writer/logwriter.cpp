#include "logwriter.h"

LogWriter::LogWriter(QObject *parent) : QObject(parent)
{
    logPath_ = qApp->applicationDirPath();
}


LogWriter::LogWriter(const QString &logPath, QObject *parent) : QObject(parent)
{
    logPath_ = logPath;
}

bool LogWriter::writeLogEntry(QString entry)
{
    qDebug() << entry << endl;

    QFile logFile(logPath_ + "/" + "logFile-" + QDate::currentDate().toString(Qt::ISODate) + ".txt");
    if(logFile.open(QFile::Append | QIODevice::Text))
    {
        QTextStream out(&logFile);
        out << QString(QDateTime::currentDateTime().toString(Qt::ISODateWithMs) + " -- ") << entry << endl << endl;
    }
    else
    {
        qDebug() << "Could not open log file for writing";
        return false;
    }
    logFile.close();
    return true;

}
