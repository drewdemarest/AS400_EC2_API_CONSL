#include "jsonsettings.h"

JsonSettings::JsonSettings(QObject *parent) : QObject(parent)
{
}


QJsonObject JsonSettings::loadSettings(const QFile &dbFile,const QJsonObject &jsonSettings)
{
    QJsonObject populatedJsonSettings;
    emit debugMessage("Loading JsonSettings.");
    if(!dbFile.fileName().isEmpty() && !jsonSettings.isEmpty())
    {
        if(!doesDatabaseExist(dbFile))
        {
            makeInitalSettingsDatabase(dbFile);
        }

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbFile.fileName());
            db.setDatabaseName(dbFile.fileName());
            if(db.open())
            {
                populatedJsonSettings = selectSettingsFromDB(db,jsonSettings);
            }
            else
            {
                emit debugMessage("Failed to load settings database " +
                                  dbFile.fileName() +
                                  " returning empty settings object");

                return jsonSettings;
            }
            db.close();
        }
        QSqlDatabase::removeDatabase(dbFile.fileName());
    }
    else
    {
        emit debugMessage("JsonSettings::loadSettings error, dbPath or JsonObject is null");
        return jsonSettings;
    }

    return populatedJsonSettings;
}

QJsonObject JsonSettings::selectSettingsFromDB(QSqlDatabase &db, QJsonObject jsonSettings)
{
    QSqlQuery query(db);
    QStringList jsonKeys = jsonSettings.keys();
    for(int i=0; i < jsonKeys.size(); ++i)
    {
        jsonKeys[i].append("\"");
        jsonKeys[i].prepend("\"");
    }

    QString queryString  = "SELECT * FROM settings WHERE key IN (" + jsonKeys.join(", ") + ")";
    if(query.exec(queryString))
    {
        emit debugMessage("JsonSettings::loadSettings query success.");

        while(query.next())
        {
            if(query.value("isJsonArray").toBool())
                jsonSettings[query.value("key").toString()] = QJsonDocument::fromJson(query.value("value").toString().toUtf8()).array();
            else
                jsonSettings[query.value("key").toString()] = query.value("value").toJsonValue();
        }
    }
    else
        emit debugMessage("JsonSettings::loadSettings ERROR: " + query.lastError().text());

    return jsonSettings;
}

bool JsonSettings::saveSettings(const QFile &dbFile, const QJsonObject &jsonSettings)
{
    bool success = false;
    if(dbFile.fileName().isEmpty())
        return success;

    if(!doesDatabaseExist(dbFile))
        makeInitalSettingsDatabase(dbFile);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbFile.fileName());
        db.setDatabaseName(dbFile.fileName());
        if(db.open())
        {
            success = insertSettingsIntoDB(db, jsonSettings);
            db.close();
        }
        else
        {
            emit debugMessage("Could not save json object to database. DB error = " + db.lastError().text());
        }
    }
    QSqlDatabase::removeDatabase(dbFile.fileName());

    return success;
}

bool JsonSettings::insertSettingsIntoDB(QSqlDatabase &db, const QJsonObject &jsonSettings)
{
    bool success =false;
    QSqlQuery query(db);
    QString queryString = makeInsertString(jsonSettings);
    success = query.exec(queryString);

    if(!success)
        emit debugMessage("Could not save json object to database. Query error = " + query.lastError().text());

    return success;
}


QString JsonSettings::makeInsertString(const QJsonObject &jsonSettings)
{
    QString queryString = "INSERT or REPLACE into settings VALUES ";
    QStringList values;
    QStringList valueTuples;
    for(auto key: jsonSettings.keys())
    {
        values.clear();
        if(jsonSettings[key].isArray())
        {
            QJsonDocument arrayToString;
            arrayToString.setArray(jsonSettings[key].toArray());
            values << QString("\"" + key + "\"") << QString("\"" + QString(arrayToString.toJson()) + "\"") << QString::number(true);
            valueTuples.append("(" + values.join(", ") + ")");
        }
        else{
            values << QString("\"" + key + "\"") << QString("\"" + jsonSettings[key].toString() + "\"")  << QString::number(false);
            valueTuples.append("(" + values.join(", ") + ")");
        }
    }
    queryString.append(valueTuples.join(", "));
    return queryString;
}


bool JsonSettings::doesDatabaseExist(const QFile &dbFile)
{
    if(dbFile.exists())
        return true;
    else
        return false;
}

bool JsonSettings::makeInitalSettingsDatabase(const QFile &dbFile)
{
    bool success = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbFile.fileName());

        db.setDatabaseName(dbFile.fileName());
        if(db.open())
        {
            QSqlQuery query(db);
            success = query.exec("CREATE TABLE settings(key text PRIMARY KEY, value text, isJsonArray integer)");
            query.finish();
            if(!success)
                emit debugMessage("Failed to exec init query. " + query.lastError().text());
        }
        else
             emit debugMessage("Failed to make init db. DB could not be opened." +  db.lastError().text());

        db.close();
    }
    QSqlDatabase::removeDatabase(dbFile.fileName());
    return success;
}
