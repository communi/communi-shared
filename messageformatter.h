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

#ifndef MESSAGEFORMATTER_H
#define MESSAGEFORMATTER_H

#include <QHash>
#include <QColor>
#include <QString>
#include <QDateTime>
#include <IrcGlobal>

IRC_FORWARD_DECLARE_CLASS(IrcBuffer)
IRC_FORWARD_DECLARE_CLASS(IrcUserModel)
IRC_FORWARD_DECLARE_CLASS(IrcTextFormat)
IRC_FORWARD_DECLARE_CLASS(IrcInviteMessage)
IRC_FORWARD_DECLARE_CLASS(IrcJoinMessage)
IRC_FORWARD_DECLARE_CLASS(IrcKickMessage)
IRC_FORWARD_DECLARE_CLASS(IrcModeMessage)
IRC_FORWARD_DECLARE_CLASS(IrcNamesMessage)
IRC_FORWARD_DECLARE_CLASS(IrcNickMessage)
IRC_FORWARD_DECLARE_CLASS(IrcNoticeMessage)
IRC_FORWARD_DECLARE_CLASS(IrcNumericMessage)
IRC_FORWARD_DECLARE_CLASS(IrcPartMessage)
IRC_FORWARD_DECLARE_CLASS(IrcPongMessage)
IRC_FORWARD_DECLARE_CLASS(IrcPrivateMessage)
IRC_FORWARD_DECLARE_CLASS(IrcQuitMessage)
IRC_FORWARD_DECLARE_CLASS(IrcTopicMessage)
IRC_FORWARD_DECLARE_CLASS(IrcMessage)

struct MessageFormat
{
    QString plainText;
    QString richText;
    QString detailed;
};

class MessageFormatter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(IrcBuffer* buffer READ buffer WRITE setBuffer)
    Q_PROPERTY(IrcTextFormat* textFormat READ textFormat WRITE setTextFormat)
    Q_PROPERTY(QColor baseColor READ baseColor WRITE setBaseColor)

public:
    explicit MessageFormatter(QObject* parent = 0);

    IrcBuffer* buffer() const;
    void setBuffer(IrcBuffer* buffer);

    IrcTextFormat* textFormat() const;
    void setTextFormat(IrcTextFormat* format);

    QColor baseColor() const;
    void setBaseColor(const QColor& color);

    MessageFormat formatMessage(IrcMessage* message) const;

    QString formatText(const QString& text, Qt::TextFormat format) const;
    QString formatNick(const QString& nick, Qt::TextFormat format, bool colorize = false) const;
    QString formatPrefix(const QString& prefix, Qt::TextFormat format, bool colorize = false) const;

protected:
    MessageFormat formatInviteMessage(IrcInviteMessage* message) const;
    MessageFormat formatJoinMessage(IrcJoinMessage* message) const;
    MessageFormat formatKickMessage(IrcKickMessage* message) const;
    MessageFormat formatModeMessage(IrcModeMessage* message) const;
    MessageFormat formatNamesMessage(IrcNamesMessage* message) const;
    MessageFormat formatNickMessage(IrcNickMessage* message) const;
    MessageFormat formatNoticeMessage(IrcNoticeMessage* message) const;
    MessageFormat formatNumericMessage(IrcNumericMessage* message) const;
    MessageFormat formatPartMessage(IrcPartMessage* message) const;
    MessageFormat formatPongMessage(IrcPongMessage* message) const;
    MessageFormat formatPrivateMessage(IrcPrivateMessage* message) const;
    MessageFormat formatQuitMessage(IrcQuitMessage* message) const;
    MessageFormat formatTopicMessage(IrcTopicMessage* message) const;
    MessageFormat formatUnknownMessage(IrcMessage* message) const;

private slots:
    void indexNames(const QStringList& names);

private:
    QString formatNames(const QStringList& names, Qt::TextFormat format, int columns = 6) const;

    struct Private {
        QColor baseColor;
        IrcBuffer* buffer;
        IrcUserModel* userModel;
        IrcTextFormat* textFormat;
        QMultiHash<QChar, QString> names;
        mutable QHash<IrcBuffer*, bool> repeats;
    } d;
};

#endif // MESSAGEFORMATTER_H
