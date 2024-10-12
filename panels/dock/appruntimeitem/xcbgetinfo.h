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
#include "abstractwindow.h"
#include <mutex>
#include <sys/types.h>

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

class XcbGetInfo : public AbstractWindow
{
    Q_OBJECT
Q_SIGNALS:
    void windowCreated(xcb_window_t window, const QString name);
    void windowDestroyed(xcb_window_t window);
    void windowGetAll(xcb_window_t window, const QString name);

    void windowCreatedForeground(xcb_window_t window, const QString name);
    void windowCreatedBackground(xcb_window_t window, const QString name);
    void eventFilterWindowCreated(xcb_create_notify_event_t *);
    void eventFilterWindowDestroyed(xcb_destroy_notify_event_t *);
    void windowEnterChanged(xcb_window_t window);
    void windowLeaveChanged(xcb_window_t window);
    void windowPropertyChanged(xcb_window_t window,xcb_atom_t atom);

    void windowLeaveChangedInactive(xcb_window_t window,const QString name);
    void windowEnterChangedActive(xcb_window_t window,const QString name);

    void windowLeaveChangedInactiveName(xcb_window_t window,const QString &name);
    void windowEnterChangedActiveName(xcb_window_t window,const QString &name);

    void windowInfoChanged(const QString &name, uint id);
    void windowDestroyChanged(uint id);
    void windowInfoChangedForeground(const QString &name, uint id);
    void windowInfoChangedBackground(const QString &name, uint id);

public Q_SLOTS:
    void handleCreateNotifyEvent(xcb_create_notify_event_t *e);
    void handleDestroyNotifyEvent(xcb_destroy_notify_event_t *e);
    void handlePropertyNotifyEvent(xcb_window_t window,xcb_atom_t atom);

    void handleEnterEvent(xcb_window_t window);
    void handleLeaveEvent(xcb_window_t window);

public:
    void saveDestroyWindow(xcb_window_t window, char *name);
    XcbGetInfo();
    void printWindowInfo(xcb_window_t window_id);
    int deploy(void);
    void getChildren(xcb_connection_t* , xcb_window_t , xcb_window_t** , int* , xcb_get_geometry_reply_t*** );
    void getAllWindows();
    QString getName(xcb_window_t);

    virtual uint32_t id() override;
    virtual pid_t pid() override;
    virtual QString identity() override;
    virtual QString icon() override;
    virtual QString title() override;
    virtual bool isActive() override;
    virtual bool shouldSkip() override;
    virtual bool isMinimized() override;
    virtual bool allowClose() override;
    virtual bool isAttention() override;

    virtual void close() override;
    virtual void activate() override;
    virtual void maxmize() override;
    virtual void minimize() override;
    virtual void killClient() override;
    virtual void setWindowIconGeometry(const QWindow* baseWindow, const QRect& gemeotry) override;

private:
    QScopedPointer<XcbEventFilter> m_xcbEventGet;
    xcb_window_t m_windowID;
    QList<xcb_atom_t> m_windowStates;
    std::once_flag m_windowStateFlag;


};

}
