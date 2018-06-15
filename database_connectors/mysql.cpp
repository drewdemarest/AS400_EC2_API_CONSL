#include "mysql.h"

MySQL::MySQL(QObject *parent) : QObject(parent)
{
    mySQLSettings_ = settings_.loadSettings(QFile(dbPath_), mySQLSettings_);
}

void MySQL::init()
{
    connect(&settings_, &JsonSettings::debugMessage, this, &MySQL::debugMessage);
}

bool MySQL::exportInvoiceResults(QMap<QString, QVariantList> sqlResutls)
{
    QString tableName = "invoice";
    return exportResults(tableName,sqlResutls);
}

bool MySQL::exportCustomerChainResults(QMap<QString, QVariantList> sqlResutls)
{
    QString tableName = "customerChain";
    return exportResults(tableName,sqlResutls);
}

bool MySQL::exportOpenOrderHeaderResult(bool needToTruncate, QMap<QString, QVariantList> sqlResutls)
{
    QString tableName = "openOrderHeader";
    if(needToTruncate)
    {
        if(truncateATable(tableName))
            return exportResults(tableName,sqlResutls);

        else
            return false;
    }
    else
        return exportResults(tableName,sqlResutls);
}

bool MySQL::exportOpenOrderDetailResult(bool needToTruncate, QMap<QString, QVariantList> sqlResutls)
{
    QString tableName = "openOrderDetail";

    if(needToTruncate)
    {
        if(truncateATable(tableName))
            return exportResults(tableName,sqlResutls);

        else
            return false;
    }
    else
        return exportResults(tableName,sqlResutls);
}

bool MySQL::truncateATable(const QString &tableName)
{
    QString truncateTableQuery = "TRUNCATE " + tableName;

    bool success = false;

    {
        QSqlDatabase sslDB = QSqlDatabase::addDatabase("QMYSQL", mySQLSettings_["databaseName"].toString());

        QString connectString =   "SSL_CA=" + mySQLSettings_["caStr"].toString() + ";"
                                + "SSL_KEY=" + mySQLSettings_["clientKeyStr"].toString() + ";"
                                + "SSL_CERT=" + mySQLSettings_["clientCertStr"].toString() + ";";

        emit debugMessage("Beginning MySQL upload");
        sslDB.setHostName(mySQLSettings_["hostName"].toString());
        sslDB.setDatabaseName(mySQLSettings_["databaseName"].toString());
        sslDB.setUserName(mySQLSettings_["userName"].toString());
        sslDB.setPassword(mySQLSettings_["password"].toString());
        sslDB.setConnectOptions(connectString);

        if(sslDB.open())
        {
            QSqlQuery query(sslDB);
            success = query.exec(truncateTableQuery);
            if(!success)
            {
                emit debugMessage("Failed to truncate MySQL database "
                                    + mySQLSettings_["databaseName"].toString()
                                    + " for table "
                                    + tableName);

                emit debugMessage("Truncate query error: " + query.lastError().text());
            }
        }
        else
        {
            emit debugMessage("Failed to open MySQL database.");
            emit debugMessage(sslDB.lastError().text());
        }

        sslDB.close();
        emit debugMessage("Finished MySQL. Truncated database "
                            + mySQLSettings_["databaseName"].toString()
                            + " for table " + tableName);
    }
    emit debugMessage("Cleaning up MySQL " + mySQLSettings_["databaseName"].toString() + " connection.");
    QSqlDatabase::removeDatabase(mySQLSettings_["databaseName"].toString());
    return success;
}

bool MySQL::exportResults(const QString &tableName, QMap<QString, QVariantList> invoiceResults)
{
    bool success = false;
    if(invoiceResults.isEmpty())
        return success;

    {
        QSqlDatabase sslDB = QSqlDatabase::addDatabase("QMYSQL", mySQLSettings_["databaseName"].toString());

        QString connectString =   "SSL_CA=" + mySQLSettings_["caStr"].toString() + ";"
                                + "SSL_KEY=" + mySQLSettings_["clientKeyStr"].toString() + ";"
                                + "SSL_CERT=" + mySQLSettings_["clientCertStr"].toString() + ";";

        emit debugMessage("Beginning MySQL upload");
        sslDB.setHostName(mySQLSettings_["hostName"].toString());
        sslDB.setDatabaseName(mySQLSettings_["databaseName"].toString());
        sslDB.setUserName(mySQLSettings_["userName"].toString());
        sslDB.setPassword(mySQLSettings_["password"].toString());
        sslDB.setConnectOptions(connectString);

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
