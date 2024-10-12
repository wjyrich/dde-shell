// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xcbgetinfo.h"
#include <QDebug>
#include <QPointer>
#include <QCoreApplication>

#define X11 X11Utils::instance()

namespace dock {
static QPointer<XcbGetInfo> xcbgetinfo;

bool XcbEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *)
{
    if (eventType != "xcb_generic_event_t")
        return false;

    auto xcb_event = reinterpret_cast<xcb_generic_event_t*>(message);
    switch (xcb_event->response_type) {
    case XCB_PROPERTY_NOTIFY: {
        auto pE = reinterpret_cast<xcb_property_notify_event_t*>(xcb_event);
        Q_EMIT xcbgetinfo->windowPropertyChanged(pE->window, pE->atom);
        break;
    }
    case XCB_CREATE_NOTIFY: {
        auto e = reinterpret_cast<xcb_create_notify_event_t*>(xcb_event);
        Q_EMIT xcbgetinfo->eventFilterWindowCreated(e);
        break;
    }
    case XCB_DESTROY_NOTIFY: {
        auto e = reinterpret_cast<xcb_destroy_notify_event_t*>(xcb_event);
        Q_EMIT xcbgetinfo->eventFilterWindowDestroyed(e);
    }
    case XCB_ENTER_NOTIFY: {
        auto eN = reinterpret_cast<xcb_enter_notify_event_t *>(xcb_event);
        Q_EMIT xcbgetinfo->windowEnterChanged(eN->event);
        break;
    }
    case XCB_LEAVE_NOTIFY: {
        auto lN = reinterpret_cast<xcb_leave_notify_event_t *>(xcb_event);
        Q_EMIT xcbgetinfo->windowLeaveChanged(lN->event);
        break;
    }
    }
    return false;
}

XcbGetInfo::XcbGetInfo()
{
    xcbgetinfo = this;
    connect(this, &XcbGetInfo::windowEnterChanged, this, &XcbGetInfo::handleEnterEvent);
    connect(this, &XcbGetInfo::windowLeaveChanged, this, &XcbGetInfo::handleLeaveEvent);
    connect(this, &XcbGetInfo::windowPropertyChanged, this, &XcbGetInfo::handlePropertyNotifyEvent);
    connect(this, &XcbGetInfo::windowPropertyChanged, this, &XcbGetInfo::handlePropertyNotifyEvent);

    connect(this, &XcbGetInfo::eventFilterWindowCreated, this, &XcbGetInfo::handleCreateNotifyEvent);
    connect(this, &XcbGetInfo::eventFilterWindowDestroyed, this, &XcbGetInfo::handleDestroyNotifyEvent);
    connect(this, &XcbGetInfo::windowEnterChangedActive, this, [this](xcb_window_t window, const QString name) {
        Q_EMIT windowLeaveChangedInactiveName(window, name);
    });
    connect(this, &XcbGetInfo::windowEnterChangedActive, this, [this](xcb_window_t window, const QString name) {
        Q_EMIT windowEnterChangedActiveName(window, name);
    });

    connect(this, &XcbGetInfo::windowCreatedForeground, this, [this](xcb_window_t window, const QString name) {
        Q_EMIT windowInfoChangedForeground(name, window);
    });
    connect(this, &XcbGetInfo::windowCreated, this, [this](xcb_window_t window, const QString name) {
        Q_EMIT windowInfoChanged(name, window);
    });
    connect(this, &XcbGetInfo::windowDestroyed, this, [this](xcb_window_t window) {
        Q_EMIT windowDestroyChanged(window);
    });
}

int XcbGetInfo::deploy()
{
    /* init xcb and grab events */
    m_conn = X11->getXcbConnection();
    m_focuswin = X11->getRootWindow();
    uint32_t value_list[] = {
        0                               | XCB_EVENT_MASK_PROPERTY_CHANGE        |
        XCB_EVENT_MASK_VISIBILITY_CHANGE    | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY    |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY     | XCB_EVENT_MASK_FOCUS_CHANGE
    };
    xcb_change_window_attributes(m_conn, X11->getRootWindow(), XCB_CW_EVENT_MASK, value_list);
    xcb_flush(m_conn);
    m_xcbEventGet.reset(new XcbEventFilter());
    qApp->installNativeEventFilter(m_xcbEventGet.get());
    return 0;
}

void XcbGetInfo::handleEnterEvent(xcb_window_t window)
{
    QString m_name = getName(window);
    Q_EMIT windowEnterChangedActive(window,m_name);
}

void XcbGetInfo::handleLeaveEvent(xcb_window_t window)
{
    QString m_name = getName(window);
    Q_EMIT windowLeaveChangedInactive(window,m_name);

}

void XcbGetInfo::handleCreateNotifyEvent(xcb_create_notify_event_t *e)
{
    if (!e->override_redirect) {
        QString name;
        name = X11->getWindowName(e->window);
        if (!name.isEmpty()) {
            Q_EMIT windowCreated(e->window, name);
        }
    }
}

