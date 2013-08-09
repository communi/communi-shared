/*
* Copyright (C) 2008-2013 The Communi Project
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the <organization> nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
