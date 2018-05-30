#ifndef MYSQL_H
#define MYSQL_H

//Qt
#include <QObject>
#include <QtSql>
#include <QtCore>
//std
#include <iostream>
//Custom
#include "json_settings/jsonsettings.h"
#include "log_writer/logwriter.h"
#include "mysql_oot_dialogs/inputsettingsthread.hpp"

class MySQL : public QObject
{
    Q_OBJECT
public:
    explicit MySQL(QObject *parent = nullptr);
    void init();

signals:
    void debugMessage(QString dbg);

public slots:
    bool exportInvoiceResults(QMap<QString, QVariantList> invoiceResults);
    bool exportCustomerChainResults(QMap<QString, QVariantList> invoiceResults);
    void handleSettingsDialog(bool inputNewSettings);

private:
    bool exportResults(const QString &tableName, QMap<QString, QVariantList> invoiceResults);
    void inputMySQLSettings();
    JsonSettings settings_;
    QJsonObject mySQLSettings_ =    {{"caStr",           QJsonValue()},
                                     {"clientKeyStr",    QJsonValue()},
                                     {"clientCertStr",   QJsonValue()},
                                     {"hostName",        QJsonValue()},
                                     {"databaseName",    QJsonValue()},
                                     {"userName",        QJsonValue()},
                                     {"password",        QJsonValue()}};



    QStringList generateValueTuples(QMap<QString, QVariantList> invoiceResults);
    bool executeQueryAsBatch(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> invoiceResults);
    bool executeQueryAsString(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> invoiceResults);
    void thingToForward(QString dbg);
};

#endif // MYSQL_H
