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

#ifndef UT_WIDGETNOTIFICATIONSINK_H
#define UT_WIDGETNOTIFICATIONSINK_H

#include <QtTest/QtTest>
#include <QObject>
#include "notification.h"

class DuiApplication;
class TestWidgetNotificationSink;
class QGraphicsWidget;
class QAction;
class TestNotificationParameters;

class Ut_WidgetNotificationSink : public QObject
{
    Q_OBJECT

public:
    static QList<QString> contents;
    static QHash<const QGraphicsWidget *, QList<QAction *> > actions;
    static int actionTriggeredCount;

    static QImage *testImage;

signals:
    void click();

private slots:
    // Called before the first testfunction is executed
    void initTestCase();
    // Called after the last testfunction was executed
    void cleanupTestCase();
    // Called before each testfunction is executed
    void init();
    // Called after every testfunction
    void cleanup();

    void testWithoutEventTypeOrIconId();
    void testWithEventTypeWithoutIconId();
    void testWithEventTypeAndIconId();
    void testWithoutEventTypeWithIconId();
    void testUpdateActions();
    void testInfoBannerClicking();
    void testInfoBannerClickingWhenNotUserRemovableInParameters();
    void testInfoBannerClickingWhenNotUserRemovableByEventType();
    void testInfoBannerCreationWithRemoteAction();
    void testInfoBannerCreationWithoutRemoteAction();
    void testInfoBannerCreationWithNotificationParameters();

    void testLoadIconDefaultSize();
    void testLoadIconSmallerThanDefault();
    void testLoadIconScaleDownToFit();
    void testLoadIconScaleToFitBigHeight();
    void testLoadIconScaleToFitBigWidth();
    void testLoadIconTooBig();
    void testLoadIconWidthTooBig();
    void testLoadIconHeightTooBig();
    void testLoadIconBrokenImage();
    void testLoadIconCannotReadImage();

private:
    // Helper for the "test clicking when not user removable" cases
    void testInfoBannerClickingWhenNotUserRemovable(TestNotificationParameters &parameters);

    // HomeApplication
    DuiApplication *app;
    // The object being tested
    TestWidgetNotificationSink *m_subject;
};

#endif