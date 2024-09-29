// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "qobjectdefs.h"
#include <xcb/xcb.h>
#include <unistd.h>
#include <err.h>
#include <QObject>
#include "x11utils.h"

#include <QAbstractNativeEventFilter>

typedef struct {
    xcb_window_t window;
    char *name;
    time_t createTime;
} WindowInfo;
Q_DECLARE_METATYPE(WindowInfo)

enum { INACTIVE, ACTIVE };

/* global variables */
static xcb_connection_t		*m_conn;
static xcb_window_t		 m_focuswin;

namespace dock {
class XcbEventFilter: public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *) override;
};

class XcbGetInfo : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void windowCreated(xcb_window_t window, const QString name);
    void windowDestroyed(xcb_window_t window);
    void windowCreatedForeground(xcb_window_t window, const QString name);
    void windowCreatedBackground(xcb_window_t window, const QString name);
    void eventFilterWindowCreated(xcb_create_notify_event_t *);
    void eventFilterWindowDestroyed(xcb_destroy_notify_event_t *);

    void windowInfoChanged(const QString &name, uint id);
    void windowDestroyChanged(uint id);
    void windowInfoChangedForeground(const QString &name, uint id);
    void windowInfoChangedBackground(const QString &name, uint id);

public Q_SLOTS:
    void handleCreateNotifyEvent(xcb_create_notify_event_t *e);
    void handleDestroyNotifyEvent(xcb_destroy_notify_event_t *e);
public:
    void saveDestroyWindow(xcb_window_t window, char *name);
    XcbGetInfo();
    void printWindowInfo(xcb_window_t window_id);
    int deploy(void);
    void getChildren(xcb_connection_t* , xcb_window_t , xcb_window_t** , int* , xcb_get_geometry_reply_t*** );
    void getAllWindows();
    QString getName(xcb_window_t);
private:
    QScopedPointer<XcbEventFilter> m_xcbEventGet;

};

}
