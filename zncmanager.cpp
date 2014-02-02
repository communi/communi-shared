/*
  Copyright (C) 2008-2014 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Jolla Ltd nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "zncmanager.h"
#include "ignoremanager.h"
#include <ircconnection.h>
#include <ircmessage.h>
#include <ircbuffer.h>

ZncManager::ZncManager(QObject* parent) : QObject(parent)
{
    d.model = 0;
    d.buffer = 0;
    d.playback = false;
    d.timestamp = QDateTime::fromTime_t(0);
    setModel(qobject_cast<IrcBufferModel*>(parent));
}

ZncManager::~ZncManager()
{
}

IrcBufferModel* ZncManager::model() const
{
    return d.model;
}

void ZncManager::setModel(IrcBufferModel* model)
{
    if (d.model != model) {
        if (d.model && d.model->connection()) {
            IrcConnection* connection = d.model->connection();
            disconnect(connection->network(), SIGNAL(requestingCapabilities()), this, SLOT(requestCapabilities()));
            connection->removeMessageFilter(this);
        }
        d.model = model;
        if (d.model && d.model->connection()) {
            IrcConnection* connection = d.model->connection();
            connect(connection->network(), SIGNAL(requestingCapabilities()), this, SLOT(requestCapabilities()));
            connection->installMessageFilter(this);
        }
        emit modelChanged(model);
    }
}

bool ZncManager::messageFilter(IrcMessage* message)
{
    if (message->tags().contains("time")) {
        d.timestamp = message->tags().value("time").toDateTime();
        if (d.timestamp.isValid())
            message->setTimeStamp(d.timestamp.toTimeSpec(Qt::LocalTime));
    }

    if (message->type() == IrcMessage::Private) {
        if (message->nick() == QLatin1String("***")) {
            IrcPrivateMessage* privMsg = static_cast<IrcPrivateMessage*>(message);
            QString content = privMsg->content();
            if (content == QLatin1String("Buffer Playback...")) {
                d.playback = true;
                d.buffer = d.model->find(privMsg->target());
                if (d.buffer)
                    emit playbackBegin(d.buffer);
                return false;
            } else if (content == QLatin1String("Playback Complete.")) {
                if (d.buffer)
                    emit playbackEnd(d.buffer);
                d.playback = false;
                d.buffer = 0;
                return false;
            }
        }
    } else if (message->type() == IrcMessage::Notice) {
        if (message->nick() == "*communi") {
            d.timestamp = QDateTime::fromTime_t(static_cast<IrcNoticeMessage*>(message)->content().toLong());
            return true;
        }
    }

    if (d.playback && d.buffer) {
        switch (message->type()) {
        case IrcMessage::Private:
            return processMessage(static_cast<IrcPrivateMessage*>(message));
        case IrcMessage::Notice:
            return processNotice(static_cast<IrcNoticeMessage*>(message));
        default:
            break;
        }
    }
    return false;
}

bool ZncManager::processMessage(IrcPrivateMessage* message)
{
    QString msg = message->content();
    if (message->nick() == "*buffextras") {
        int idx = msg.indexOf(" ");
        QString prefix = msg.left(idx);
        QString content = msg.mid(idx + 1);

        IrcMessage* tmp = 0;
        if (content.startsWith("joined")) {
            tmp = IrcMessage::fromParameters(prefix, "JOIN", QStringList() << message->target(), message->connection());
        } else if (content.startsWith("parted")) {
            QString reason = content.mid(content.indexOf("[") + 1);
            reason.chop(1);
            tmp = IrcMessage::fromParameters(prefix, "PART", QStringList() << message->target() << reason , message->connection());
        } else if (content.startsWith("quit")) {
            QString reason = content.mid(content.indexOf("[") + 1);
            reason.chop(1);
            tmp = IrcMessage::fromParameters(prefix, "QUIT", QStringList() << reason , message->connection());
        } else if (content.startsWith("is")) {
            QStringList tokens = content.split(" ", QString::SkipEmptyParts);
            tmp = IrcMessage::fromParameters(prefix, "NICK", QStringList() << tokens.last() , message->connection());
        } else if (content.startsWith("set")) {
            QStringList tokens = content.split(" ", QString::SkipEmptyParts);
            QString user = tokens.takeLast();
            QString mode = tokens.takeLast();
            tmp = IrcMessage::fromParameters(prefix, "MODE", QStringList() << message->target() << mode << user, message->connection());
        } else if (content.startsWith("changed")) {
            QString topic = content.mid(content.indexOf(":") + 2);
            tmp = IrcMessage::fromParameters(prefix, "TOPIC", QStringList() << message->target() << topic, message->connection());
        } else if (content.startsWith("kicked")) {
            QString reason = content.mid(content.indexOf("[") + 1);
            reason.chop(1);
            QStringList tokens = content.split(" ", QString::SkipEmptyParts);
            tmp = IrcMessage::fromParameters(prefix, "KICK", QStringList() << message->target() << tokens.value(1) << reason, message->connection());
        }
        if (tmp) {
            tmp->setTimeStamp(message->timeStamp());
            d.buffer->receiveMessage(tmp);
            tmp->deleteLater();
            return true;
        }
    }

    if (message->isAction())
        msg = QString("\1ACTION %1\1").arg(msg);
    else if (message->isRequest())
        msg = QString("\1%1\1").arg(msg);
    message->setParameters(QStringList() << message->target() << msg);

    return IgnoreManager::instance()->messageFilter(message);
}

bool ZncManager::processNotice(IrcNoticeMessage* message)
{
    QString msg = message->content();
    if (message->isReply())
        msg = QString("\1%1\1").arg(msg);
    message->setParameters(QStringList() << message->target() << msg);

    return IgnoreManager::instance()->messageFilter(message);
}

void ZncManager::requestCapabilities()
{
    QStringList request;
    QStringList available = d.model->network()->availableCapabilities();

    if (available.contains("communi"))
        request << "communi" << QString("communi/%1").arg(d.timestamp.toTime_t());

    if (available.contains("server-time"))
        request << "server-time";
    else if (available.contains("znc.in/server-time-iso"))
        request << "znc.in/server-time-iso";
    else if (available.contains("znc.in/server-time"))
        request << "znc.in/server-time";

    if (!request.isEmpty())
        d.model->network()->requestCapabilities(request);
}
