#include "uploadschedule.h"

UploadSchedule::UploadSchedule(QObject *parent) : QObject(parent)
{

}

void UploadSchedule::init()
{
    connect(&as400_, &AS400::debugMessage, &logWriter_, &LogWriter::writeLogEntry);
    connect(&mysql_, &MySQL::debugMessage, &logWriter_, &LogWriter::writeLogEntry);
    connect(&as400_, &AS400::invoiceDataResults, &mysql_, &MySQL::exportInvoiceResults);
    connect(&as400_, &AS400::customerChainResults, &mysql_, &MySQL::exportCustomerChainResults);
    connect(&as400_, &AS400::openOrderHeaderResults, &mysql_, &MySQL::exportOpenOrderHeaderResult);
    connect(&as400_, &AS400::openOrderDetailResults, &mysql_, &MySQL::exportOpenOrderDetailResult);

    custChainQuerySettings_     = settings_.loadSettings(custChainQueryDBPath_,         custChainQuerySettings_);
    invoiceQuerySettings_       = settings_.loadSettings(invoiceQueryDBPath_,           invoiceQuerySettings_);
    openOrderHeaderSettings_    = settings_.loadSettings(openOrderHeaderQueryDBPath_,   openOrderHeaderSettings_);
    openOrderDetailSettings_    = settings_.loadSettings(openOrderDetailQueryDBPath_,   openOrderDetailSettings_);

    as400_.init();
    mysql_.init();
}

bool UploadSchedule::isNegative(const long long number)
{
    if(number < 0)
        return true;
    else
        return false;
}

bool UploadSchedule::isZero(const long long number)
{
   if(number == 0)
       return true;
   else
       return false;
}

void UploadSchedule::run()
{
    configCustomerChainUpload();
    configInvoiceUpload();
    configOpenOrderHeaderUpload();
    configOpenOrderDetailUpload();
}

void UploadSchedule::runAS400InvoiceUpload()
{
    as400_.getInvoiceData(QDate::currentDate().addDays(-invoiceQuerySettings_["invoiceDaysPrior"].toString().toInt()), QDate::currentDate(), invoiceQuerySettings_["chunkSize"].toString().toInt());
}

void UploadSchedule::runAS400CustomerChains()
{
    as400_.getCustomerChains(custChainQuerySettings_["chunkSize"].toString().toInt());
}

void UploadSchedule::runAS400OpenOrderHeaderUpload()
{
    as400_.getOpenOrderHeaders(openOrderHeaderSettings_["chunkSize"].toString().toInt());
}

void UploadSchedule::runAS400OpenOrderDetailUpload()
{
    as400_.getOpenOrderDetails(openOrderDetailSettings_["chunkSize"].toString().toInt());
}

void UploadSchedule::setupDailySchedule(QTimer *uploadTimer, void (UploadSchedule::*queryFunc)())
{
    connect(uploadTimer, &QTimer::timeout, this, queryFunc);
    uploadTimer->setSingleShot(false);
    uploadTimer->start(86400000);
    (this->*queryFunc)();
}

void UploadSchedule::beginDailyInvoiceUpload()
{
    setupDailySchedule(invoiceUploadTimer_, &runAS400InvoiceUpload);
}

void UploadSchedule::beginDailyCustomerChainUpload()
{
    setupDailySchedule(customerChainUploadTimer_, &runAS400CustomerChains);
}

void UploadSchedule::beginDailyOpenOrderHeaderUpload()
{
    setupDailySchedule(openOrderHeaderUploadTimer_, &runAS400OpenOrderHeaderUpload);
}

void UploadSchedule::beginDailyOpenOrderDetailUpload()
{
    setupDailySchedule(openOrderDetailUploadTimer_, &runAS400OpenOrderDetailUpload);
}

void UploadSchedule::configUpload(QJsonObject querySettings,
                                  QTimer *uploadTimer,
                                  QTimer *uploadDailyTimer,
                                  void (UploadSchedule::*queryFunc)(),
                                  void (UploadSchedule::*dailyUploadFunc)())
{
    if(querySettings["usingUploadInterval"].toString().toInt())
    {
        connect(uploadTimer, &QTimer::timeout, this, queryFunc);
        uploadTimer->start(jsonValueToLongLong(querySettings["invoiceInterval"]));
        (this->*queryFunc)();
    }

    if(querySettings["usingUploadDaily"].toString().toInt())
    {
        qint64 msUntilExec =
                QTime::currentTime().msecsTo
                (QTime::fromString(querySettings["dailyUploadTime"].toString(), Qt::ISODate));

        if(msUntilExec <= 0)
            msUntilExec = 86400000 + msUntilExec;

        connect(uploadDailyTimer, &QTimer::timeout, this, dailyUploadFunc);
        uploadDailyTimer->setSingleShot(true);
        invoiceUploadDailyTimer_->start(msUntilExec);
    }
}

void UploadSchedule::configInvoiceUpload()
{
    configUpload(invoiceQuerySettings_,
                 invoiceUploadTimer_,
                 invoiceUploadDailyTimer_,
                 &runAS400InvoiceUpload,
                 &configInvoiceUpload);
}

void UploadSchedule::configOpenOrderHeaderUpload()
{
    configUpload(openOrderHeaderSettings_,
                 openOrderHeaderUploadTimer_,
                 openOrderHeaderUploadDailyTimer_,
                 &runAS400OpenOrderHeaderUpload,
                 &configOpenOrderHeaderUpload);
}

void UploadSchedule::configOpenOrderDetailUpload()
{
    configUpload(openOrderDetailSettings_,
                 openOrderDetailUploadTimer_,
                 openOrderDetailUploadDailyTimer_,
                 &runAS400OpenOrderDetailUpload,
                 &configOpenOrderDetailUpload);
}

void UploadSchedule::configCustomerChainUpload()
{
    configUpload(custChainQuerySettings_,
                 customerChainUploadTimer_,
                 customerChainUploadDailyTimer_,
                 &runAS400CustomerChains,
                 &configCustomerChainUpload);
}

long long UploadSchedule::jsonValueToLongLong(const QJsonValue &value)
{
    return QString(value.toString()).toLongLong();
}
