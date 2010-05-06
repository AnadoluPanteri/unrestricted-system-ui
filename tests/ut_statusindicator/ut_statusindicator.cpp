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

#include <MOnDisplayChangeEvent>
#include "ut_statusindicator.h"
#include "statusindicator.h"
#include "statusindicatoranimationview.h"
#include "testcontextitem.h"
#include "inputmethodstatusindicatoradaptor_stub.h"

QHash<QString, TestContextItem *> testContextItems;

// Test context
class TestContext : public ApplicationContext
{
public:
    virtual ContextItem *createContextItem(const QString &key) {
        Q_UNUSED(key)

        testContextItems[key] = new TestContextItem;

        return testContextItems[key];
    }
};

// Stubs for status indicator views

QVariant gModelValue;

TestStatusIndicatorIconView::TestStatusIndicatorIconView(StatusIndicator *controller) :
    StatusIndicatorIconView(controller)
{
}

void TestStatusIndicatorIconView::updateData(const QList<const char *>& modifications)
{
    MWidgetView::updateData(modifications);
    const char *member;
    foreach(member, modifications) {
        if (member == StatusIndicatorModel::Value) {
            gModelValue = model()->value();
        }
    }
}



void Ut_StatusIndicator::init()
{
    testContext = new TestContext();
    testContextItems.clear();
    gModelValue.clear();
}

void Ut_StatusIndicator::cleanup()
{
    delete testContext;
}

void Ut_StatusIndicator::initTestCase()
{
    // MApplications must be created manually these days due to theme system changes
    static int argc = 1;
    static char *app_name = (char *)"./ut_statusindicator";
    app = new MApplication(argc, &app_name);
}

void Ut_StatusIndicator::cleanupTestCase()
{
    // Destroy the MApplication
    delete app;
}

void Ut_StatusIndicator::testModelUpdates()
{
    MOnDisplayChangeEvent exitDisplayEvent(MOnDisplayChangeEvent::FullyOffDisplay, QRectF());
    MOnDisplayChangeEvent enterDisplayEvent(MOnDisplayChangeEvent::FullyOnDisplay, QRectF());
    StatusIndicator *statusIndicator = new ClockAlarmStatusIndicator(*testContext);

    // When the application is visible the model should be updated
    qApp->sendEvent(statusIndicator, &enterDisplayEvent);
    testContextItems["UserAlarm.Present"]->setValue(QVariant(true));
    QCOMPARE(statusIndicator->model()->value(), QVariant(1));

    // When the application is not visible the model should not be updated
    qApp->sendEvent(statusIndicator, &exitDisplayEvent);
    testContextItems["UserAlarm.Present"]->setValue(QVariant(false));
    QCOMPARE(statusIndicator->model()->value(), QVariant(1));

    // When the application becomes visible the model should be updated
    qApp->sendEvent(statusIndicator, &enterDisplayEvent);
    QCOMPARE(statusIndicator->model()->value(), QVariant(0));

    delete statusIndicator;
}

void Ut_StatusIndicator::testPhoneNetworkSignalStrength()
{
    StatusIndicator *statusIndicator = new PhoneNetworkSignalStrengthStatusIndicator(*testContext);
    statusIndicator->setView(new TestStatusIndicatorIconView(statusIndicator));

    testContextItems["Cellular.SignalStrength"]->setValue(QVariant(100));

    QVERIFY(statusIndicator->model()->value().type() == QVariant::Double);
    QCOMPARE(qRound(statusIndicator->model()->value().toDouble() * 100), 100);

    delete statusIndicator;
}

void Ut_StatusIndicator::testBattery()
{
    StatusIndicator *statusIndicator = new BatteryStatusIndicator(*testContext);

    testContextItems["Battery.ChargePercentage"]->setValue(QVariant(100));
    QVERIFY(statusIndicator->model()->value().type() == QVariant::Double);
    QCOMPARE(qRound(statusIndicator->model()->value().toDouble() * 100), 100);

    testContextItems["Battery.IsCharging"]->setValue(QVariant(false));
    QVERIFY(statusIndicator->objectName().indexOf("Level") >= 0);

    testContextItems["Battery.IsCharging"]->setValue(QVariant(true));
    QVERIFY(statusIndicator->objectName().indexOf("Charging") >= 0);

    delete statusIndicator;
}

void Ut_StatusIndicator::testAlarm()
{
    StatusIndicator *statusIndicator = new ClockAlarmStatusIndicator(*testContext);

    testContextItems["UserAlarm.Present"]->setValue(QVariant(false));
    QCOMPARE(statusIndicator->model()->value(), QVariant(false));

    testContextItems["UserAlarm.Present"]->setValue(QVariant(true));
    QCOMPARE(statusIndicator->model()->value(), QVariant(true));

    delete statusIndicator;
}

void Ut_StatusIndicator::testBluetooth()
{
    StatusIndicator *statusIndicator = new BluetoothStatusIndicator(*testContext);

    testContextItems["Bluetooth.Enabled"]->setValue(QVariant(false));
    QCOMPARE(statusIndicator->model()->value(), QVariant(false));

    testContextItems["Bluetooth.Enabled"]->setValue(QVariant(true));
    QCOMPARE(statusIndicator->model()->value(), QVariant(true));

    delete statusIndicator;
}

