/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (directui@nokia.com)
**
** This file is part of system ui.
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

#ifndef NOTIFICATIONAREASTYLE_H
#define NOTIFICATIONAREASTYLE_H

#include <DuiWidgetStyle>

class NotificationAreaStyle : public DuiWidgetStyle
{
    Q_OBJECT
    DUI_STYLE(NotificationAreaStyle)
};

class NotificationAreaStyleContainer : public DuiWidgetStyleContainer
{
    DUI_STYLE_CONTAINER(NotificationAreaStyle)
};

#endif
