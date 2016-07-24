/*
  Copyright (C) 2008-2016 The Communi Project

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
            caps += "batch";
            caps += "server-time";
            caps += "echo-message";
            caps += "znc.in/batch";
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
    IrcConnection* connection = message->connection();
    if (connection->isConnected())
        d.timestamp = qMax(d.timestamp, message->timeStamp());

    if (message->type() == IrcMessage::Batch) {
        IrcBatchMessage* batch = static_cast<IrcBatchMessage*>(message);
        if (batch->batch() == "znc.in/playback") {
            IrcBuffer* buffer = d.model->add(batch->parameters().value(2));
            foreach (IrcMessage* msg, batch->messages()) {
                msg->setFlags(msg->flags() | IrcMessage::Playback);
                if (msg->type() == IrcMessage::Private)
                    processMessage(static_cast<IrcPrivateMessage*>(msg));
            }
            buffer->receiveMessage(batch);
            return true;
        }
    }

    return IgnoreManager::instance()->messageFilter(message);
}

void ZncManager::processMessage(IrcPrivateMessage* message)
{
    if (message->nick() == "*buffextras") {
        const QString msg = message->content();
        const int idx = msg.indexOf(" ");
        const QString prefix = msg.left(idx);
        const QString content = msg.mid(idx + 1);

        message->setPrefix(prefix);
        if (content.startsWith("joined")) {
            message->setTag("intent", "JOIN");
            message->setParameters(QStringList() << message->target());
        } else if (content.startsWith("parted")) {
            message->setTag("intent", "PART");
            QString reason = content.mid(content.indexOf("[") + 1);
            reason.chop(1);
            message->setParameters(QStringList() << message->target() << reason);
        } else if (content.startsWith("quit")) {
            message->setTag("intent", "QUIT");
            QString reason = content.mid(content.indexOf("[") + 1);
            reason.chop(1);
            message->setParameters(QStringList() << reason);
        } else if (content.startsWith("is")) {
            message->setTag("intent", "NICK");
            const QStringList tokens = content.split(" ", QString::SkipEmptyParts);
            message->setParameters(QStringList() << tokens.last());
        } else if (content.startsWith("set")) {
            message->setTag("intent", "MODE");
            QStringList tokens = content.split(" ", QString::SkipEmptyParts);
            const QString user = tokens.takeLast();
            const QString mode = tokens.takeLast();
            message->setParameters(QStringList() << message->target() << mode << user);
        } else if (content.startsWith("changed")) {
            message->setTag("intent", "TOPIC");
            const QString topic = content.mid(content.indexOf(":") + 2);
            message->setParameters(QStringList() << message->target() << topic);
        } else if (content.startsWith("kicked")) {
            message->setTag("intent", "KICK");
            QString reason = content.mid(content.indexOf("[") + 1);
            reason.chop(1);
            QStringList tokens = content.split(" ", QString::SkipEmptyParts);
            message->setParameters(QStringList() << message->target() << tokens.value(1) << reason);
        }
    }
}

void ZncManager::requestPlayback()
{
    if (d.model->network()->isCapable("znc.in/playback")) {
        IrcConnection* connection = d.model->connection();
        connection->sendRaw(QString("ZNC *playback PLAY * %1").arg(d.timestamp.isValid() ? d.timestamp.toTime_t() : 0));
    }
}

void ZncManager::clearBuffer(IrcBuffer* buffer)
{
    if (d.model->network()->isCapable("znc.in/playback") && !buffer->title().contains("*")) {
        IrcConnection* connection = d.model->connection();
        connection->sendRaw(QString("ZNC *playback CLEAR %1").arg(buffer->title()));
    }
}
