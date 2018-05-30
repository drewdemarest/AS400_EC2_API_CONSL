#ifndef AS400_H
#define AS400_H

#include <QObject>
#include <QtSql>
#include <QtCore>

enum AS400QueryType {Invoice, CustomerChain};

class AS400 : public QObject
{
    Q_OBJECT
public:
    explicit AS400(const QString &systemIP,
                   const QString &username,
                   const QString &password,
                   QObject *parent = nullptr);

    bool getCustomerChains(const int chunkSize);

    bool getInvoiceData(const QDate &minDate,
                        const QDate &maxDate,
                        const int chunkSize);

    bool getCustomerData();

    bool getRouteAssignmentData();

signals:
    void invoiceDataResults(QMap<QString,QVariantList> sqlResults);
    void customerChainResults(QMap<QString,QVariantList> sqlResults);
    void customerDataResults(QMap<QString,QVariantList> sqlResults);
    void routeAssignmentResults(QMap<QString,QVariantList> sqlResults);
    void debugMessage(QString dbg);

public slots:

private:
    bool queryAS400(const AS400QueryType queryType, const QString &queryString, const int chunkSize);
    void processQuery(const AS400QueryType queryType, QSqlQuery &query, const int chunkSize);
    QString connectString_;
    QString username_;
    QString password_;
    //Data Formats
};

#endif // AS400_H
