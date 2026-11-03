// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "taskoverflowcontroller.h"
#include "textcalculator.h"

#include <QAbstractItemModelTester>
#include <QKeyEvent>
#include <QLibrary>

#include <QStandardItemModel>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtTest>

namespace {
enum Role { Id = Qt::UserRole, Active, Windows, Docked, Icon, Title };

class AppModel : public QStandardItemModel
{
public:
    AppModel()
    {
        setItemRoleNames({{Id, "itemId"}, {Active, "active"}, {Windows, "windows"},
                          {Docked, "docked"}, {Icon, "iconName"}, {Title, "title"}});
    }

    mutable int roleQueries = 0;
    mutable int dataReads = 0;
    QHash<int, QByteArray> roleNames() const override {
        ++roleQueries;
        return QStandardItemModel::roleNames();
    }
    QVariant data(const QModelIndex &index, int role) const override {
        ++dataReads;
        return QStandardItemModel::data(index, role);
    }

    void add(const QString &id, bool running = true, bool docked = false)
    {
        auto item = new QStandardItem;
        item->setData(id, Id);
        item->setData(running ? QStringList{id + "-window"} : QStringList{}, Windows);
        item->setData(docked, Docked);
        appendRow(item);
    }

    void activate(int row)
    {
        for (int i = 0; i < rowCount(); ++i)
            setData(index(i, 0), i == row, Active);
    }
};
}

class TaskOverflowControllerTests : public QObject
{
    Q_OBJECT
private:
    static void configure(dock::TaskOverflowController &controller, AppModel &model, qreal available = 100)
    {
        controller.setDataModel(&model);
        controller.setFallbackItemExtent(24);
        controller.setSpacing(10);
        controller.setAvailableExtent(available);
        controller.setMinimumReached(true);
    }

    static QString recent(const dock::TaskOverflowController &controller)
    {
        return controller.recentItem().value("itemId").toString();
    }

private slots:
    void singleRowUpdatePreservesDelegates()
    {
        AppModel model;
        for (int i = 0; i < 5; ++i)
            model.add(QString::number(i));
        dock::TaskOverflowController controller;
        configure(controller, model);
        QTRY_COMPARE(controller.popupCount(), 4);
        QAbstractItemModelTester tester(controller.popupModel(), QAbstractItemModelTester::FailureReportingMode::QtTest);
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("overflow", &controller);
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            Item {
                id: root
                property int created: 0
                property int removed: 0
                Repeater {
                    model: overflow.popupModel
                    onItemAdded: root.created++
                    onItemRemoved: root.removed++
                    delegate: Item { required property var entry }
                }
            }
        )", QUrl());
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QCOMPARE(root->property("created").toInt(), 4);
        QSignalSpy updates(controller.popupModel(), &QAbstractItemModel::dataChanged);
        model.roleQueries = model.dataReads = 0;
        model.setData(model.index(2, 0), "new-icon", Icon);
        // An icon update must not synchronously scan application activity.
        QCOMPARE(model.dataReads, 0);
        QTRY_COMPARE(updates.count(), 1);
        QCOMPARE(model.roleQueries, 0);
        QVERIFY(model.dataReads < 10);
        QCOMPARE(root->property("created").toInt(), 4);
        QCOMPARE(root->property("removed").toInt(), 0);

