/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (directui@nokia.com)
**
** This file is part of systemui.
**
** If you have questions regarding the use of this file, please contact
** Nokia at directui@nokia.com.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#include <QtTest/QtTest>
#include <MOnDisplayChangeEvent>
#include <QScopedPointer>
#include "ut_clock.h"
#include "clock.h"

#ifdef HAVE_QMSYSTEM
Q_DECLARE_METATYPE(MeeGo::QmTime::TimeFormat);
Q_DECLARE_METATYPE(MeeGo::QmTimeWhatChanged);

MeeGo::QmTime::TimeFormat MeeGo::QmTime::getTimeFormat()
{
    return Ut_Clock::expectedTimeFormat;
}
#endif

QDateTime QDateTime::currentDateTime()
{
    return Ut_Clock::expectedDateTime;
}

void QTimer::start(int msec)
{
    Ut_Clock::timerTimeout = msec;
}

void QTimer::stop()
{
    Ut_Clock::timerTimeout = -1;
}

int Ut_Clock::timerTimeout;
QDateTime Ut_Clock::expectedDateTime;
#ifdef HAVE_QMSYSTEM
MeeGo::QmTime::TimeFormat Ut_Clock::expectedTimeFormat;
#endif

// Called before the first testfunction is executed
void Ut_Clock::initTestCase()
{
    expectedDateTime = QDateTime(QDate(2000, 1, 1));
}

// Called after the last testfunction was executed
void Ut_Clock::cleanupTestCase()
{
}

// Called before each testfunction is executed
void Ut_Clock::init()
{
#ifdef HAVE_QMSYSTEM
    expectedTimeFormat = MeeGo::QmTime::format12h;
#endif

    m_subject = new Clock;
    connect(this, SIGNAL(shortDisplayMode(bool)),
            m_subject, SLOT(setShortDisplay(bool)));

#ifdef HAVE_QMSYSTEM
    connect(this, SIGNAL(timeOrSettingsChanged(MeeGo::QmTimeWhatChanged)),
            m_subject, SLOT(updateSettings(MeeGo::QmTimeWhatChanged)));
#endif
}

// Called after every testfunction
void Ut_Clock::cleanup()
{
    delete m_subject;
}

void Ut_Clock::testConstruction()
{
#ifdef HAVE_QMSYSTEM
    QVERIFY(disconnect(&m_subject->qmTime,
                       SIGNAL(timeOrSettingsChanged(MeeGo::QmTimeWhatChanged)),
                       m_subject,
                       SLOT(updateSettings(MeeGo::QmTimeWhatChanged))));
#endif
    QVERIFY(disconnect(&m_subject->timer,
                       SIGNAL(timeout()),
                       m_subject, SLOT(updateModelAndSetupTimer())));
    QVERIFY(m_subject->timer.isSingleShot());
    QDateTime nextUpdateTime = expectedDateTime.addSecs(60);
    QTime time = nextUpdateTime.time();
    time.setHMS(time.hour(), time.minute(), 0);
    nextUpdateTime.setTime(time);
    QVERIFY(timerTimeout > expectedDateTime.msecsTo(nextUpdateTime));
}

void Ut_Clock::testTimeUpdate()
{
    // Check that time was initialized correctly
    QCOMPARE(m_subject->model()->time(), expectedDateTime);

#ifdef HAVE_QMSYSTEM
    // If qmsystem notifies up that time changed, model should be updated accordingly
    expectedDateTime = QDateTime(QDate(2010, 1, 1));
    emit timeOrSettingsChanged(MeeGo::QmTimeTimeChanged);
    QCOMPARE(m_subject->model()->time(), expectedDateTime);
#endif
}

void Ut_Clock::testModelUpdates()
{
    MOnDisplayChangeEvent exitDisplayEvent(MOnDisplayChangeEvent::FullyOffDisplay, QRectF());
    MOnDisplayChangeEvent enterDisplayEvent(MOnDisplayChangeEvent::FullyOnDisplay, QRectF());

    // The timer should be running by default and the model should contain the current time
    QVERIFY(timerTimeout >= 0);
    QCOMPARE(m_subject->model()->time(), expectedDateTime);

    // When the application becomes invisible the timer should stop
    qApp->sendEvent(m_subject, &exitDisplayEvent);
    QCOMPARE(timerTimeout, -1);

    // When the application becomes visible the timer should start and the model should be updated to contain the current time
    expectedDateTime = QDateTime(QDate(2001, 1, 1));
    qApp->sendEvent(m_subject, &enterDisplayEvent);
    QVERIFY(timerTimeout >= 0);
    QCOMPARE(m_subject->model()->time(), expectedDateTime);
}

void Ut_Clock::testShortDisplayToggling()
{
    // Check that the "setShortDisplay" slot sets the correct model field
    emit shortDisplayMode(true);
    QCOMPARE(m_subject->model()->shortDisplay(), true);
    emit shortDisplayMode(false);
    QCOMPARE(m_subject->model()->shortDisplay(), false);
}

QTEST_MAIN(Ut_Clock)
