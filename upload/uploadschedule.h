#ifndef UPLOADSCHEDULE_H
#define UPLOADSCHEDULE_H

#include <QObject>
#include <QtCore>
#include "database_connectors/as400.h"
#include "database_connectors/mysql.h"
#include "json_settings/jsonsettings.h"
#include "log_writer/logwriter.h"
#include <iostream>

class UploadSchedule : public QObject
{
    Q_OBJECT
public:
    explicit UploadSchedule(QObject *parent = nullptr);
    void init();
    void run();
signals:
    void debugMessage(QString dbg);

public slots:
    void handleSettingsDialog(bool inputNewSettings);

private slots:
    void runAS400InvoiceUpload();
    void runAS400CustomerChains();

private:
    JsonSettings settings_;
    QJsonObject scheduleSettings_ =    {{"invoiceInterval",         QJsonValue()},
                                        {"invoiceDaysPrior",        QJsonValue()},
                                        {"invoiceChunkSize",        QJsonValue()},
                                        {"customerChainInterval",   QJsonValue()},
                                        {"customerChainChunkSize",  QJsonValue()}};

    void inputScheduleSettings();
    bool isNegative(const long long number);
    bool isZero(const long long number);
    long long jsonValueToLongLong(const QJsonValue &value);

    LogWriter   logWriter_;
    AS400       as400_;
    MySQL       mysql_;
};

#endif // UPLOADSCHEDULE_H
