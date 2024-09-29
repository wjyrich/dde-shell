// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "windowmanager.h"
#include <QDebug>

WindowManager::WindowManager(QObject *parent)
    : QAbstractListModel(parent)
{
}

int WindowManager::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_WindowList.size();
}

QVariant WindowManager::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_WindowList.size()) {
        return QVariant();
    }

    const AppRuntimeInfo &info = m_WindowList[index.row()];
    switch (role) {
    case NameRole:
        return info.name;
    case IdRole:
        return info.id;
    case StartTimeRole:
        return info.startTime.toMSecsSinceEpoch();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> WindowManager::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IdRole] = "id";
    roles[StartTimeRole] = "startTime";
    return roles;
}

void WindowManager::setWindowInfoForeground(const QString &name, uint id) {
    bool found = false;
    for (const auto &info : m_WindowList) {
        if (info.name == name) {
            found = true;
            break;
        }
    }
    if (!found) {
        AppRuntimeInfo info;
        info.name = "ForegroundApp："+name;
        info.id = id;
        info.startTime = QDateTime::currentDateTime();
        beginInsertRows(QModelIndex(), m_WindowList.size(), m_WindowList.size());
        m_WindowList.append(info);
        endInsertRows();
    }

}

void WindowManager::setWindowInfoBackground(const QString &name, uint id)
{
    bool found = false;
    for (const auto &info : m_WindowList) {
        if (info.name == name) {
            found = true;
            break;
        }
    }
    if (!found) {
        AppRuntimeInfo info;
        info.name = "BackgroundApp："+name;
        info.id = id;
        info.startTime = QDateTime::currentDateTime();
        beginInsertRows(QModelIndex(), m_WindowList.size(), m_WindowList.size());
        m_WindowList.append(info);
        endInsertRows();
    }
}

void WindowManager::WindowDetroyInfo(uint id)
{
    // Find the item to remove, matching only by id
    for (int i = 0; i < m_WindowList.size(); ++i) {
        if (m_WindowList[i].id == id) {
            // Start removing the row
            beginRemoveRows(QModelIndex(), i, i);
            m_WindowList.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}
