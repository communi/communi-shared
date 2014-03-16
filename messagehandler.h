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

#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <QObject>
#include <QPointer>
#include <IrcMessage>

class IrcBuffer;
class IrcBufferModel;

class MessageHandler : public QObject
{
    Q_OBJECT

public:
    explicit MessageHandler(QObject* parent = 0);
    virtual ~MessageHandler();

    IrcBufferModel* model() const;
    void setModel(IrcBufferModel* model);

    IrcBuffer* defaultBuffer() const;
    IrcBuffer* currentBuffer() const;

public slots:
    void setDefaultBuffer(IrcBuffer* buffer);
    void setCurrentBuffer(IrcBuffer* buffer);

protected slots:
    void handleMessage(IrcMessage* message);

protected:
    void handleInviteMessage(IrcInviteMessage* message);
    void handleNoticeMessage(IrcNoticeMessage* message);
    void handleNumericMessage(IrcNumericMessage* message);
    void handlePongMessage(IrcPongMessage* message);
    void handleUnknownMessage(IrcMessage* message);

private:
    void sendMessage(IrcMessage* message, IrcBuffer* buffer);
    void sendMessage(IrcMessage* message, const QString& buffer);

    struct Private {
        QPointer<IrcBufferModel> model;
        QPointer<IrcBuffer> defaultBuffer;
        QPointer<IrcBuffer> currentBuffer;
    } d;
};

#endif // MESSAGEHANDLER_H
