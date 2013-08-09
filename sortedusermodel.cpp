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

#include "sortedusermodel.h"
#include <IrcSessionInfo>
#include <IrcBufferModel>
#include <IrcUserModel>
#include <IrcSession>
#include <IrcChannel>

SortedUserModel::SortedUserModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSourceModel(new IrcUserModel(this));
    setChannel(qobject_cast<IrcChannel*>(parent));
}

IrcChannel* SortedUserModel::channel() const
{
    return static_cast<IrcUserModel*>(sourceModel())->channel();
}

void SortedUserModel::setChannel(IrcChannel* channel)
{
    static_cast<IrcUserModel*>(sourceModel())->setChannel(channel);

    if (channel) {
        m_prefixes = IrcSessionInfo(channel->session()).prefixes();
        sort(0, Qt::AscendingOrder);
    }
}

bool SortedUserModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const QString p1 = left.data(Irc::PrefixRole).toString();
    const QString p2 = right.data(Irc::PrefixRole).toString();

    const int i1 = !p1.isEmpty() ? m_prefixes.indexOf(p1.at(0)) : -1;
    const int i2 = !p2.isEmpty() ? m_prefixes.indexOf(p2.at(0)) : -1;

    if (i1 >= 0 && i2 < 0)
        return true;
    if (i1 < 0 && i2 >= 0)
        return false;
    if (i1 >= 0 && i2 >= 0 && i1 != i2)
        return i1 < i2;

    const QString n1 = left.data(Irc::NameRole).toString();
    const QString n2 = right.data(Irc::NameRole).toString();
    return n1.compare(n2, Qt::CaseInsensitive) < 0;
}
