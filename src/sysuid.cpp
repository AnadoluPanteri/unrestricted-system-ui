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

#include <MLocale>
#include <MTheme>
#include <MLocale>

#include <QDBusConnection>
#include <QDebug>

#include "usbui.h"
#include "sysuid.h"
#include "sysuidrequest.h"
#include "batterybusinesslogic.h"
#include "lockscreenbusinesslogic.h"
#include "shutdownbusinesslogic.h"
#include "statusarearenderer.h"
#include "notificationmanager.h"
#include "mcompositornotificationsink.h"
#include "ngfnotificationsink.h"
#include "contextframeworkcontext.h"
#include "unlocknotificationsink.h"
#include "volumecontrolui.h"

#define DEBUG
#define WARNING
#include "debug.h"

#include <QX11Info>
#include <X11/Xutil.h>

#define TRANSLATION_CATALOG "systemui"

Sysuid* Sysuid::m_Sysuid = NULL;

Sysuid::Sysuid (QObject* parent) : QObject (parent)
{
    SYS_DEBUG ("Starting sysuidaemon");

    m_Sysuid = this;

    // Load translation of System-UI
    retranslate ();

    m_ShutdownLogic   = new ShutdownBusinessLogic (this);
    m_BatteryLogic    = new BatteryBusinessLogic (this);
    m_UsbUi           = new UsbUi (this);

    m_notificationManager        = new NotificationManager (3000);
    m_compositorNotificationSink = new MCompositorNotificationSink;
    m_ngfNotificationSink        = new NGFNotificationSink;
    m_unlockNotificationSink     = new UnlockNotificationSink;

    // D-Bus registration and stuff
    new ShutdownBusinessLogicAdaptor (this, m_ShutdownLogic);

    QDBusConnection bus = QDBusConnection::sessionBus ();
    if (!bus.registerService (dbusService ())) {
        qCritical () << Q_FUNC_INFO << "failed to register dbus service";
        abort();
    }
    if (!bus.registerObject (dbusPath (), sysuid ())) {
        qCritical () << Q_FUNC_INFO << "failed to register dbus object";
        abort();
    }
    // Show status area when sysui daemon starts
    m_statusArea = new StatusAreaRenderer(this);

    // Connect to D-Bus and register the DBus source as an object
    bus.registerService("com.meego.core.MStatusBar");
    bus.registerObject("/statusbar", m_statusArea);

    connect (m_statusArea, SIGNAL (statusIndicatorMenuVisibilityChanged (bool)),
             m_compositorNotificationSink, SLOT (setDisabled (bool)));
    // Connect the notification signals for the compositor notification sink
    connect (m_notificationManager, SIGNAL (notificationUpdated (const Notification &)),
             m_compositorNotificationSink, SLOT (addNotification (const Notification &)));
    connect (m_notificationManager, SIGNAL (notificationRemoved (uint)),
             m_compositorNotificationSink, SLOT (removeNotification (uint)));
    connect (m_compositorNotificationSink, SIGNAL (notificationRemovalRequested (uint)),
             m_notificationManager, SLOT (removeNotification (uint)));

    // Connect the notification signals for the feedback notification sink
    connect (m_notificationManager, SIGNAL (notificationUpdated (const Notification &)),
             m_ngfNotificationSink, SLOT (addNotification (const Notification &)));
    connect (m_notificationManager, SIGNAL (notificationRemoved (uint)),
              m_ngfNotificationSink, SLOT (removeNotification (uint)));

    // Connect the notification signals for the unlock-screen notification sink
    connect (m_notificationManager, SIGNAL (notificationUpdated (const Notification &)),
             m_unlockNotificationSink, SLOT (addNotification (const Notification &)));
    connect (m_notificationManager, SIGNAL (notificationRemoved (uint)),
             m_unlockNotificationSink, SLOT (removeNotification (uint)));

    // Subscribe to a context property for getting information about the video recording status
    ContextFrameworkContext context;
    useMode = QSharedPointer<ContextItem> (context.createContextItem ("Use.Mode"));
    connect (useMode.data (), SIGNAL (contentsChanged ()),
             this, SLOT (applyUseMode ()));
    applyUseMode ();

    // Restore persistent notifications after all the signal connections are made to the notification sinks
    m_notificationManager->restorePersistentData ();

    /*
     * The screen locking is implemented in this separate class, because it is
     * bound to the system bus (MCE wants to contact us on the system bus).
     */
    m_sysuidRequest = new SysUidRequest;

    // Connect the unlock-screen notification sink to LockScreenBusinessLogic
    if (m_sysuidRequest->getLockScreenLogic () != 0)
    {
        connect (m_sysuidRequest->getLockScreenLogic (),
                 SIGNAL (screenIsLocked (bool)),
                 m_unlockNotificationSink,
                 SLOT (setLockedState (bool)));
        connect (m_sysuidRequest->getLockScreenLogic (),
                 SIGNAL (screenIsLocked (bool)),
                 m_compositorNotificationSink,
                 SLOT (setDisabled (bool)));
    }

    /*
     * Instantiate the volume-control UI
     */
    new VolumeControlUI (this);
}

Sysuid::~Sysuid ()
{
    m_Sysuid = NULL;
    delete m_sysuidRequest;
}

Sysuid* Sysuid::sysuid ()
{
    return m_Sysuid;
}

QString Sysuid::dbusService ()
{
    return QString ("com.nokia.systemui");
}

QString Sysuid::dbusPath ()
{
    return QString ("/");
}

/*!
 * Please note that in the libmeegotouch 0.19.4 manipulating theh MLocale in this
 * function might cause an endless recursion. I added a protection for brake the
 * recursion.
 */
void Sysuid::retranslate ()
{
    static bool      running = false;

    SYS_DEBUG ("*** running = %s", running ? "true" : "false");
    if (running)
        return;
    running = true;

    MLocale        locale;

    SYS_DEBUG (" lang = %s", SYS_STR (locale.language ()));

    // Install real translations
    locale.installTrCatalog ("usb");
    locale.installTrCatalog ("reset");
    locale.installTrCatalog ("energy");
    locale.installTrCatalog ("shutdown");
    locale.installTrCatalog ("profiles");
    locale.installTrCatalog ("screenlock");
    locale.installTrCatalog (TRANSLATION_CATALOG);

    MLocale::setDefault (locale);

    running = false;
}

NotificationManager &Sysuid::notificationManager ()
{
    return *m_notificationManager;
}

MCompositorNotificationSink& Sysuid::compositorNotificationSink ()
{
    return *m_compositorNotificationSink;
}

UnlockNotificationSink& Sysuid::unlockNotificationSink ()
{
    return *m_unlockNotificationSink;
}

void Sysuid::applyUseMode ()
{
    bool videoRecording =
        useMode->value ().toString () == "recording";

    m_compositorNotificationSink->setApplicationEventsEnabled (!videoRecording);
    m_ngfNotificationSink->setApplicationEventsEnabled (!videoRecording);
}

