#include "uploadschedule.h"

UploadSchedule::UploadSchedule(QObject *parent) : QObject(parent)
{
}



void UploadSchedule::init()
{
    QObject::connect(&as400_, &AS400::debugMessage, &logWriter_, &LogWriter::writeLogEntry);
    QObject::connect(&mysql_, &MySQL::debugMessage, &logWriter_, &LogWriter::writeLogEntry);
    QObject::connect(&as400_, &AS400::invoiceDataResults, &mysql_, &MySQL::exportInvoiceResults);
    QObject::connect(&as400_, &AS400::customerChainResults, &mysql_, &MySQL::exportCustomerChainResults);

    std::cout << "Input new upload schedule settings? y/n: ";

    //Total hack to make an input dialog that just expires
    //and sets iteself to a default value.
    InputSettingsThread* settingsThread = new InputSettingsThread();

    connect(settingsThread, &InputSettingsThread::result, this, &UploadSchedule::handleSettingsDialog);
    connect(settingsThread, &InputSettingsThread::debugMessage,  &logWriter_, &LogWriter::writeLogEntry);
    connect(settingsThread, &InputSettingsThread::finished, settingsThread, &QObject::deleteLater);
    settingsThread->start();

    QTimer timer;
    timer.setSingleShot(true);
    timer.start(30000);
    while(timer.isActive() && !settingsThread->isFinished())
    {
        qApp->processEvents();
    }
    if(!settingsThread->isFinished())
    {
        qDebug() << endl;
        emit debugMessage("No response from user after 30sec, using any existing settings");
        settingsThread->quit();
        handleSettingsDialog(false);
        settingsThread->deleteLater();
    }

    as400_.init();
    mysql_.init();
}

void UploadSchedule::handleSettingsDialog(bool inputNewSettings)
{
    if(inputNewSettings)
        inputScheduleSettings();

    if(!inputNewSettings)
        scheduleSettings_ = settings_.loadSettings(QFile(qApp->applicationDirPath() + "/uploadschedulesettings.db"), scheduleSettings_);
}

void UploadSchedule::inputScheduleSettings()
{
    QTextStream s(stdin);
    QMap<QString,QPair<QString,qint64>>intPrompts
       {{"invoiceInterval",         QPair<QString,qint64>("Invoice Update Interval in milliseconds (int): ",         0)},
        {"invoiceDaysPrior",        QPair<QString,qint64>("How many days back to get invoices in days (int): ",      0)},
        {"invoiceChunkSize",        QPair<QString,qint64>("Invoice Chunk Size (int): ",                              0)},
        {"customerChainInterval",   QPair<QString,qint64>("Customer Chain Update Interval in milliseconds (int): ",  0)},
        {"customerChainChunkSize",  QPair<QString,qint64>("Customer Chain Chunk Size (int): ",                       0)}};


    for(auto key:intPrompts.keys())
    {
        while(isNegative(intPrompts[key].second) || isZero(intPrompts[key].second))
        {
            std::cout << intPrompts[key].first.toStdString();
            intPrompts[key].second = s.readLine().toLongLong();

            if(isNegative(intPrompts[key].second))
            {
                qDebug() << intPrompts[key].first << " cannot be negative, try again. Think positive thoughts..." << endl;
            }
            if(isZero(intPrompts[key].second))
            {
                qDebug() << intPrompts[key].first << " cannot be zero." << endl;
            }
        }
    }

    qDebug() << "Please review settings.";
    for(auto key:intPrompts.keys())
    {
        qDebug() << intPrompts[key].first << intPrompts[key].second;
        scheduleSettings_[key] = QString::number(intPrompts[key].second);
    }

    settings_.saveSettings(QFile(qApp->applicationDirPath() + "/uploadschedulesettings.db"), scheduleSettings_);
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
    QTimer *invoiceUploadTimer          = new QTimer(this);
    QTimer *customerChainUploadTimer    = new QTimer(this);

    connect(invoiceUploadTimer, SIGNAL(timeout()), this, SLOT(runAS400InvoiceUpload()));
    connect(customerChainUploadTimer, SIGNAL(timeout()),this,SLOT(runAS400CustomerChains()));

    invoiceUploadTimer->start(jsonValueToLongLong(scheduleSettings_["invoiceInterval"]));
    customerChainUploadTimer->start(jsonValueToLongLong(scheduleSettings_["customerChainInterval"]));

    runAS400InvoiceUpload();
    runAS400CustomerChains();
}

void UploadSchedule::runAS400InvoiceUpload()
{
    as400_.getInvoiceData(QDate::currentDate().addDays(-scheduleSettings_["invoiceDaysPrior"].toInt()), QDate::currentDate(), scheduleSettings_["invoiceChunkSize"].toInt());
}

void UploadSchedule::runAS400CustomerChains()
{
    as400_.getCustomerChains(scheduleSettings_["customerChainChunkSize"].toInt());
}

long long UploadSchedule::jsonValueToLongLong(const QJsonValue &value)
{
    return QString(value.toString()).toLongLong();
}
