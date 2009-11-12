#ifndef UT_LOWBATTERYNOTIFIER_H
#define UT_LOWBATTERYNOTIFIER_H

#include "notifier.h"

#include <QtTest/QtTest>
#include <QObject>
#include <QThread>

class LowBatteryNotifier;
class QTime;

class LowBatteryHelper : public QObject
{
    Q_OBJECT

public:
    LowBatteryHelper() : QObject() { ;}
    void start();    
    QList<int> notificationTimes();

public slots:
    void notificationShown();

private: // attributes
    QList<int> times;
    QTime time;

};

class Ut_LowBatteryNotifier : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();

    void testShowNotificationInActiveUse();
    void testShowNotificationInDiverseUse();
    void testShowNotificationInInactiveUse();

private: //attributes
    LowBatteryNotifier *m_subject;
    LowBatteryHelper *m_helper;
    QTime time;
    int shown;
};

#endif