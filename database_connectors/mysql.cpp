#include "mysql.h"

MySQL::MySQL(QObject *parent) : QObject(parent)
{

}

void MySQL::init()
{
    connect(&settings_, &JsonSettings::debugMessage, this, &MySQL::debugMessage);

    std::cout << "Input new MySQL settings? y/n: ";

    //Total hack to make an input dialog that just expires
    //and sets iteself to a default value.
    InputSettingsThread* settingsThread = new InputSettingsThread();
    connect(settingsThread, &InputSettingsThread::result, this, &MySQL::handleSettingsDialog);
    connect(settingsThread, &InputSettingsThread::debugMessage, this, &MySQL::debugMessage);
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

void MySQL::handleSettingsDialog(bool inputNewSettings)
{
    if(inputNewSettings)
        inputMySQLSettings();

    if(!inputNewSettings)
        mySQLSettings_ = settings_.loadSettings(QFile(qApp->applicationDirPath() + "/mysqlsettings.db"), mySQLSettings_);
}

void MySQL::inputMySQLSettings()
{
    QTextStream s(stdin);
    QString dirBool;
    QString password;
    QString username;
    QString databaseName;
    QString hostName;
    QString caFullPath;
    QString clientKeyFullPath;
    QString clientCertFullPath;

    std::cout << "Set hostname or IP: ";
    hostName = s.readLine();

    std::cout << "Set database name: ";
    databaseName = s.readLine();

    std::cout << "Set username: ";
    username = s.readLine();

    std::cout << "Set password: ";
    password = s.readLine();

    std::cout << "Use .exe directory for certs and ca? y/n: ";
    dirBool = s.readLine();

    if(!(dirBool == "y" || dirBool == "n"))
    {
        emit debugMessage("MySQL user input error: incorrect user input for ssl cert locations. Input not y or n");
        emit debugMessage("Using .exe directory as default");
    }
    if(dirBool == "y")
    {
        std::cout << "Set CA cert name: ";
        caFullPath = "SSL_CA=" + qApp->applicationDirPath() + "/" + s.readLine() + ";";

        std::cout << "Set client key file name: ";
        clientKeyFullPath = "SSL_KEY=" + qApp->applicationDirPath() + "/" + s.readLine() + ";";

        std::cout << "Set client cert file name: ";
        clientCertFullPath = "SSL_CERT=" + qApp->applicationDirPath() + "/" + s.readLine() + ";";
    }

    if(dirBool == "n")
    {
        std::cout << "Set CA full file path: ";
        caFullPath = "SSL_CA=" + s.readLine() + ";";

        std::cout << "Set client key full file path: ";
        clientKeyFullPath = "SSL_KEY=" + s.readLine() + ";";

        std::cout << "Set client cert full file path: ";
        clientCertFullPath = "SSL_CERT=" + s.readLine() + ";";
    }

    qDebug() << "Please review settings.";
    qDebug() << password;
    qDebug() << username;
    qDebug() << databaseName;
    qDebug() << hostName;
    qDebug() << caFullPath;
    qDebug() << clientKeyFullPath;
    qDebug() << clientCertFullPath;

    mySQLSettings_["password"] = password;
    mySQLSettings_["userName"] = username;
    mySQLSettings_["databaseName"] = databaseName;
    mySQLSettings_["hostName"] = hostName;
    mySQLSettings_["caStr"] = caFullPath;
    mySQLSettings_["clientKeyStr"] = clientKeyFullPath;
    mySQLSettings_["clientCertStr"] = clientCertFullPath;

    settings_.saveSettings(QFile(qApp->applicationDirPath() + "/mysqlsettings.db"), mySQLSettings_);
}

bool MySQL::exportInvoiceResults(QMap<QString, QVariantList> invoiceResults)
{
    QString tableName = "invoice";
    return exportResults(tableName,invoiceResults);
}

bool MySQL::exportCustomerChainResults(QMap<QString, QVariantList> invoiceResults)
{
    QString tableName = "customerChain";
    return exportResults(tableName,invoiceResults);
}

bool MySQL::exportResults(const QString &tableName, QMap<QString, QVariantList> invoiceResults)
{
    bool success = false;
    if(invoiceResults.isEmpty())
        return success;

    {
        QSqlDatabase sslDB = QSqlDatabase::addDatabase("QMYSQL", mySQLSettings_["databaseName"].toString());
        emit debugMessage("Beginning MySQL upload");
        sslDB.setHostName(mySQLSettings_["hostName"].toString());
        sslDB.setDatabaseName(mySQLSettings_["databaseName"].toString());
        sslDB.setUserName(mySQLSettings_["userName"].toString());
        sslDB.setPassword(mySQLSettings_["password"].toString());
        sslDB.setConnectOptions(mySQLSettings_["caStr"].toString()
                + mySQLSettings_["clientKeyStr"].toString()
                + mySQLSettings_["clientCertStr"].toString());

        if(sslDB.open())
        {
            success = executeQueryAsString(sslDB,tableName, invoiceResults);
            if(!success)
                success = executeQueryAsBatch(sslDB,tableName, invoiceResults);
        }
        else
        {
            emit debugMessage("Failed to open MySQL database.");
            emit debugMessage(sslDB.lastError().text());
        }

        sslDB.close();
        emit debugMessage("Finished MySQL");
    }
    emit debugMessage("Cleaning up MySQL " + mySQLSettings_["databaseName"].toString() + " connection.");
    QSqlDatabase::removeDatabase(mySQLSettings_["databaseName"].toString());
    return success;
}

bool MySQL::executeQueryAsString(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> invoiceResults)
{
    bool success = false;
    QSqlQuery query(db);
    QStringList valueTuples;
    QString queryString = "INSERT INTO " + tableName +" (";
    QStringList columnsToUpdate;
    QStringList updateStatements;

    for(auto key:invoiceResults.keys())
    {
        QString keyTick = "`" + key + "`";
        columnsToUpdate.append(keyTick);
        updateStatements.append(keyTick + "=VALUES(" + keyTick + ")");
    }

    valueTuples = generateValueTuples(invoiceResults);
    queryString.append(columnsToUpdate.join(", ") + ") VALUES " + valueTuples.join(", "));
    queryString.append(" ON DUPLICATE KEY UPDATE " + updateStatements.join(", "));


    emit debugMessage(QString("Query length is " + QString::number(queryString.size()) + " char."));
    emit debugMessage(QString("Inserting " + QString::number(invoiceResults.first().size()) + " records into MySQL"));

    db.driver()->beginTransaction();

    if(query.exec(queryString))
    {
        success = true;
        emit debugMessage("MySQL string upload completed.");
    }
    else
    {
        success = false;
        emit debugMessage("Error in MySQL string query...");
        emit debugMessage(query.lastError().text());
    }
    db.driver()->commitTransaction();



    return success;
}

QStringList MySQL::generateValueTuples(QMap<QString, QVariantList> invoiceResults)
{
    QStringList valueList;
    QStringList valueTuples;
    for(int i = 0; i < invoiceResults.first().size(); ++i)
    {
        valueList.clear();

        for(auto key:invoiceResults.keys())
        {
            switch(invoiceResults[key][i].type()) {

            case QVariant::Type::Int:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(invoiceResults[key][i].toString());
                break;

            case QVariant::Type::LongLong:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(invoiceResults[key][i].toString());
                break;

            case QVariant::Type::Double:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(invoiceResults[key][i].toString());
                break;

            case QVariant::Type::String:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append("\"" + invoiceResults[key][i].toString().toLatin1() + "\"");
                break;

            case QVariant::Type::Date:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append("\"" + invoiceResults[key][i].toDate().toString("yyyy-MM-dd") + "\"");
                break;

            default:
                emit debugMessage(QString("Unsupported data type from AS400 database "
                                          + invoiceResults[key][i].toString()
                                          + " "
                                          + invoiceResults[key][i].type()));
                break;
            }
        }
        valueTuples.append("(" + valueList.join(", ") + ")");
    }
    return valueTuples;
}

bool MySQL::executeQueryAsBatch(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> invoiceResults)
{
    emit debugMessage("Falling back to batch insert.");

    bool success = false;
    int recordCount = 0;
    QString queryString = "INSERT INTO " + tableName + " (";
    QStringList columnsToUpdate;
    QStringList fieldList;
    QStringList updateStatements;
    QSqlQuery query(db);

    for(auto key:invoiceResults.keys())
    {
        QString keyTick = "`" + key + "`";
        columnsToUpdate.append(keyTick);
        updateStatements.append(keyTick + "=VALUES(" + keyTick + ")");
        fieldList.append("?");
    }

    recordCount = invoiceResults.first().size();

    queryString.append(columnsToUpdate.join(",") + ") VALUES (" + fieldList.join(", ") + ")");
    queryString.append(" ON DUPLICATE KEY UPDATE " + updateStatements.join(", "));
    query.prepare(queryString);

    for(auto results:invoiceResults)
        query.addBindValue(results);

    emit debugMessage("Beginning MySQL fallback batch insert of " + QString::number(recordCount) + " length");

    db.driver()->beginTransaction();

    if(query.execBatch())
        success = true;
    else
        emit debugMessage("Query Error in batch insert. Last error is: " + query.lastError().text());

    db.driver()->commitTransaction();

    if(success)
        emit debugMessage("Completed MySQL batch insert.");
    else
        emit debugMessage("Failed MySQL batch insert.");

    return success;
}
