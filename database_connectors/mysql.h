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
#include "oot_dialogs/inputsettingsthread.hpp"

class MySQL : public QObject
{
    Q_OBJECT
public:
    explicit MySQL(QObject *parent = nullptr);
    void init();

signals:
    void debugMessage(QString dbg);

public slots:
    bool exportInvoiceResults(QMap<QString, QVariantList> sqlResutls);
    bool exportCustomerChainResults(QMap<QString, QVariantList> sqlResutls);
    bool exportOpenOrderHeaderResult(bool needToTruncate, QMap<QString, QVariantList> sqlResutls);
    bool exportOpenOrderDetailResult(bool needToTruncate, QMap<QString, QVariantList> sqlResutls);

private:
    bool truncateATable(const QString &tableName);
    bool exportResults(const QString &tableName, QMap<QString, QVariantList> invoiceResults);
    void inputMySQLSettings();
    QString dbPath_ = qApp->applicationDirPath() + "/mysqlsettings.db";
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
