/***************************************************************************
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

#include <QApplication>
#include <MMessageBox>
#include <memnotify/memory_notification.hpp>
#include "lowmemorynotifier.h"

LowMemoryNotifier::LowMemoryNotifier(QObject *parent) : QObject(parent),
    memoryNotification(MEMNOTIFY::MemoryNotification::defaultObject("sysuid")),
    messageBox(new MMessageBox)
{
    connect(&memoryNotification, SIGNAL(notified(const QString&, const bool)), this, SLOT(handleLowMemoryNotification(const QString&, const bool)));
    if (memoryNotification.valid() && memoryNotification.enable()) {
        memoryNotification.poll();
    }

    messageBox->setSystem(true);
    messageBox->setIconId("icon-m-bootloader-warning");

    connect(qApp, SIGNAL(localeSettingsChanged()), this, SLOT(retranslateUi()));
    retranslateUi();
}

LowMemoryNotifier::~LowMemoryNotifier()
{
}

void LowMemoryNotifier::handleLowMemoryNotification(const QString &, const bool state)
{
    if (state) {
        messageBox->appear(MSceneWindow::KeepWhenDone);
    } else {
        messageBox->disappear();
    }
}

void LowMemoryNotifier::retranslateUi()
{
    //% "Memory low"
    messageBox->setTitle(qtTrId("qtn_comm_memory_low"));
    //% "The system is running low on memory. Please close some running applications to free up resources."
    messageBox->setText(qtTrId("qtn_comm_memory_low_body"));
}