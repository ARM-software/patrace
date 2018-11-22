#include "percentage_thread.h"

PercentageThread::PercentageThread(QObject *parent):
    QObject(parent)
{}

PercentageThread::~PercentageThread()
{}

void PercentageThread::dowork()
{
    static int last_progress_num = -1;
    while(Pat_reading == true)
    {
        if (progress_num != last_progress_num) {
            last_progress_num = progress_num;
            emit startShowingPercentage();
        }
    }
}

void PercentageThread::dowork_saving()
{
    static int last_progress_num = -1;
    while(Pat_saving == true)
    {
        if (progress_num != last_progress_num) {
            last_progress_num = progress_num;
            emit startShowingPercentage_saving();
        }
    }
}