void XcbGetInfo::handleDestroyNotifyEvent(xcb_destroy_notify_event_t *e)
{
    Q_EMIT windowDestroyed(e->window);
}

void XcbGetInfo::handlePropertyNotifyEvent(xcb_window_t window, xcb_atom_t atom)
{
}

void XcbGetInfo::getChildren(xcb_connection_t* c, xcb_window_t window, xcb_window_t** children, int* count, xcb_get_geometry_reply_t*** geoms)
{
    *count = 0;
    *children = NULL;

    // Query tree to get children windows
    xcb_query_tree_cookie_t cookie = xcb_query_tree(c, window);
    xcb_query_tree_reply_t *reply = xcb_query_tree_reply(c, cookie, NULL);

    *count = xcb_query_tree_children_length(reply);
    *children = xcb_query_tree_children(reply);

    // Allocate memory for geometry replies
    *geoms = (xcb_get_geometry_reply_t **)malloc(*count * sizeof(xcb_get_geometry_reply_t *));

    // Get geometry for each child window
    for (int i = 0; i < *count; i++) {
        (*geoms)[i] = xcb_get_geometry_reply(c, xcb_get_geometry(c, (*children)[i]), NULL);
    }

    free(reply);
}

void XcbGetInfo::getAllWindows(){
    // Get children windows and their geometries
    const xcb_setup_t *setup = xcb_get_setup(m_conn);
    xcb_screen_t *screen = xcb_setup_roots_iterator(setup).data;
    xcb_window_t rootWindow = screen->root;
    int nChildren;
    xcb_window_t *children;
    xcb_get_geometry_reply_t **geoms;
    getChildren(m_conn, rootWindow, &children, &nChildren, &geoms);

    for (int i = 0; i < nChildren; i++) {
        xcb_window_t wid = children[i];
        xcb_get_geometry_reply_t *geom = geoms[i];

        QString name = getName(wid);

        if(!name.isEmpty()){
            if(geom->width>10||geom->height>10){
                Q_EMIT windowCreatedForeground(wid,name);
            }
            else{
                Q_EMIT windowCreatedBackground(wid,name);
            }
            free(geom);
        }
    }
    // Cleanup
    free(geoms);
}

QString XcbGetInfo::getName(xcb_window_t window)
{
    QString m_windowName = X11->getWindowName(window);
    return m_windowName;
}

uint32_t XcbGetInfo::id()
{
    return m_windowID;
}

pid_t XcbGetInfo::pid()
{
    pid_t m_pid = X11->getWindowPid(m_windowID);
    return m_pid;
}

QString XcbGetInfo::identity()
{
    QString m_identity = "";
    return m_identity;
}

QString XcbGetInfo::icon()
{
    QString m_icon = X11->getWindowIcon(m_windowID);
    return m_icon;
}

QString XcbGetInfo::title()
{
    QString m_title = X11->getWindowName(m_windowID);
    return m_title;
}

bool XcbGetInfo::isActive()
{
    std::call_once(m_windowStateFlag, [this](){
        m_windowStates.clear();
        m_windowStates = X11->getWindowState(m_windowID);
        Q_EMIT stateChanged();
    });
    return m_windowStates.contains(X11->getAtomByName("_NET_WM_STATE_FOCUSED"));
}

bool XcbGetInfo::shouldSkip()
{
    return true;
}

bool XcbGetInfo::isMinimized()
{
    std::call_once(m_windowStateFlag, [this](){
        m_windowStates.clear();
        m_windowStates = X11->getWindowState(m_windowID);
        Q_EMIT stateChanged();
    });
    return m_windowStates.contains(X11->getAtomByName("_NET_WM_STATE_HIDDEN"));
}

bool XcbGetInfo::allowClose()
{
    return true;
}

bool XcbGetInfo::isAttention()
{
    return m_windowStates.contains(X11->getAtomByName("_NET_WM_STATE_DEMANDS_ATTENTION"));
}

void XcbGetInfo::close()
{
    X11->closeWindow(m_windowID);
}

void XcbGetInfo::activate()
{
    X11->setActiveWindow(m_windowID);
}

void XcbGetInfo::maxmize()
{
    X11->maxmizeWindow(m_windowID);
}

void XcbGetInfo::minimize()
{
    X11->minimizeWindow(m_windowID);
}

void XcbGetInfo::killClient()
{
    X11->killClient(m_windowID);
}

void XcbGetInfo::setWindowIconGeometry(const QWindow *baseWindow, const QRect &gemeotry)
{
    auto pos = baseWindow->position() + gemeotry.topLeft();
    X11->setWindowIconGemeotry(m_windowID, QRect(pos.x(), pos.y(), gemeotry.width(), gemeotry.height()));
}
}
