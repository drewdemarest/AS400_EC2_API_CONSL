#include <QCoreApplication>
#include "upload/uploadschedule.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    UploadSchedule schedule;
    schedule.init();
    schedule.run();

    return a.exec();
}
