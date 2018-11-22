#ifndef OPEN_THREAD_H
#define OPEN_THREAD_H

#include <QString>
#include <QObject>

#include <sys/stat.h>
#include <iostream>
#include "tool/config.hpp"
#include "common/os_time.hpp"
#include "eglstate/common.hpp"
#include "tool/pat_editor/commonData.hpp"

extern bool Pat_reading;
extern bool Pat_saving;

class OpenJson_Worker : public QObject
{
    Q_OBJECT
public:
    OpenJson_Worker(QObject* parent = NULL);
    ~OpenJson_Worker();

signals:
    void resultReady(const QStringList &result, int length, int number);
    void extractFinish();
    void startShowingPercentage();

public slots:
    void openJson(const QStringList&);
    void extractPat(const QString&, const QString&);
    void mergePat(const QString&, const QString&);
};

#endif // OPEN_THREAD_H
