/*
* Copyright (C) 2008-2014 The Communi Project
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

#include "messagehandler.h"
#include <IrcBufferModel>
#include <IrcConnection>
#include <IrcMessage>
#include <IrcChannel>
#include <IrcBuffer>

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
        case IrcMessage::Invite:
            handleInviteMessage(static_cast<IrcInviteMessage*>(message));
            break;
        case IrcMessage::Notice:
            handleNoticeMessage(static_cast<IrcNoticeMessage*>(message));
            break;
        case IrcMessage::Numeric:
            handleNumericMessage(static_cast<IrcNumericMessage*>(message));
            break;
        case IrcMessage::Unknown:
            handleUnknownMessage(static_cast<IrcMessage*>(message));
            break;
        default:
            break;
    }
}

void MessageHandler::handleInviteMessage(IrcInviteMessage* message)
{
    sendMessage(message, d.currentBuffer);
}

void MessageHandler::handleNoticeMessage(IrcNoticeMessage* message)
{
    QString target = message->target();

    // forward ChanServ's "[#chan] msg" to the appropriate channel
    if (target == message->connection()->nickName() && message->nick() == "ChanServ") {
        const QString msg = message->content();
        if (msg.startsWith("[")) {
            int idx = msg.indexOf("]");
            if (idx != -1) {
                const QString view = msg.mid(1, idx - 1);
                if (d.model->contains(view))
                    target = view;
            }
        }
    }

    if (target == "$$*") {
        // global notice
        foreach (IrcBuffer* buffer, d.model->buffers())
            buffer->receiveMessage(message);
    } else if (!message->connection()->isConnected() || target.isEmpty() || target == "*")
        sendMessage(message, d.defaultBuffer);
    else if (IrcBuffer* buffer = d.model->find(message->nick()))
        sendMessage(message, buffer);
    else if (target == message->connection()->nickName() || target.contains("*"))
        sendMessage(message, d.currentBuffer);
    else if (d.model)
        sendMessage(message, d.model->find(target));
}

void MessageHandler::handleNumericMessage(IrcNumericMessage* message)
{
    if (Irc::codeToString(message->code()).startsWith("ERR_")) {
        sendMessage(message, d.currentBuffer);
        return;
    }

    switch (message->code()) {
        case Irc::RPL_ENDOFWHO:
        case Irc::RPL_WHOREPLY:
        case Irc::RPL_UNAWAY:
        case Irc::RPL_NOWAWAY:
        case Irc::RPL_AWAY:
        case Irc::RPL_WHOISOPERATOR:
        case Irc::RPL_WHOISMODES: // "is using modes"
        case Irc::RPL_WHOISREGNICK: // "is a registered nick"
        case Irc::RPL_WHOISHELPOP: // "is available for help"
        case Irc::RPL_WHOISSPECIAL: // "is identified to services"
        case Irc::RPL_WHOISHOST: // nick is connecting from <...>
        case Irc::RPL_WHOISSECURE: // nick is using a secure connection
        case Irc::RPL_WHOISUSER:
        case Irc::RPL_WHOISSERVER:
        case Irc::RPL_WHOISACCOUNT: // nick user is logged in as
        case Irc::RPL_WHOWASUSER:
        case Irc::RPL_WHOISIDLE:
        case Irc::RPL_WHOISCHANNELS:
        case Irc::RPL_ENDOFWHOIS:
        case Irc::RPL_INVITING:
        case Irc::RPL_VERSION:
        case Irc::RPL_TIME:
            sendMessage(message, d.currentBuffer);
            break;

        case Irc::RPL_ENDOFBANLIST:
        case Irc::RPL_ENDOFEXCEPTLIST:
        case Irc::RPL_ENDOFINFO:
        case Irc::RPL_ENDOFINVITELIST:
        case Irc::RPL_ENDOFLINKS:
        case Irc::RPL_ENDOFSTATS:
        case Irc::RPL_ENDOFUSERS:
        case Irc::RPL_ENDOFWHOWAS:
        case Irc::RPL_NOTOPIC:
        case Irc::RPL_TOPIC:
        case Irc::RPL_CHANNELMODEIS:
            break; // ignore

        case Irc::RPL_CHANNEL_URL:
        case Irc::RPL_CREATIONTIME:
        case Irc::RPL_TOPICWHOTIME:
            sendMessage(message, message->parameters().value(1));
            break;

        case Irc::RPL_NAMREPLY: {
            const int count = message->parameters().count();
            const QString channel = message->parameters().value(count - 2);
            IrcBuffer* buffer = d.model->find(channel.toLower());
            if (buffer)
                buffer->receiveMessage(message);
            else if (d.currentBuffer)
                d.currentBuffer->receiveMessage(message);
            break;
        }

        case Irc::RPL_ENDOFNAMES:
            if (d.model->contains(message->parameters().value(1)))
                sendMessage(message, message->parameters().value(1));
            break;

        default:
            sendMessage(message, d.defaultBuffer);
            break;
    }
}

void MessageHandler::handlePongMessage(IrcPongMessage* message)
{
    sendMessage(message, d.currentBuffer);
}

void MessageHandler::handleUnknownMessage(IrcMessage* message)
{
    sendMessage(message, d.defaultBuffer);
}

void MessageHandler::sendMessage(IrcMessage* message, IrcBuffer* buffer)
{
    if (buffer)
        buffer->receiveMessage(message);
}

void MessageHandler::sendMessage(IrcMessage* message, const QString& buffer)
{
    sendMessage(message, d.model->find(buffer));
}
