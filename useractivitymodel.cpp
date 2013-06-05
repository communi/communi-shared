/*
* Copyright (C) 2008-2013 The Communi Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "useractivitymodel.h"
#include <IrcUserModel>
#include <IrcChannel>
#include <IrcUser>

UserActivityModel::UserActivityModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    IrcUserModel* userModel = new IrcUserModel(this);
    connect(userModel, SIGNAL(userAdded(IrcUser*)), this, SLOT(onUserAdded(IrcUser*)));
    connect(userModel, SIGNAL(userRemoved(IrcUser*)), this, SLOT(onUserRemoved(IrcUser*)));
    setSourceModel(userModel);

    setChannel(qobject_cast<IrcChannel*>(parent));
    sort(0, Qt::DescendingOrder);
}

IrcChannel* UserActivityModel::channel() const
{
    return static_cast<IrcUserModel*>(sourceModel())->channel();
}

void UserActivityModel::setChannel(IrcChannel* channel)
{
    IrcUserModel* userModel = static_cast<IrcUserModel*>(sourceModel());

    foreach (IrcUser* user, userModel->users())
        disconnect(user, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onUserMessageReceived(IrcMessage*)));

    userModel->setChannel(channel);

    foreach (IrcUser* user, userModel->users())
        connect(user, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onUserMessageReceived(IrcMessage*)));

    m_users.clear();
    invalidate();
}

bool UserActivityModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const IrcUser* u1 = left.data(Irc::UserRole).value<IrcUser*>();
    const IrcUser* u2 = right.data(Irc::UserRole).value<IrcUser*>();
    return m_users.value(u1) < m_users.value(u2);
}

void UserActivityModel::onUserAdded(IrcUser* user)
{
    connect(user, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onUserMessageReceived(IrcMessage*)));
    m_users.remove(user);
}

void UserActivityModel::onUserRemoved(IrcUser* user)
{
    disconnect(user, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onUserMessageReceived(IrcMessage*)));
    m_users.remove(user);
}

void UserActivityModel::onUserMessageReceived(IrcMessage* message)
{
    Q_UNUSED(message);
    if (IrcUser* user = qobject_cast<IrcUser*>(sender())) {
        ++m_users[user];
        invalidate();
    }
}
