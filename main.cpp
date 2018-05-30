#include <QCoreApplication>
#include <QtCore>
#include "database_connectors/as400.h"
#include "database_connectors/mysql.h"
#include "json_settings/jsonsettings.h"
#include "log_writer/logwriter.h"
#include "unistd.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    LogWriter logWriter;
    AS400 as400();
    MySQL mysql;
    QObject::connect(&as400, &AS400::debugMessage, &logWriter, &LogWriter::writeLogEntry);
    QObject::connect(&mysql, &MySQL::debugMessage, &logWriter, &LogWriter::writeLogEntry);
    QObject::connect(&as400, &AS400::invoiceDataResults, &mysql, &MySQL::exportInvoiceResults);

    mysql.init();

    while(true)
    {
        as400.getInvoiceData(QDate::currentDate().addDays(-2), QDate::currentDate(), 10000);
        qDebug() << "Waiting 43200 seconds until next upload.";
        sleep(43200);
    }

    return a.exec();
}
