#include "as400.h"

AS400::AS400(const QString &systemIP, const QString &username, const QString &password, QObject *parent) : QObject(parent)
{
    connectString_ = "DRIVER={iSeries Access ODBC Driver};SYSTEM=" + systemIP +";";
    username_ = username;
    password_ = password;
}

bool AS400::getInvoiceData(const QDate &minDate, const QDate &maxDate, const int chunkSize)
{
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
                        "BETWEEN \'"
                        +minDate.toString("yyyy-MM-dd")
                        +"\' AND \'"
                        +maxDate.toString("yyyy-MM-dd")+"\'");

    return queryAS400(AS400QueryType::Invoice, queryString, chunkSize);
}

bool AS400::getCustomerChains(const int chunkSize)
{
    QString queryString = "SELECT CAST(BBSCMPN AS INT) AS \"companyNumber\", "
                          "CAST(BBSDIVN AS INT) AS \"divisionNumber\", "
                          "CAST(BBSDPTN AS INT) AS \"departmentNumber\", "
                          "REPLACE(BBSCSCD,' ','') AS \"chainStoreCode\", "
                          "BBSCTDC AS \"chainDescription\" "
                          "FROM PWRDTA.BBSCHNHP";

    return queryAS400(AS400QueryType::CustomerChain, queryString,chunkSize);
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

bool AS400::queryAS400(const AS400QueryType queryType, const QString &queryString, const int chunkSize)
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

            emit debugMessage(QString("Running against AS400 with a chunk size of " + QString::number(chunkSize)));
            emit debugMessage(QString("If you have an overflow, reduce chunk size."));
            emit debugMessage(queryString);

            if(query.exec(queryString))
            {
                success = true;
                processQuery(queryType, query, chunkSize);
            }
            else
            {
                success = false;
                emit debugMessage("AS400 Query Error: " + query.lastError().text());
            }

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

void AS400::processQuery(const AS400QueryType queryType, QSqlQuery &query,const int chunkSize)
{
    int recordCounter = 0;
    QMap<QString,QVariantList> sqlData;

    while(query.next())
    {
        if(recordCounter == chunkSize)
        {
            //Count the amt of records.
            emit debugMessage(QString("Retrieved " +  QString::number(sqlData.first().size()) + " records from AS400."));

            switch (queryType)
            {
            case AS400QueryType::Invoice :
                emit invoiceDataResults(sqlData);
                break;

            case AS400QueryType::CustomerChain :
                emit customerChainResults(sqlData);
                break;
            }

            for(auto key: sqlData.keys())
            {
                sqlData[key].clear();
            }
            sqlData.clear();
            recordCounter = 0;
        }

        for(int j = 0; j < query.record().count(); ++j)
        {
            sqlData[query.record().fieldName(j)].append(query.value(j));
        }
        ++recordCounter;
    }

    //Count the amt of records.
    emit debugMessage(QString("Retrieved " +  QString::number(sqlData.first().size()) + " records from AS400."));

    switch (queryType)
    {
    case AS400QueryType::Invoice :
        emit invoiceDataResults(sqlData);
        break;

    case AS400QueryType::CustomerChain :
        emit customerChainResults(sqlData);
        break;
    }

    for(auto key: sqlData.keys())
        sqlData[key].clear();

    sqlData.clear();
}

