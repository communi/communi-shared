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

#include "messagehandler.h"
#include <IrcBufferModel>
#include <IrcConnection>
#include <IrcMessage>
#include <IrcChannel>
#include <IrcBuffer>

IRC_USE_NAMESPACE

MessageHandler::MessageHandler(QObject* parent) : QObject(parent)
{
    setModel(qobject_cast<IrcBufferModel*>(parent));
}

MessageHandler::~MessageHandler()
{
}

IrcBufferModel* MessageHandler::model() const
{
    return d.model;
}

void MessageHandler::setModel(IrcBufferModel* model)
{
    if (d.model != model) {
        if (d.model)
            disconnect(d.model, SIGNAL(messageIgnored(IrcMessage*)), this, SLOT(handleMessage(IrcMessage*)));
        d.model = model;
        if (model)
            connect(model, SIGNAL(messageIgnored(IrcMessage*)), this, SLOT(handleMessage(IrcMessage*)));
    }
}

IrcBuffer* MessageHandler::defaultBuffer() const
{
    return d.defaultBuffer;
}

void MessageHandler::setDefaultBuffer(IrcBuffer* buffer)
{
    d.defaultBuffer = buffer;
}

IrcBuffer* MessageHandler::currentBuffer() const
{
    return d.currentBuffer;
}

void MessageHandler::setCurrentBuffer(IrcBuffer* buffer)
{
    d.currentBuffer = buffer;
}

void MessageHandler::handleMessage(IrcMessage* message)
{
    switch (message->type()) {
        case IrcMessage::Motd:
            sendMessage(message, d.defaultBuffer);
            break;
        case IrcMessage::Numeric:
            if (static_cast<IrcNumericMessage*>(message)->code() < 300)
                sendMessage(message, d.defaultBuffer);
            else
                sendMessage(message, d.currentBuffer);
            break;
        default:
            sendMessage(message, d.currentBuffer);
            break;
    }
}

void MessageHandler::sendMessage(IrcMessage* message, IrcBuffer* buffer)
{
    if (!buffer)
        buffer = d.defaultBuffer;
    if (buffer)
        buffer->receiveMessage(message);
}

void MessageHandler::sendMessage(IrcMessage* message, const QString& buffer)
{
    sendMessage(message, d.model->find(buffer));
}
