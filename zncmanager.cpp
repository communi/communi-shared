/*
  Copyright (C) 2008-2015 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

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
#include <ircbuffermodel.h>
#include <ircconnection.h>
#include <irccommand.h>
#include <ircmessage.h>
#include <ircbuffer.h>

IRC_USE_NAMESPACE

ZncManager::ZncManager(QObject* parent) : QObject(parent)
{
    d.model = 0;
    d.buffer = 0;
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
            disconnect(connection, SIGNAL(connected()), this, SLOT(requestPlayback()));
            connection->removeMessageFilter(this);
            disconnect(model, SIGNAL(removed(IrcBuffer*)), this, SLOT(clearBuffer(IrcBuffer*)));
        }
        d.model = model;
        if (d.model && d.model->connection()) {
            IrcNetwork* network = d.model->network();
            QStringList caps = network->requestedCapabilities();
            caps += "echo-message";
            caps += "server-time";
            caps += "znc.in/playback";
            caps += "znc.in/server-time";
            caps += "znc.in/echo-message";
            caps += "znc.in/server-time-iso";
            network->setRequestedCapabilities(caps);

            IrcConnection* connection = d.model->connection();
            connect(connection, SIGNAL(connected()), this, SLOT(requestPlayback()));
            connection->installMessageFilter(this);
            connect(model, SIGNAL(removed(IrcBuffer*)), this, SLOT(clearBuffer(IrcBuffer*)));
        }
        emit modelChanged(model);
    }
}

bool ZncManager::messageFilter(IrcMessage* message)
{
    bool playback = false;
    if (message->connection()->isConnected() && message->tags().contains("time")) {
        QDateTime timestamp = message->tags().value("time").toDateTime();
        if (timestamp.isValid()) {
            message->setTimeStamp(timestamp.toTimeSpec(Qt::LocalTime));
            playback = timestamp < d.timestamp;
            d.timestamp = qMax(timestamp, d.timestamp);
        }
    }

    if (!playback && d.buffer) {
        emit playbackEnd(d.buffer);
        d.buffer = 0;
    }

    if (message->type() == IrcMessage::Private) {
        IrcPrivateMessage* msg = static_cast<IrcPrivateMessage*>(message);
        IrcBuffer* buffer = d.model->find(msg->target());
        if (buffer) {
            if (d.buffer != buffer) {
                if (d.buffer) {
                    emit playbackEnd(d.buffer);
                    d.buffer = 0;
                }
                if (playback) {
                    emit playbackBegin(buffer);
                    d.buffer = buffer;
                }
            }
            return processMessage(buffer, msg);
        }
    }
    return IgnoreManager::instance()->messageFilter(message);
}

bool ZncManager::processMessage(IrcBuffer* buffer, IrcPrivateMessage* message)
{
    if (message->nick() == "*buffextras") {
        const QString msg = message->content();
        const int idx = msg.indexOf(" ");
        const QString prefix = msg.left(idx);
        const QString content = msg.mid(idx + 1);

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
            const QStringList tokens = content.split(" ", QString::SkipEmptyParts);
            tmp = IrcMessage::fromParameters(prefix, "NICK", QStringList() << tokens.last() , message->connection());
        } else if (content.startsWith("set")) {
            QStringList tokens = content.split(" ", QString::SkipEmptyParts);
            const QString user = tokens.takeLast();
            const QString mode = tokens.takeLast();
            tmp = IrcMessage::fromParameters(prefix, "MODE", QStringList() << message->target() << mode << user, message->connection());
        } else if (content.startsWith("changed")) {
            const QString topic = content.mid(content.indexOf(":") + 2);
            tmp = IrcMessage::fromParameters(prefix, "TOPIC", QStringList() << message->target() << topic, message->connection());
        } else if (content.startsWith("kicked")) {
            QString reason = content.mid(content.indexOf("[") + 1);
            reason.chop(1);
            QStringList tokens = content.split(" ", QString::SkipEmptyParts);
            tmp = IrcMessage::fromParameters(prefix, "KICK", QStringList() << message->target() << tokens.value(1) << reason, message->connection());
        }
        if (tmp) {
            tmp->setTags(message->tags());
            tmp->setTimeStamp(message->timeStamp());
            buffer->receiveMessage(tmp);
            tmp->deleteLater();
            return true;
        }
    }
    return IgnoreManager::instance()->messageFilter(message);
}

void ZncManager::requestPlayback()
{
    if (d.model->network()->isCapable("znc.in/playback")) {
        IrcConnection* connection = d.model->connection();
        IrcCommand* cmd = IrcCommand::createMessage("*playback", QString("PLAY * %1").arg(d.timestamp.isValid() ? d.timestamp.toTime_t() : 0));
        cmd->setParent(this);
        connection->sendCommand(cmd);
    }
}

void ZncManager::clearBuffer(IrcBuffer* buffer)
{
    if (d.model->network()->isCapable("znc.in/playback") && !buffer->title().contains("*"))
        buffer->sendCommand(IrcCommand::createMessage("*playback", QString("CLEAR %1").arg(buffer->title())));
}
