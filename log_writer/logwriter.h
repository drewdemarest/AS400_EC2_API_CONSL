#ifndef LOGWRITER_H
#define LOGWRITER_H

#include <QObject>
#include <QtCore>

class LogWriter : public QObject
{
    Q_OBJECT
public:
    explicit LogWriter(QObject *parent = nullptr);
    explicit LogWriter(const QString &logPath, QObject *parent = nullptr);

signals:

public slots:
    bool writeLogEntry(QString entry);

private:
    QString logPath_;
};

#endif // LOGWRITER_H
