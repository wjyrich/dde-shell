#include "appruntimeitem.h"
#include "applet.h"
#include "pluginfactory.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThread>
#include <QObject>
#include <QDebug>
#include <QQmlContext>


#include "windowmanager.h"
#include "xcbthread.h"

DS_BEGIN_NAMESPACE
namespace dock {

AppRuntimeItem::AppRuntimeItem(QObject *parent)
    : DApplet{parent}
{

}

void AppRuntimeItem::toggleruntimeitem()
{
    // 创建并启动 XCB 事件处理线程
        XcbThread xcbThread;
        qRegisterMetaType<xcb_window_t>("xcb_window_t");
        xcbThread.start();
        QQmlApplicationEngine engine;
        const QUrl url(QStringLiteral("qrc:/ddeshell/package/appruntimeitem_test.qml"));
        // 创建 WindowManager 实例，用于管理窗口信息
            WindowManager windowManager;
            // 将 windowManager 注册到 QML 上下文
            // 注册自定义类型
            qRegisterMetaType<QVector<WindowInfo_1>>("QVector<WindowInfo_1>");
            qRegisterMetaType<WindowInfo_1>("WindowInfo_1");
            engine.rootContext()->setContextProperty("windowManager", &windowManager);
            // 根据新创建的窗口进行更新为前台应用
            QObject::connect(&xcbThread, &XcbThread::windowInfoChanged,
                             &windowManager, &WindowManager::setWindowInfo_qiantai,
                             Qt::QueuedConnection);
            // 更新前台应用
            QObject::connect(&xcbThread, &XcbThread::windowInfoChanged_qiantai,
                             &windowManager, &WindowManager::setWindowInfo_qiantai,
                             Qt::QueuedConnection);
            // 更新后台应用
            QObject::connect(&xcbThread, &XcbThread::windowInfoChanged_houtai,
                             &windowManager, &WindowManager::setWindowInfo_houtai,
                             Qt::QueuedConnection);
            // 使用 XCB 线程中的信号来更新 WindowManager 中的已经被摧毁窗口信息
            QObject::connect(&xcbThread, &XcbThread::windowDestroyChanged,
                             &windowManager, &WindowManager::WindowDetroyInfo,
                             Qt::QueuedConnection);
            // 加载 QML 文件
            engine.load(url);
            if (engine.rootObjects().isEmpty()) {
                   qFatal("Failed to load QML");
            }
            // 等待 XCB 线程结束
            xcbThread.quit();
            xcbThread.wait();
}

D_APPLET_CLASS(AppRuntimeItem)
}

DS_END_NAMESPACE

#include "appruntimeitem.moc"
