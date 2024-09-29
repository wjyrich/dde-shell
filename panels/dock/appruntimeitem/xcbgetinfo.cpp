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
    }
    return false;
}

XcbGetInfo::XcbGetInfo()
{
    xcbgetinfo = this;
    connect(this, &XcbGetInfo::eventFilterWindowCreated, this, &XcbGetInfo::handleCreateNotifyEvent);
    connect(this, &XcbGetInfo::eventFilterWindowDestroyed, this, &XcbGetInfo::handleDestroyNotifyEvent);
    connect(this, &XcbGetInfo::windowCreatedBackground, this, [this](xcb_window_t window, const QString name) {
        Q_EMIT windowInfoChangedBackground(name, window);
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

void XcbGetInfo::handleCreateNotifyEvent(xcb_create_notify_event_t *e)
{
    if (!e->override_redirect) {
        QString name;
        name = X11->getWindowName(e->window);
        if (!name.isEmpty()) {
            //Q_EMIT signal with window ID and name
            Q_EMIT windowCreated(e->window, name);
        }
    }
}

void XcbGetInfo::handleDestroyNotifyEvent(xcb_destroy_notify_event_t *e)
{
    Q_EMIT windowDestroyed(e->window);
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
}
