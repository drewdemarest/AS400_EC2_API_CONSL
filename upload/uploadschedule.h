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

private slots:
    void runAS400InvoiceUpload();
    void runAS400CustomerChains();
    void runAS400OpenOrderHeaderUpload();
    void runAS400OpenOrderDetailUpload();
    void beginDailyInvoiceUpload();
    void beginDailyCustomerChainUpload();
    void beginDailyOpenOrderHeaderUpload();
    void beginDailyOpenOrderDetailUpload();

private:
    void setupDailySchedule(QTimer *uploadTimer, void (UploadSchedule::*queryFunc)());

    JsonSettings settings_;
    QString invoiceQueryDBPath_ = qApp->applicationDirPath() + "/invoiceQuery.db";
    QJsonObject invoiceQuerySettings_ =     {{"millisecondInterval",    QJsonValue()},
                                            {"dailyUploadTime",         QJsonValue()},
                                            {"invoiceDaysPrior",        QJsonValue()},
                                            {"chunkSize",               QJsonValue()},
                                            {"usingUploadInterval",     QJsonValue()},
                                            {"usingUploadDaily",        QJsonValue()}};

    QString custChainQueryDBPath_ = qApp->applicationDirPath() + "/custChainQuery.db";
    QJsonObject custChainQuerySettings_ =   {{"millisecondInterval", QJsonValue()},
                                            {"dailyUploadTime",     QJsonValue()},
                                            {"chunkSize",           QJsonValue()},
                                            {"usingUploadInterval", QJsonValue()},
                                            {"usingUploadDaily",    QJsonValue()}};

    QString openOrderHeaderQueryDBPath_ = qApp->applicationDirPath() + "/openOrderHeaderQuery.db";
    QJsonObject openOrderHeaderSettings_ =      {{"millisecondInterval", QJsonValue()},
                                                {"dailyUploadTime",      QJsonValue()},
                                                {"chunkSize",            QJsonValue()},
                                                {"usingUploadInterval",  QJsonValue()},
                                                {"usingUploadDaily",     QJsonValue()}};

    QString openOrderDetailQueryDBPath_ = qApp->applicationDirPath() + "/openOrderDetailQuery.db";
    QJsonObject openOrderDetailSettings_ =      {{"millisecondInterval", QJsonValue()},
                                                {"dailyUploadTime",      QJsonValue()},
                                                {"chunkSize",            QJsonValue()},
                                                {"usingUploadInterval",  QJsonValue()},
                                                {"usingUploadDaily",     QJsonValue()}};

    bool isNegative(const long long number);
    bool isZero(const long long number);
    long long jsonValueToLongLong(const QJsonValue &value);

    QTimer *invoiceUploadTimer_             = new QTimer(this);
    QTimer *invoiceUploadDailyTimer_        = new QTimer(this);

    QTimer *customerChainUploadTimer_       = new QTimer(this);
    QTimer *customerChainUploadDailyTimer_  = new QTimer(this);

    QTimer *openOrderHeaderUploadTimer_       = new QTimer(this);
    QTimer *openOrderHeaderUploadDailyTimer_  = new QTimer(this);

    QTimer *openOrderDetailUploadTimer_       = new QTimer(this);
    QTimer *openOrderDetailUploadDailyTimer_  = new QTimer(this);

    void configUpload(QJsonObject querySettings,
                      QTimer *uploadTimer,
                      QTimer *uploadDailyTimer,
                      void (UploadSchedule::*queryFunc)(),
                      void (UploadSchedule::*dailyUploadFunc)());

    void configInvoiceUpload();
    void configCustomerChainUpload();
    void configOpenOrderHeaderUpload();
    void configOpenOrderDetailUpload();

    LogWriter   logWriter_;
    AS400       as400_;
    MySQL       mysql_;
};

#endif // UPLOADSCHEDULE_H
