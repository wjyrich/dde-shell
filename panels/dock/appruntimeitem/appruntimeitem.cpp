// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appruntimeitem.h"
#include "applet.h"
#include "pluginfactory.h"

#include "../constants.h"

#include <DDBusSender>
#include <DDciIcon>
#include <DGuiApplicationHelper>

#include <QGuiApplication>
#include <QBuffer>

DGUI_USE_NAMESPACE

namespace dock {
    AppRuntimeItem::AppRuntimeItem(QObject *parent)
        : DApplet(parent)
        ,m_xcbGetInfo(nullptr)
        , m_engine(nullptr)
        , m_windowManager(nullptr)
        , m_Visible(true)
        , m_appRuntimeVisible(false){}

    void AppRuntimeItem::toggleruntimeitem()
    {
        if(!m_xcbGetInfo){
            m_xcbGetInfo  = new XcbGetInfo();
            m_xcbGetInfo->deploy();
            // Create an instance of QQmlApplicationEngine
            m_engine = new QQmlApplicationEngine();
            const QUrl url(QStringLiteral("qrc:/ddeshell/package/ShowRuntimeMenu.qml"));

            // Create an instance of WindowManager to manage window information
            m_windowManager = new WindowManager();

            // Register custom types
            qRegisterMetaType<xcb_window_t>("xcb_window_t");
            qRegisterMetaType<QVector<AppRuntimeInfo>>("QVector<AppRuntimeInfo>");
            qRegisterMetaType<AppRuntimeInfo>("AppRuntimeInfo");

            // Register windowManager to the QML context
            m_engine->rootContext()->setContextProperty("windowManager", m_windowManager);

            // Connect signals and slots
            connect(m_xcbGetInfo, &XcbGetInfo::windowInfoChanged,
                    m_windowManager, &WindowManager::setWindowInfoForeground,
                    Qt::QueuedConnection);
            connect(m_xcbGetInfo, &XcbGetInfo::windowInfoChangedForeground,
                    m_windowManager, &WindowManager::setWindowInfoForeground,
                    Qt::QueuedConnection);
            connect(m_xcbGetInfo, &XcbGetInfo::windowInfoChangedBackground,
                    m_windowManager, &WindowManager::setWindowInfoBackground,
                    Qt::QueuedConnection);
            connect(m_xcbGetInfo, &XcbGetInfo::windowDestroyChanged,
                    m_windowManager, &WindowManager::WindowDetroyInfo,
                    Qt::QueuedConnection);
            connect(m_xcbGetInfo, &XcbGetInfo::windowLeaveChangedInactiveName,
                    m_windowManager, &WindowManager::setWindowInfoInActive,
                    Qt::QueuedConnection);
            connect(m_xcbGetInfo, &XcbGetInfo::windowEnterChangedActiveName,
                    m_windowManager, &WindowManager::setWindowInfoActive,
                    Qt::QueuedConnection);
            m_xcbGetInfo->getAllWindows();
            // Load the QML file
            m_engine->load(url);
            if (m_engine->rootObjects().isEmpty()) {
                qFatal("Failed to load QML");
            }
            // Connect engine destroyed signal to clean up resources
            QObject::connect(m_engine, &QQmlApplicationEngine::destroyed, [this]() {
                m_windowManager->deleteLater();
                m_windowManager = nullptr;  // Clear pointer
                m_engine = nullptr;  // Clear pointer
            });
        }
        else{
            // If the window already exists, check if the window is visible
            QObject *rootObject = m_engine->rootObjects().first();
            QQuickWindow *window = qobject_cast<QQuickWindow *>(rootObject);
            if (window) {
                // If the window is not visible, show it
                if (!window->isVisible()) {
                    window->setVisible(true);
                } else {
                    // If the window is visible, hide it
                    window->setVisible(false);
                }
            }
        }
    }
    DockItemInfo AppRuntimeItem::dockItemInfo()
    {
        DockItemInfo info;
        info.name = "appruntime";
        info.displayName = tr("App_runtime");
        info.itemKey = "appruntime";
        info.settingKey = "appruntime";
        info.visible = m_Visible;
        info.dccIcon = DCCIconPath + "appruntime.svg";
        return info;
    }
    void AppRuntimeItem::setVisible(bool visible)
    {
        if (m_Visible != visible) {
            m_Visible = visible;

            Q_EMIT visibleChanged(visible);
        }
    }

    void AppRuntimeItem::onappruntimeVisibleChanged(bool visible)
    {
        if (m_appRuntimeVisible != visible) {
            m_appRuntimeVisible = visible;
            Q_EMIT appruntimeVisibleChanged(visible);
        }
    }

    D_APPLET_CLASS(AppRuntimeItem)
}

#include "appruntimeitem.moc"
