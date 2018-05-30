#ifndef JSONSETTINGS_H
#define JSONSETTINGS_H

#include <QObject>
#include <QtCore>
#include <QtSql>

class JsonSettings : public QObject
{
    Q_OBJECT
public:
    explicit JsonSettings(QObject *parent = nullptr);

    QJsonObject loadSettings(const QFile &dbFile,const QJsonObject &jsonSettings);
    bool saveSettings(const QFile &dbFile, const QJsonObject &jsonSettings);
    bool doesDatabaseExist(const QFile &dbFile);

signals:
    void debugMessage(QString dbg);

public slots:

private:
    bool makeInitalSettingsDatabase(const QFile &dbFile);

    QJsonObject selectSettingsFromDB(QSqlDatabase &db, QJsonObject jsonSettings);

    bool insertSettingsIntoDB(QSqlDatabase &db, const QJsonObject &jsonSettings);
    QString makeInsertString(const QJsonObject &jsonSettings);
};

#endif // JSONSETTINGS_H