void Ut_StatusIndicator::testInternetConnection()
{
    StatusIndicator *statusIndicator = new InternetConnectionStatusIndicator(*testContext);

    testContextItems["Internet.SignalStrength"]->setValue(QVariant(100));
    QVERIFY(statusIndicator->model()->value().type() == QVariant::Double);
    QCOMPARE(qRound(statusIndicator->model()->value().toDouble() * 100), 100);

    delete statusIndicator;
}

void Ut_StatusIndicator::testAnimation()
{
    MOnDisplayChangeEvent exitDisplayEvent(MOnDisplayChangeEvent::FullyOffDisplay, QRectF());
    MOnDisplayChangeEvent enterDisplayEvent(MOnDisplayChangeEvent::FullyOnDisplay, QRectF());
    StatusIndicator *statusIndicator = new BatteryStatusIndicator(*testContext);

    testContextItems["Battery.IsCharging"]->setValue(QVariant(true));
    qApp->sendEvent(statusIndicator, &exitDisplayEvent);
    QCOMPARE(statusIndicator->model()->animate(), false);

    qApp->sendEvent(statusIndicator, &enterDisplayEvent);
    QCOMPARE(statusIndicator->model()->animate(), true);
    delete statusIndicator;
}

void Ut_StatusIndicator::testPhoneNetwork()
{
    StatusIndicator *statusIndicator = new PhoneNetworkStatusIndicator(*testContext);
    testContextItems["Cellular.NetworkName"]->setValue(QVariant("foobarbarabush"));

    QVERIFY(statusIndicator->model()->value().type() == QVariant::String);
    QCOMPARE(statusIndicator->model()->value(), QVariant("foobarbarabus"));

    delete statusIndicator;
}

void Ut_StatusIndicator::testInputMethod()
{
    InputMethodStatusIndicator *statusIndicator = new InputMethodStatusIndicator;
    statusIndicator->setIconID("test");
    QVERIFY(statusIndicator->model()->value().type() == QVariant::String);
    QCOMPARE(statusIndicator->model()->value(), QVariant("test"));

    delete statusIndicator;
}

void Ut_StatusIndicator::testCall()
{
    StatusIndicator *statusIndicator = new CallStatusIndicator(*testContext);
    testContextItems["Phone.Call"]->setValue(QVariant("inactive"));
    QVERIFY(statusIndicator->model()->value().type() == QVariant::Int);
    QCOMPARE(statusIndicator->model()->value(), QVariant(0));
    QVERIFY(statusIndicator->objectName().indexOf("Ringing") < 0);
    QVERIFY(statusIndicator->objectName().indexOf("Ongoing") < 0);

    testContextItems["Phone.Call"]->setValue(QVariant("ringing"));
    QVERIFY(statusIndicator->objectName().indexOf("Ringing") >= 0);
    QCOMPARE(statusIndicator->model()->value(), QVariant(0));

    testContextItems["Phone.Call"]->setValue(QVariant("active"));
    QVERIFY(statusIndicator->objectName().indexOf("Ongoing") >= 0);
    QCOMPARE(statusIndicator->model()->value(), QVariant(0));

    testContextItems["Phone.Call"]->setValue(QVariant("knocking"));
    QVERIFY(statusIndicator->objectName().indexOf("Ringing") >= 0);
    QCOMPARE(statusIndicator->model()->value(), QVariant(0));

    testContextItems["Phone.Call"]->setValue(QVariant("inactive"));
    testContextItems["Phone.Muted"]->setValue(QVariant(true));
    QVERIFY(statusIndicator->objectName().indexOf("Ringing") < 0);
    QVERIFY(statusIndicator->objectName().indexOf("Ongoing") < 0);
    QCOMPARE(statusIndicator->model()->value(), QVariant(0));

    testContextItems["Phone.Call"]->setValue(QVariant("ringing"));
    QVERIFY(statusIndicator->objectName().indexOf("Ringing") >= 0);
    QCOMPARE(statusIndicator->model()->value(), QVariant(0));

    testContextItems["Phone.Call"]->setValue(QVariant("active"));
    QVERIFY(statusIndicator->objectName().indexOf("Ongoing") >= 0);
    QCOMPARE(statusIndicator->model()->value(), QVariant(1));

    testContextItems["Phone.Call"]->setValue(QVariant("knocking"));
    QVERIFY(statusIndicator->objectName().indexOf("Ringing") >= 0);
    QCOMPARE(statusIndicator->model()->value(), QVariant(0));
}

void Ut_StatusIndicator::testProfile()
{
    StatusIndicator *statusIndicator = new ProfileStatusIndicator(*testContext);
    testContextItems["Profile.Name"]->setValue(QVariant("silent"));
    QVERIFY(statusIndicator->objectName().indexOf("Silent") >= 0);
    testContextItems["Profile.Name"]->setValue(QVariant("default"));
    QVERIFY(statusIndicator->objectName().indexOf("Silent") < 0);

    delete statusIndicator;
}

QTEST_APPLESS_MAIN(Ut_StatusIndicator)
