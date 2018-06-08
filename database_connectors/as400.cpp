#include "as400.h"

AS400::AS400(QObject *parent) : QObject(parent)
{
}

AS400::AS400(const QString &systemIP, const QString &username, const QString &password, QObject *parent) : QObject(parent)
{
    AS400Settings_["connectString"] = "DRIVER={iSeries Access ODBC Driver};SYSTEM=" + systemIP +";";
    AS400Settings_["username"] = username;
    AS400Settings_["password"] = password;
}

void AS400::init()
{
    connect(&settings_, &JsonSettings::debugMessage, this, &AS400::debugMessage);

    std::cout << "Input new AS400 settings? y/n: ";

    //Total hack to make an input dialog that just expires
    //and sets iteself to a default value.
    InputSettingsThread* settingsThread = new InputSettingsThread();
    connect(settingsThread, &InputSettingsThread::result, this, &AS400::handleSettingsDialog);
    connect(settingsThread, &InputSettingsThread::debugMessage, this, &AS400::debugMessage);
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
}

void AS400::handleSettingsDialog(bool inputNewSettings)
{
    if(inputNewSettings)
        inputAS400Settings();

    if(!inputNewSettings)
        AS400Settings_ = settings_.loadSettings(QFile(qApp->applicationDirPath() + "/as400settings.db"), AS400Settings_);
}

void AS400::inputAS400Settings()
{
    QTextStream s(stdin);
    QString password;
    QString driver;
    QString username;
    QString hostName;

    std::cout << "Set database hostname or IP: ";
    hostName = s.readLine();

    std::cout << "Set ODBC driver e.g. iSeries Access ODBC Driver: ";
    driver = s.readLine();

    std::cout << "Set username: ";
    username = s.readLine();

    std::cout << "Set password: ";
    password = s.readLine();

    qDebug() << "Please review settings.";
    qDebug() << password;
    qDebug() << username;
    qDebug() << driver;
    qDebug() << hostName;

    AS400Settings_["password"] = password;
    AS400Settings_["username"] = username;
    AS400Settings_["driver"] = "DRIVER={" + driver + "};";
    AS400Settings_["system"] = "SYSTEM=" + hostName + ";";
    AS400Settings_["connectString"] = "DRIVER={" + driver + "};SYSTEM=" + hostName + ";";

    settings_.saveSettings(QFile(qApp->applicationDirPath() + "/as400settings.db"), AS400Settings_);
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
    emit debugMessage(AS400Settings_["connectString"].toString());
    {
        QSqlDatabase odbc = QSqlDatabase::addDatabase("QODBC", "AS400");
        odbc.setUserName(AS400Settings_["username"].toString());
        odbc.setPassword(AS400Settings_["password"].toString());
        odbc.setDatabaseName(AS400Settings_["connectString"].toString());

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

