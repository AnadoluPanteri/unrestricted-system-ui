/****************************************************************************
 **
 ** Copyright (C) 2010 Nokia Corporation and/or its subsidiary (-ies).
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
#ifndef BATTERYBUSINESSLOGIC_H
#define BATTERYBUSINESSLOGIC_H

#include <QObject>
#include <QTimer>
#include <QScopedPointer>
#include <MNotification>

#ifdef HAVE_QMSYSTEM
#include <qmled.h>
#include <qmbattery.h>
#include <qmdevicemode.h>
#endif

class LowBatteryNotifier;
class MNotification;

/*!
 * Implements the configuration and state for the battery, the power save mode.
 */
class BatteryBusinessLogic : public QObject {
    Q_OBJECT

public:
    /*!
     * Creates a new battery business logic.
     *
     * \param the parent QObject
     */
    BatteryBusinessLogic(QObject *parent = NULL);

    /*!
     * Destroys the battery business logic.
     */
    virtual ~BatteryBusinessLogic();

    typedef enum {
        NotificationCharging,
        NotificationChargingComplete,
        NotificationRemoveCharger,
        NotificationChargingNotStarted,
        NotificationRechargeBattery,
        NotificationEnteringPSM,
        NotificationExitingPSM,
        NotificationLowBattery,
        NotificationNoEnoughPower
    } NotificationID;

public slots:
    //! Initializes the battery status from the current values given by QmBattery
    void initBattery();

    //! Sends a low battery notification
    void lowBatteryAlert();

    /*!
     * Sets the touch screen lock active state so notifications can be enabled/disabled based on that.
     *
     * \param active \c true if the touch screen lock is active, \c false otherwise
     */
    void setTouchScreenLockActive(bool active);

private slots:
#ifdef HAVE_QMSYSTEM
    void batteryStateChanged(MeeGo::QmBattery::BatteryState state);
    void chargingStateChanged(MeeGo::QmBattery::ChargingState state);
    void batteryChargerEvent(MeeGo::QmBattery::ChargerType type);
    void devicePSMStateChanged(MeeGo::QmDeviceMode::PSMState PSMState);
#endif
    void utiliseLED(bool activate, const QString &pattern);

private:
    //! Sends a notification based on the notification ID
    void sendNotification(BatteryBusinessLogic::NotificationID id);

    //! Sends a notification
    void sendNotification(const QString &eventType, const QString &text, const QString &icon = QString(""));

    //! Removes the current notification if its type is one listed in eventTypes
    void removeNotification(const QStringList &eventTypes);

    //! Returns the charging image ID based on the current battery level
    QString chargingImageId();

    //! Starts the low battery notifier if not already started
    void startLowBatteryNotifier();

    //! Stops the low battery notifier if not already stopped
    void stopLowBatteryNotifier();

    //! Low battery notifier for getting notifications about low battery state
    LowBatteryNotifier *lowBatteryNotifier;

    //! The current notification
    QScopedPointer<MNotification> notification;

    //! Timer for checking whether the current notification can be removed or not
    QTimer notificationTimer;

    //! Whether the touch screen lock is active or not
    bool touchScreenLockActive;

#ifdef HAVE_QMSYSTEM
    //! For getting battery state
    MeeGo::QmBattery *qmBattery;

    //! For getting device mode
    MeeGo::QmDeviceMode *qmDeviceMode;

    //! For controlling the LED
    MeeGo::QmLED *qmLed;

    //! The current charger type
    MeeGo::QmBattery::ChargerType chargerType;
#endif

#ifdef UNIT_TEST
    friend class Ut_BatteryBusinessLogic;
#endif
};

#endif
