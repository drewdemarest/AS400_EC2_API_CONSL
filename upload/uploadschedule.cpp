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

//-----------------------------------------------------------------------------
// CREATE A SINGLE FUNCTION AND CALL IN A LOOP!
// Everything below this here ain't sustainable.
//-----------------------------------------------------------------------------

void UploadSchedule::beginDailyInvoiceUpload()
{
    connect(invoiceUploadTimer_, SIGNAL(timeout()), this, SLOT(runAS400InvoiceUpload()));
    invoiceUploadTimer_->setSingleShot(86400000);
    invoiceUploadTimer_->start(86400000);
    runAS400InvoiceUpload();
}

void UploadSchedule::beginDailyCustomerChainUpload()
{
    connect(customerChainUploadTimer_, SIGNAL(timeout()), this, SLOT(runAS400CustomerChains()));
    customerChainUploadTimer_->setSingleShot(false);
    customerChainUploadTimer_->start(86400000);
    runAS400CustomerChains();
}

void UploadSchedule::beginDailyOpenOrderHeaderUpload()
{
    connect(openOrderHeaderUploadTimer_, SIGNAL(timeout()), this, SLOT(runAS400OpenOrderHeaderUpload()));
    openOrderHeaderUploadTimer_->setSingleShot(false);
    openOrderHeaderUploadTimer_->start(86400000);
    runAS400OpenOrderHeaderUpload();
}

void UploadSchedule::beginDailyOpenOrderDetailUpload()
{
    connect(openOrderDetailUploadTimer_, SIGNAL(timeout()), this, SLOT(runAS400OpenOrderDetailUpload()));
    openOrderDetailUploadTimer_->setSingleShot(false);
    openOrderDetailUploadTimer_->start(86400000);
    runAS400OpenOrderDetailUpload();
}

void UploadSchedule::configInvoiceUpload()
{
    if(invoiceQuerySettings_["usingUploadInterval"].toString().toInt())
    {
        connect(invoiceUploadTimer_, SIGNAL(timeout()), this, SLOT(runAS400InvoiceUpload()));
        invoiceUploadTimer_->start(jsonValueToLongLong(invoiceQuerySettings_["invoiceInterval"]));
        runAS400InvoiceUpload();
    }

    if(invoiceQuerySettings_["usingUploadDaily"].toString().toInt())
    {
        qint64 msUntilExec =
                QTime::currentTime().msecsTo
                (QTime::fromString(invoiceQuerySettings_["dailyUploadTime"].toString(), Qt::ISODate));

        if(msUntilExec <= 0)
            msUntilExec = 86400000 + msUntilExec;

        connect(invoiceUploadDailyTimer_, SIGNAL(timeout()), this, SLOT(beginDailyInvoiceUpload()));
        invoiceUploadDailyTimer_->setSingleShot(true);
        invoiceUploadDailyTimer_->start(msUntilExec);
    }
}

void UploadSchedule::configOpenOrderHeaderUpload()
{
    if(openOrderHeaderSettings_["usingUploadInterval"].toString().toInt())
    {
        connect(openOrderHeaderUploadTimer_, SIGNAL(timeout()), this, SLOT(runAS400OpenOrderHeaderUpload()));
        openOrderHeaderUploadTimer_->start(jsonValueToLongLong(openOrderHeaderSettings_["invoiceInterval"]));
        runAS400OpenOrderHeaderUpload();
    }

    if(openOrderHeaderSettings_["usingUploadDaily"].toString().toInt())
    {
        qint64 msUntilExec =
                QTime::currentTime().msecsTo
                (QTime::fromString(openOrderHeaderSettings_["dailyUploadTime"].toString(), Qt::ISODate));

        if(msUntilExec <= 0)
            msUntilExec = 86400000 + msUntilExec;

        connect(openOrderHeaderUploadDailyTimer_, SIGNAL(timeout()), this, SLOT(beginDailyOpenOrderHeaderUpload()));
        openOrderHeaderUploadDailyTimer_->setSingleShot(true);
        openOrderHeaderUploadDailyTimer_->start(msUntilExec);
    }
}

void UploadSchedule::configOpenOrderDetailUpload()
{
    if(openOrderDetailSettings_["usingUploadInterval"].toString().toInt())
    {
        connect(openOrderDetailUploadTimer_, SIGNAL(timeout()), this, SLOT(runAS400OpenOrderDetailUpload()));
        openOrderDetailUploadTimer_->start(jsonValueToLongLong(openOrderDetailSettings_["invoiceInterval"]));
        runAS400OpenOrderDetailUpload();
    }

    if(openOrderDetailSettings_["usingUploadDaily"].toString().toInt())
    {
        qint64 msUntilExec =
                QTime::currentTime().msecsTo
                (QTime::fromString(openOrderDetailSettings_["dailyUploadTime"].toString(), Qt::ISODate));

        if(msUntilExec <= 0)
            msUntilExec = 86400000 + msUntilExec;

        connect(openOrderDetailUploadDailyTimer_, SIGNAL(timeout()), this, SLOT(beginDailyOpenOrderHeaderUpload()));
        openOrderDetailUploadDailyTimer_->setSingleShot(true);
        openOrderDetailUploadDailyTimer_->start(msUntilExec);
    }
}

void UploadSchedule::configCustomerChainUpload()
{
    if(custChainQuerySettings_["usingUploadInterval"].toString().toInt())
    {
        connect(customerChainUploadTimer_, SIGNAL(timeout()), this, SLOT(runAS400CustomerChains()));
        customerChainUploadTimer_->start(jsonValueToLongLong(custChainQuerySettings_["invoiceInterval"]));
        runAS400InvoiceUpload();
    }

    if(custChainQuerySettings_["usingUploadDaily"].toString().toInt())
    {
        qint64 msUntilExec =
                QTime::currentTime().msecsTo
                (QTime::fromString(custChainQuerySettings_["dailyUploadTime"].toString(), Qt::ISODate));

        if(msUntilExec <= 0)
            msUntilExec = 86400000 + msUntilExec;

        connect(customerChainUploadDailyTimer_, SIGNAL(timeout()), this, SLOT(beginDailyCustomerChainUpload()));
        customerChainUploadDailyTimer_->setSingleShot(true);
        customerChainUploadDailyTimer_->start(msUntilExec);
    }
}

long long UploadSchedule::jsonValueToLongLong(const QJsonValue &value)
{
    return QString(value.toString()).toLongLong();
}
