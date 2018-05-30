#ifndef AS400_H
#define AS400_H

#include <QObject>
#include <QtSql>
#include <QtCore>

class AS400 : public QObject
{
    Q_OBJECT
public:
    explicit AS400(const QString &systemIP,
                   const QString &username,
                   const QString &password,
                   QObject *parent = nullptr);

    bool getInvoiceData(const QDate &minDate,
                        const QDate &maxDate,
                        const int chunkSize);

    bool getCustomerData();

    bool getRouteAssignmentData();

signals:
    void invoiceDataResults(QMap<QString,QVariantList> sqlResults);
    void customerDataResults(QMap<QString,QVariantList> sqlResults);
    void routeAssignmentResults(QMap<QString,QVariantList> sqlResults);
    void debugMessage(QString dbg);

public slots:

private:
    void processQuery(QSqlQuery & query, const int chunkSize);
    QString connectString_;
    QString username_;
    QString password_;
    //Data Formats
    QMap<QString,QVariantList> invoiceDataFmt_ {};
};

#endif // AS400_H
