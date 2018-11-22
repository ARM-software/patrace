#ifndef PERCENTAGE_THREAD_H
#define PERCENTAGE_THREAD_H

#include <QString>
#include <QObject>
#include "open_thread.h"

class PercentageThread : public QObject
{
    Q_OBJECT

public:
    PercentageThread(QObject* parent = NULL);
    ~PercentageThread();

public slots:
    void dowork();
    void dowork_saving();

signals:
    void startShowingPercentage();
    void startShowingPercentage_saving();
};

#endif // PERCENTAGE_THREAD_H
