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
#include <IrcMessage>
#include <IrcChannel>
#include <IrcUser>

UserActivityModel::UserActivityModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    d.counter = 0;
    d.userModel = new IrcUserModel(this);

    setSourceModel(d.userModel);
    connect(d.userModel, SIGNAL(removed(IrcUser*)), this, SLOT(onUserRemoved(IrcUser*)));
    setChannel(qobject_cast<IrcChannel*>(parent));
    sort(0, Qt::DescendingOrder);
}

IrcChannel* UserActivityModel::channel() const
{
    return d.userModel->channel();
}

void UserActivityModel::setChannel(IrcChannel* channel)
{
    if (channel != d.userModel->channel()) {
        if (d.userModel->channel())
            disconnect(d.userModel->channel(), SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));

        d.users.clear();
        d.userModel->setChannel(channel);

        if (channel)
            connect(channel, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));
    }
}

bool UserActivityModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const IrcUser* u1 = left.data(Irc::UserRole).value<IrcUser*>();
    const IrcUser* u2 = right.data(Irc::UserRole).value<IrcUser*>();
    return d.users.value(u1) < d.users.value(u2);
}

void UserActivityModel::onUserRemoved(IrcUser* user)
{
    d.users.remove(user);
}

void UserActivityModel::onMessageReceived(IrcMessage* message)
{
    QString name = message->sender().name();
    if (message->type() == IrcMessage::Nick)
        name = static_cast<IrcNickMessage*>(message)->nick();

    if (IrcUser* user = d.userModel->user(name)) {
        d.users[user] = ++d.counter;
        invalidate();
    }
}