        const QPersistentModelIndex unchanged(controller.popupModel()->index(2, 0));
        model.removeRow(2);
        QTRY_COMPARE(controller.popupCount(), 3);
        QVERIFY(unchanged.isValid());
        QCOMPARE(unchanged.data(Qt::UserRole).toMap().value("itemId").toString(), QString("3"));
    }

    void narrowLayoutKeepsShortcutsInsideBudget()
    {
        AppModel model;
        for (int i = 0; i < 5; ++i)
            model.add(QString::number(i));
        dock::TaskOverflowController controller;
        configure(controller, model, 114);
        controller.setOverflowSidePadding(11);
        QTRY_COMPARE(controller.visibleItemCount(), 1);
        QVERIFY(controller.showRecent());
        controller.setAvailableExtent(58);
        QTRY_VERIFY(!controller.showRecent());
        QCOMPARE(controller.trailingExtent(), 46);
        QCOMPARE(controller.visibleItemCount(), 0);
        for (qreal available : {23., 1., 0.}) {
            controller.setAvailableExtent(available);
            QTRY_COMPARE(controller.buttonExtent(), available);
            QVERIFY(controller.trailingExtent() <= available);
            QVERIFY(controller.buttonOffset() + controller.buttonExtent() <= available);
            QCOMPARE(controller.popupCount(), 5);
        }
        controller.setAvailableExtent(200);
        QTRY_COMPARE(controller.popupCount(), 0);
        QCOMPARE(controller.visibleItemCount(), 5);
        QCOMPARE(controller.trailingExtent(), 0);
    }

    void modelChangesWaitUntilDragFinishes()
    {
        AppModel model;
        for (int i = 0; i < 5; ++i)
            model.add(QString::number(i));
        dock::TaskOverflowController controller;
        configure(controller, model);
        QTRY_COMPARE(controller.popupCount(), 4);
        controller.beginDrag();
        model.add("new");
        QCoreApplication::processEvents();
        QCOMPARE(controller.popupCount(), 4);
        QVERIFY(!controller.finishDrag(Qt::MoveAction));
        QTRY_COMPARE(controller.popupCount(), 5);
        controller.beginDrag();
        QCoreApplication::processEvents();
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QCoreApplication::sendEvent(QCoreApplication::instance(), &escape);
        QVERIFY(!controller.finishDrag(Qt::IgnoreAction));
        QVERIFY(!controller.dragInProgress());
    }

    void totalWidthUpdatesWithoutChangingCharacterLimit()
    {
        AppModel model;
        model.add("a");
        model.setData(model.index(0, 0), "I", Title);
        dock::TextCalculator calculator;
        calculator.setDataModel(&model);
        calculator.setRemainingSpace(2000);
        calculator.setEnabled(true);
        calculator.componentComplete();
        QTRY_VERIFY(calculator.totalWidth() > 0);
        const auto previous = calculator.totalWidth();
        QSignalSpy widths(&calculator, &dock::TextCalculator::totalWidthChanged);
        QSignalSpy limits(&calculator, &dock::TextCalculator::optimalSingleTextWidthChanged);
        model.setData(model.index(0, 0), "WW", Title);
        model.setData(model.index(0, 0), "WWW", Title);
        QTRY_VERIFY(calculator.totalWidth() > previous);
        QCOMPARE(widths.count(), 1);
        QCOMPARE(limits.count(), 0);
    }

    void changedQmlComponentsCompile()
    {
        // Dock's QML plugin resolves the frame symbols exported by dde-shell.
        QLibrary frame(QStringLiteral(TEST_FRAME_LIBRARY));
        frame.setLoadHints(QLibrary::ExportExternalSymbolsHint);
        QVERIFY2(frame.load(), qPrintable(frame.errorString()));
        qmlRegisterType<dock::TaskOverflowController>("org.deepin.ds.dock.taskmanager", 1, 0, "TaskOverflowController");
        qmlRegisterType<dock::TextCalculator>("org.deepin.ds.dock.taskmanager", 1, 0, "TextCalculator");
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(TEST_BUILD_DIR "/plugins"));
        for (const auto &file : {"panels/dock/taskmanager/package/TaskManager.qml",
                                 "panels/dock/taskmanager/package/TaskOverflowButton.qml",
                                 "panels/dock/taskmanager/package/AppItem.qml",
                                 "panels/dock/package/main.qml", "frame/qml/PanelPopup.qml"}) {
            QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/") + file));
            QTRY_VERIFY(component.status() != QQmlComponent::Loading);
            QVERIFY2(component.isReady(), qPrintable(QString(file) + ": " + component.errorString()));
        }
    }

    void movingOverflowAppIntoTaskbarUpdatesPopupAndRecent()
    {
        AppModel model;
        for (int i = 0; i < 5; ++i)
            model.add(QString::number(i), false, true);
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("testApps", &model);
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQml
            import QtQml.Models
            DelegateModel {
                model: testApps
                delegate: QtObject {}
                function sourceIndexAt(visualIndex) {
                    return modelIndex(items.get(visualIndex).model.index)
                }
                function moveIntoTaskbar() { items.move(4, 0) }
            }
        )", QUrl());
        QScopedPointer<QObject> visualModel(component.create());
        QVERIFY2(visualModel, qPrintable(component.errorString()));

        dock::TaskOverflowController controller;
        configure(controller, model, 114);
        controller.setOverflowSidePadding(11);
        controller.setVisualModel(visualModel.get());
        QTRY_COMPARE(controller.visibleItemCount(), 1);
        QCOMPARE(recent(controller), QString("4"));

        QVERIFY(QMetaObject::invokeMethod(visualModel.get(), "moveIntoTaskbar"));
        controller.scheduleSync();
        QTRY_COMPARE(controller.popupItems().first().toMap().value("itemId").toString(), QString("0"));
        QCOMPARE(controller.popupItems().size(), 4);
        QCOMPARE(recent(controller), QString("3"));
        for (const auto &item : controller.popupItems())
            QVERIFY(item.toMap().value("itemId").toString() != "4");
        // The source model stays unchanged when DelegateModel reorders its items.
        QCOMPARE(model.data(model.index(0, 0), Id).toString(), QString("0"));
    }

    void reservesTwoSlotsAndRestoresFullLayout()
    {
        AppModel model;
        for (int i = 0; i < 5; ++i)
            model.add(QString::number(i));
        dock::TaskOverflowController controller;
        configure(controller, model);
        controller.setMinimumReached(false);
        QTRY_COMPARE(controller.visibleItemCount(), 5);
        controller.setMinimumReached(true);
        QTRY_COMPARE(controller.visibleItemCount(), 1);
        QCOMPARE(controller.popupItems().size(), 4);
        QCOMPARE(controller.popupItems().first().toMap().value("itemId").toString(), QString("1"));
        QCOMPARE(recent(controller), QString("4"));
        controller.setOverflowSidePadding(11);
        QTRY_COMPARE(controller.visibleItemCount(), 0);
        controller.setAvailableExtent(114);
        QTRY_COMPARE(controller.visibleItemCount(), 1);
        controller.setAvailableExtent(200);
        QTRY_VERIFY(controller.popupItems().isEmpty());
        QVERIFY(controller.recentItem().isEmpty());
        QCOMPARE(controller.visibleItemCount(), 5);
        controller.setAvailableExtent(58);
        QTRY_COMPARE(controller.visibleItemCount(), 0);
        QCOMPARE(controller.popupItems().size(), 5);
    }

    void defaultsToLastWithoutInventingActivationHistory()
    {
        AppModel model;
        model.add("a"); model.add("b"); model.add("c");
        model.activate(0);
        dock::TaskOverflowController controller;
        configure(controller, model, 58);
        QTRY_COMPARE(recent(controller), QString("c"));
        model.add("d", false, true);
        QTRY_COMPARE(recent(controller), QString("d"));
        model.setData(model.index(0, 0), "new-icon", Icon);
        QCoreApplication::processEvents();
        QCOMPARE(recent(controller), QString("d"));
    }

    void activationOrderAndNewPinnedApps()
    {
        AppModel model;
        model.add("a"); model.add("b"); model.add("c");
        dock::TaskOverflowController controller;
        configure(controller, model, 58);
        QTRY_COMPARE(recent(controller), QString("c"));
        model.activate(0);
        model.activate(1);
        model.activate(2);
        QTRY_COMPARE(recent(controller), QString("c"));
        model.add("pinned", false, true);
        QTRY_COMPARE(controller.popupItems().size(), 4);
        QCOMPARE(recent(controller), QString("c"));
        model.activate(0);
        QTRY_COMPARE(recent(controller), QString("a"));
        model.activate(-1);
        model.removeRow(0);
        QTRY_COMPARE(recent(controller), QString("c"));
        model.removeRow(1);
        QTRY_COMPARE(recent(controller), QString("b"));
    }

    void closingPinnedAppFallsBackToMinimizedHistory()
    {
        AppModel model;
        model.add("a", true, true); model.add("b", true, true); model.add("c", true, true);
        dock::TaskOverflowController controller;
        configure(controller, model, 58);
        QTRY_COMPARE(recent(controller), QString("c"));
        model.activate(0);
        model.activate(1);
        model.activate(-1);
        QTRY_COMPARE(recent(controller), QString("b"));
        model.setData(model.index(1, 0), QStringList{}, Windows);
        QTRY_COMPARE(recent(controller), QString("a"));
        model.setData(model.index(1, 0), QStringList{"reopened"}, Windows);
        QTRY_COMPARE(recent(controller), QString("b"));
    }

    void newlyRunningAppUpdatesExistingHistory()
    {
        AppModel model;
        model.add("a"); model.add("b"); model.add("pinned", false, true);
        dock::TaskOverflowController controller;
        configure(controller, model, 58);
        QTRY_COMPARE(recent(controller), QString("pinned"));
        model.activate(0);
        QTRY_COMPARE(recent(controller), QString("a"));
        model.add("new-app");
        QTRY_COMPARE(recent(controller), QString("new-app"));
    }

    void closingRecentPrefersStillActiveOverflowApp()
    {
        AppModel model;
        model.add("active"); model.add("b"); model.add("c");
        model.activate(0);
        dock::TaskOverflowController controller;
        configure(controller, model, 58);
        QTRY_COMPARE(recent(controller), QString("c"));
        model.add("background");
        QTRY_COMPARE(recent(controller), QString("background"));
        model.removeRow(3);
        QTRY_COMPARE(recent(controller), QString("active"));
    }

    void closedPinnedRecordsRemainAvailableAsFallback()
    {
        AppModel model;
        model.add("a", true, true); model.add("b", true, true); model.add("c", false, true);
        dock::TaskOverflowController controller;
        configure(controller, model, 58);
        QTRY_COMPARE(recent(controller), QString("c"));
        model.activate(0);
        model.activate(1);
        QTRY_COMPARE(recent(controller), QString("b"));
        model.setData(model.index(0, 0), QStringList{}, Windows);
        model.activate(-1);
        model.setData(model.index(1, 0), QStringList{}, Windows);
        QTRY_COMPARE(recent(controller), QString("a"));
        model.setData(model.index(2, 0), "unrelated", Icon);
        QCoreApplication::processEvents();
        QCOMPARE(recent(controller), QString("a"));
    }

    void layoutRemovesHistoryAndDoesNotCountAsUsage()
    {
        AppModel model;
        for (int i = 0; i < 6; ++i)
            model.add(QString::number(i));
        dock::TaskOverflowController controller;
        configure(controller, model, 58);
        QTRY_COMPARE(recent(controller), QString("5"));
        model.activate(0);
        QTRY_COMPARE(recent(controller), QString("0"));
        controller.setAvailableExtent(126);
        QTRY_COMPARE(controller.visibleItemCount(), 2);
        QCOMPARE(recent(controller), QString("5"));
        controller.setAvailableExtent(58);
        QTRY_COMPARE(controller.visibleItemCount(), 0);
        QCOMPARE(recent(controller), QString("5"));
    }

    void trimsTenOldestRecordsWhenExceedingThirty()
    {
        AppModel model;
        for (int i = 0; i < 31; ++i)
            model.add(QString::number(i));
        dock::TaskOverflowController controller;
        configure(controller, model, 58);
        QTRY_COMPARE(controller.popupItems().size(), 31);
        for (int i = 0; i < 31; ++i)
            model.activate(i);
        QTRY_COMPARE(recent(controller), QString("30"));
        model.add("fallback", false, true);
        QTRY_COMPARE(controller.popupItems().size(), 32);
        model.removeRows(10, 21);
        QTRY_COMPARE(recent(controller), QString("fallback"));
    }

    void modelDestructionClearsShortcut()
    {
        auto model = std::make_unique<AppModel>();
        model->add("a"); model->add("b"); model->add("c");
        dock::TaskOverflowController controller;
        configure(controller, *model, 58);
        QTRY_COMPARE(recent(controller), QString("c"));
        model.reset();
        QTRY_VERIFY(controller.recentItem().isEmpty());
        QVERIFY(controller.popupItems().isEmpty());
    }
};

QTEST_MAIN(TaskOverflowControllerTests)
#include "taskoverflowcontrollertests.moc"
