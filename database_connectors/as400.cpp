#include "as400.h"

AS400::AS400(const QString &systemIP, const QString &username, const QString &password, QObject *parent) : QObject(parent)
{
    connectString_ = "DRIVER={iSeries Access ODBC Driver};SYSTEM=" + systemIP +";";
    username_ = username;
    password_ = password;
}

bool AS400::getInvoiceData(const QDate &minDate, const QDate &maxDate, const int chunkSize)
{
    bool success = false;
    emit debugMessage(connectString_);
    {
        QSqlDatabase odbc = QSqlDatabase::addDatabase("QODBC", "AS400");
        odbc.setUserName(username_);
        odbc.setPassword(password_);
        odbc.setDatabaseName(connectString_);

        if(odbc.open())
        {
            QSqlQuery query(odbc);
            QString queryString("SELECT invn AS \"invoiceNumber\","
                                "whsn AS \"warehouseNumber\", "
                                "rten AS \"route\", "
                                "stpn AS \"stopNumber\", "
                                "crin AS \"credit\", "
                                "dtei AS \"invoiceDate\", "
                                "dtes AS \"shipDate\", "
                                "dtet AS \"orderDate\", "
                                "timo AS \"orderTime\", "
                                "exsh AS \"weight\", "
                                "excb AS \"caseCube\", "
                                "qysa AS \"casesShipped\", "
                                "qyoa AS \"casesOrdered\", "
                                "exsn AS \"netSales\", "
                                "exac AS \"productCost\", "
                                "exgp AS \"profit\", "
                                "ppft AS \"profitPercent\", "
                                "cusn AS \"customerNumber\", "
                                "slnb AS \"salesRep\", "
                                "demp AS \"driverNumber\", "
                                "tknb AS \"truckNumber\"  "
                                "FROM pwruser.sqlinvhdl0 "
                                "WHERE pday "
                                "BETWEEN \'"+minDate.toString("yyyy-MM-dd")+"\' AND \'"+maxDate.toString("yyyy-MM-dd")+"\'");

            emit debugMessage(QString("Running against AS400 with a chunk size of " + QString::number(chunkSize)));
            emit debugMessage(QString("If you have an overflow, reduce chunk size."));
            emit debugMessage(queryString);

            success = query.exec(queryString);

            processQuery(query, chunkSize);
        }
        else
        {
            emit debugMessage("There was an error with AS400.");
            emit debugMessage(odbc.lastError().text());
            success = false;
        }
    }
    emit debugMessage("Cleaning up AS400 connection.");
    QSqlDatabase::removeDatabase("AS400");
    return success;
}

bool AS400::getCustomerData()
{
    bool success = false;
    return success;
}

bool AS400::getRouteAssignmentData()
{
    bool success = false;
    return success;
}

void AS400::processQuery(QSqlQuery &query,const int chunkSize)
{
    int recordCounter = 0;

    while(query.next())
    {
        if(recordCounter == chunkSize)
        {
            //Count the amt of records.
            //qDebug() << invoiceDataFmt_.first().size();
            emit debugMessage(QString("Retrieved" +  QString::number(invoiceDataFmt_.first().size()) + " records from AS400."));
            emit invoiceDataResults(invoiceDataFmt_);
            for(auto key: invoiceDataFmt_.keys())
            {
                invoiceDataFmt_[key].clear();
            }
            invoiceDataFmt_.clear();
            recordCounter = 0;
        }

        for(int j = 0; j < query.record().count(); ++j)
        {
            invoiceDataFmt_[query.record().fieldName(j)].append(query.value(j));
        }
        ++recordCounter;
    }

    //Count the amt of records.
    emit debugMessage(QString("Retrieved " +  QString::number(invoiceDataFmt_.first().size()) + " records from AS400."));
    emit invoiceDataResults(invoiceDataFmt_);
    for(auto key: invoiceDataFmt_.keys())
    {
        invoiceDataFmt_[key].clear();
    }
    invoiceDataFmt_.clear();
}

