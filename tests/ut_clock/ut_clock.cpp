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
#include <DuiOnDisplayChangeEvent>
#include "ut_clock.h"
#include "clock.h"

Maemo::QmTime::TimeFormat Maemo::QmTime::getTimeFormat()
{
    return Ut_Clock::expectedTimeFormat;
}

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
Maemo::QmTime::TimeFormat Ut_Clock::expectedTimeFormat;

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
    expectedTimeFormat = Maemo::QmTime::format12h;
    m_subject = new Clock;
    connect(this, SIGNAL(timeOrSettingsChanged(QmTimeWhatChanged)), m_subject, SLOT(updateSettings(QmTimeWhatChanged)));
}

// Called after every testfunction
void Ut_Clock::cleanup()
{
    delete m_subject;
}

void Ut_Clock::test24HourModeDuringCreation()
{
    // In init() qmsystem is set up to return 12 hour mode so the clock should be in 12 hour mode
    QCOMPARE(m_subject->model()->timeFormat24h(), false);
}

void Ut_Clock::test24HourModeToggling()
{
    // Check that if qmsystem returns 24 hour mode the clock is in 24 hour mode
    expectedTimeFormat = Maemo::QmTime::format24h;
    emit timeOrSettingsChanged(Maemo::QmTimeOnlySettingsChanged);
    QCOMPARE(m_subject->model()->timeFormat24h(), true);

    // Check that if qmsystem returns 12 hour mode the clock is in 12 hour mode
    expectedTimeFormat = Maemo::QmTime::format12h;
    emit timeOrSettingsChanged(Maemo::QmTimeOnlySettingsChanged);
    QCOMPARE(m_subject->model()->timeFormat24h(), false);
}

void Ut_Clock::test24HourModeNotToggledWhenSettingsAreNotChanged()
{
    // If something else than settings changes we should not care
    expectedTimeFormat = Maemo::QmTime::format24h;
    emit timeOrSettingsChanged(Maemo::QmTimeTimeChanged);
    QCOMPARE(m_subject->model()->timeFormat24h(), false);
}

void Ut_Clock::testModelUpdates()
{
    DuiOnDisplayChangeEvent exitDisplayEvent(DuiOnDisplayChangeEvent::FullyOffDisplay, QRectF());
    DuiOnDisplayChangeEvent enterDisplayEvent(DuiOnDisplayChangeEvent::FullyOnDisplay, QRectF());

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

QTEST_MAIN(Ut_Clock)