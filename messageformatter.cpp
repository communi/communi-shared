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

#include "messageformatter.h"
#include <IrcTextFormat>
#include <IrcConnection>
#include <IrcUserModel>
#include <IrcMessage>
#include <IrcPalette>
#include <IrcChannel>
#include <Irc>
#include <QHash>
#include <QTime>
#include <QColor>
#include <QCoreApplication>
#include <QTextBoundaryFinder>

IRC_USE_NAMESPACE

static QString formatSeconds(int secs)
{
    const QDateTime time = QDateTime::fromTime_t(secs);
    return MessageFormatter::tr("%1s").arg(time.secsTo(QDateTime::currentDateTime()));
}

static QString formatDuration(int secs)
{
    QStringList idle;
    if (int days = secs / 86400)
        idle += MessageFormatter::tr("%1 days").arg(days);
    secs %= 86400;
    if (int hours = secs / 3600)
        idle += MessageFormatter::tr("%1 hours").arg(hours);
    secs %= 3600;
    if (int mins = secs / 60)
        idle += MessageFormatter::tr("%1 mins").arg(mins);
    idle += MessageFormatter::tr("%1 secs").arg(secs % 60);
    return idle.join(" ");
}

MessageFormatter::MessageFormatter(QObject* parent) : QObject(parent)
{
    d.buffer = 0;
    d.textFormat = new IrcTextFormat(this);
    d.textFormat->setSpanFormat(IrcTextFormat::SpanClass);
    d.baseColor = QColor::fromHsl(359, 102, 134);

    d.userModel = new IrcUserModel(this);
    connect(d.userModel, SIGNAL(namesChanged(QStringList)), this, SLOT(indexNames(QStringList)));
}

IrcBuffer* MessageFormatter::buffer() const
{
    return d.buffer;
}

void MessageFormatter::setBuffer(IrcBuffer* buffer)
{
    if (d.buffer != buffer) {
        d.buffer = buffer;
        d.userModel->setChannel(qobject_cast<IrcChannel*>(buffer));
    }
}

IrcTextFormat* MessageFormatter::textFormat() const
{
    return d.textFormat;
}

void MessageFormatter::setTextFormat(IrcTextFormat* format)
{
    d.textFormat = format;
}

QColor MessageFormatter::baseColor() const
{
    return d.baseColor;
}

void MessageFormatter::setBaseColor(const QColor& color)
{
    d.baseColor = color;
}

MessageFormat MessageFormatter::formatMessage(IrcMessage* msg) const
{
    MessageFormat fmt;
    switch (msg->type()) {
        case IrcMessage::Invite:
            fmt = formatInviteMessage(static_cast<IrcInviteMessage*>(msg));
            break;
        case IrcMessage::Join:
            fmt = formatJoinMessage(static_cast<IrcJoinMessage*>(msg));
            break;
        case IrcMessage::Kick:
            fmt = formatKickMessage(static_cast<IrcKickMessage*>(msg));
            break;
        case IrcMessage::Mode:
            fmt = formatModeMessage(static_cast<IrcModeMessage*>(msg));
            break;
        case IrcMessage::Names:
            fmt = formatNamesMessage(static_cast<IrcNamesMessage*>(msg));
            break;
        case IrcMessage::Nick:
            fmt = formatNickMessage(static_cast<IrcNickMessage*>(msg));
            break;
        case IrcMessage::Notice:
            fmt = formatNoticeMessage(static_cast<IrcNoticeMessage*>(msg));
            break;
        case IrcMessage::Numeric:
            fmt = formatNumericMessage(static_cast<IrcNumericMessage*>(msg));
            break;
        case IrcMessage::Part:
            fmt = formatPartMessage(static_cast<IrcPartMessage*>(msg));
            break;
        case IrcMessage::Pong:
            fmt = formatPongMessage(static_cast<IrcPongMessage*>(msg));
            break;
        case IrcMessage::Private:
            fmt = formatPrivateMessage(static_cast<IrcPrivateMessage*>(msg));
            break;
        case IrcMessage::Quit:
            fmt = formatQuitMessage(static_cast<IrcQuitMessage*>(msg));
            break;
        case IrcMessage::Topic:
            fmt = formatTopicMessage(static_cast<IrcTopicMessage*>(msg));
            break;
        case IrcMessage::Unknown:
            fmt = formatUnknownMessage(msg);
            break;
        default:
            break;
    }
    return fmt;
}

QString MessageFormatter::formatText(const QString& text, Qt::TextFormat format) const
{
    d.textFormat->parse(text);
    if (format == Qt::PlainText)
        return d.textFormat->plainText();

    QString msg = d.textFormat->html();
    if (!d.names.isEmpty()) {
        QTextBoundaryFinder finder = QTextBoundaryFinder(QTextBoundaryFinder::Word, msg);
        int pos = 0;
        while (pos < msg.length()) {
            const QChar c = msg.at(pos);
            if (!c.isSpace()) {
                // do not format nicks within links
                if (c == '<' && msg.midRef(pos, 3) == "<a ") {
                    const int end = msg.indexOf("</a>", pos + 3);
                    if (end != -1) {
                        pos = end + 4;
                        continue;
                    }
                }
                // test word start boundary
                finder.setPosition(pos);
                if (finder.isAtBoundary()) {
                    QMultiHash<QChar, QString>::const_iterator it = d.names.find(c);
                    while (it != d.names.constEnd() && it.key() == c) {
                        const QString& user = it.value();
                        if (msg.midRef(pos, user.length()) == user) {
                            // test word end boundary
                            finder.setPosition(pos + user.length());
                            if (finder.isAtBoundary()) {
                                const QString formatted = formatNick(user, format, true);
                                msg.replace(pos, user.length(), formatted);
                                pos += formatted.length();
                                finder = QTextBoundaryFinder(QTextBoundaryFinder::Word, msg);
                            }
                        }
                        ++it;
                    }
                }
            }
            ++pos;
        }
    }
    return msg;
}

QString MessageFormatter::formatNick(const QString& nick, Qt::TextFormat format, bool colorize) const
{
    if (format == Qt::PlainText)
        return nick;

    int h = qHash(nick) % 359;
    int s = colorize ? d.baseColor.saturation() : 0;
    int l = d.baseColor.lightness();
    QString n = Irc::nickFromPrefix(nick);
    if (n.isEmpty())
        n = nick;
    return tr("<b><font color=\"%1\">%2</b>").arg(QColor::fromHsl(h, s, l).name(), n);
}

QString MessageFormatter::formatPrefix(const QString& prefix, Qt::TextFormat format, bool colorize) const
{
    QString nick = formatNick(Irc::nickFromPrefix(prefix), format, colorize);
    QString ident = Irc::identFromPrefix(prefix);
    QString host = Irc::hostFromPrefix(prefix);
    if (!ident.isEmpty() && !host.isEmpty())
        return tr("%1 (%2@%3)").arg(nick, ident, host);
    return nick;
}

MessageFormat MessageFormatter::formatInviteMessage(IrcInviteMessage* msg) const
{
    MessageFormat fmt;
    fmt.plainText = tr("%1 invited to %3").arg(formatNick(msg->nick(), Qt::PlainText), msg->channel());
    fmt.richText = tr("! %1 invited to %3").arg(formatNick(msg->nick(), Qt::RichText), msg->channel());
    fmt.detailed = tr("! %1 invited to %3").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), msg->channel());
    return fmt;
}

MessageFormat MessageFormatter::formatJoinMessage(IrcJoinMessage* msg) const
{
    const bool repeat = d.repeats.value(d.buffer);
    if (msg->isOwn())
        d.repeats.insert(d.buffer, false);

    MessageFormat fmt;
    if (msg->isOwn() && repeat) {
        fmt.plainText = tr("%1 rejoined").arg(formatNick(msg->nick(), Qt::PlainText));
        fmt.richText = tr("! %1 rejoined").arg(formatNick(msg->nick(), Qt::RichText));
        fmt.detailed = tr("! %1 rejoined %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), msg->channel());
    } else {
        if (msg->isOwn()) {
            fmt.plainText = tr("%1 joined %2").arg(formatNick(msg->nick(), Qt::PlainText), msg->channel());
            fmt.richText = tr("! %1 joined %2").arg(formatNick(msg->nick(), Qt::RichText), msg->channel());
        } else {
            fmt.plainText = tr("%1 joined").arg(formatNick(msg->nick(), Qt::PlainText));
            fmt.richText = tr("! %1 joined").arg(formatNick(msg->nick(), Qt::RichText));
        }
        fmt.detailed = tr("! %1 joined %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), msg->channel());
    }
    return fmt;
}

MessageFormat MessageFormatter::formatKickMessage(IrcKickMessage* msg) const
{
    MessageFormat fmt;
    const bool isMe = !msg->user().compare(msg->connection()->nickName());
    fmt.plainText = tr("%1 kicked %2").arg(formatNick(msg->nick(), Qt::PlainText),
                                           formatNick(msg->user(), Qt::PlainText));
    fmt.richText = tr("! %1 kicked %2").arg(formatNick(msg->nick(), Qt::RichText),
                                          formatNick(msg->user(), Qt::RichText));
    if (msg->reason().isEmpty())
        fmt.detailed = tr("! %1 kicked %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()),
                                                formatNick(msg->user(), Qt::RichText, !isMe));
    else
        fmt.detailed = tr("! %1 kicked %2 (%3)").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()),
                                                     formatNick(msg->user(), Qt::RichText, !isMe));
    return fmt;
}

MessageFormat MessageFormatter::formatModeMessage(IrcModeMessage* msg) const
{
    MessageFormat fmt;
    if (msg->isReply()) {
        fmt.plainText = tr("%1 mode is %2 %3").arg(msg->target(), msg->mode(), msg->argument());
        fmt.richText = tr("! %1 mode is %2 %3").arg(msg->target(), msg->mode(), msg->argument());
    } else {
        fmt.plainText = tr("%1 sets mode %2 %3").arg(formatNick(msg->nick(), Qt::PlainText), msg->mode(), msg->argument());
        fmt.richText = tr("! %1 sets mode %2 %3").arg(formatNick(msg->nick(), Qt::RichText), msg->mode(), msg->argument());
        fmt.detailed = tr("! %1 sets mode %2 %3").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), msg->mode(), msg->argument());
    }
    return fmt;
}

MessageFormat MessageFormatter::formatNamesMessage(IrcNamesMessage* msg) const
{
    const bool repeat = d.repeats.value(d.buffer);
    d.repeats.insert(d.buffer, true);

    MessageFormat fmt;
    if (repeat) {
        QStringList names = msg->names();
        qSort(names);
        fmt.plainText = tr("%1 has %3 users: %2").arg(msg->channel(), formatNames(names, Qt::PlainText)).arg(names.count());
        fmt.richText = tr("! %1 has %3 users: %2").arg(msg->channel(), formatNames(names, Qt::RichText)).arg(names.count());
    } else {
        fmt.plainText = tr("%1 has %2 users").arg(msg->channel()).arg(msg->names().count());
        fmt.richText = tr("! %1 has %2 users").arg(msg->channel()).arg(msg->names().count());
    }
    return fmt;
}

MessageFormat MessageFormatter::formatNickMessage(IrcNickMessage* msg) const
{
    MessageFormat fmt;
    fmt.plainText = tr("%1 changed nick to %2").arg(formatNick(msg->nick(), Qt::PlainText),
                                                    formatNick(msg->newNick(), Qt::PlainText));
    fmt.richText = tr("! %1 changed nick to %2").arg(formatNick(msg->nick(), Qt::RichText),
                                                     formatNick(msg->newNick(), Qt::RichText));
    fmt.detailed = tr("! %1 changed nick to %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()),
                                                     formatNick(msg->newNick(), Qt::RichText, !msg->isOwn()));
    return fmt;
}

MessageFormat MessageFormatter::formatNoticeMessage(IrcNoticeMessage* msg) const
{
    MessageFormat fmt;
    if (msg->isReply()) {
        const QStringList params = msg->content().split(" ", QString::SkipEmptyParts);
        const QString cmd = params.value(0);
        if (cmd.toUpper() == "PING") {
            const QString secs = formatSeconds(params.value(1).toInt());
            fmt.plainText = tr("%1 replied in %2").arg(formatNick(msg->nick(), Qt::PlainText), secs);
            fmt.richText = tr("! %1 replied in %2").arg(formatNick(msg->nick(), Qt::RichText), secs);
            fmt.detailed = tr("! %1 replied in %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), secs);
            return fmt;
        } else if (cmd.toUpper() == "TIME") {
            const QString rest = QStringList(params.mid(1)).join(" ");
            fmt.plainText = tr("%1 time is %2").arg(formatNick(msg->nick(), Qt::PlainText), rest);
            fmt.richText = tr("! %1 time is %2").arg(formatNick(msg->nick(), Qt::RichText), rest);
            fmt.detailed = tr("! %1 time is %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), rest);
            return fmt;
        } else if (cmd.toUpper() == "VERSION") {
            const QString rest = QStringList(params.mid(1)).join(" ");
            fmt.plainText = tr("%1 version is %2").arg(formatNick(msg->nick(), Qt::PlainText), rest);
            fmt.richText = tr("! %1 version is %2").arg(formatNick(msg->nick(), Qt::RichText), rest);
            fmt.detailed = tr("! %1 version is %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), rest);
            return fmt;
        }
    }

    fmt.plainText = formatText(msg->content(), Qt::PlainText);
    const QString rich = formatText(msg->content(), Qt::RichText);
    fmt.richText = tr("[%1] %2").arg(formatNick(msg->nick(), Qt::RichText), rich);
    fmt.detailed = tr("[%1] %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), rich);
    return fmt;
}

#define P_(x) msg->parameters().value(x)
#define MID_(x) QStringList(msg->parameters().mid(x)).join(" ")

MessageFormat MessageFormatter::formatNumericMessage(IrcNumericMessage* msg) const
{
    MessageFormat fmt;

    const bool repeat = d.repeats.value(d.buffer);
    if (msg->code() < 300) {
        if (!repeat) {
            fmt.plainText = formatText(MID_(1), Qt::PlainText);
            fmt.richText = tr("[INFO] %1").arg(formatText(MID_(1), Qt::RichText));
        }
        return fmt;
    }

    switch (msg->code()) {
        case Irc::RPL_MOTDSTART:
        case Irc::RPL_MOTD:
            if (!repeat) {
                fmt.plainText = formatText(MID_(1), Qt::PlainText);
                fmt.richText = tr("[MOTD] %1").arg(formatText(MID_(1), Qt::RichText));
            }
            break;

        case Irc::RPL_ENDOFMOTD:
        case Irc::ERR_NOMOTD:
            d.repeats.insert(d.buffer, true);
            if (repeat) {
                fmt.plainText = tr("%1 reconnected").arg(d.buffer->connection()->nickName());
                fmt.richText = tr("! %1 reconnected").arg(d.buffer->connection()->nickName());
            }
            break;

        case Irc::RPL_AWAY:
            fmt.plainText = tr("%1 is away (%2)").arg(formatNick(P_(1), Qt::PlainText), MID_(2));
            fmt.richText = tr("! %1 is away (%2)").arg(formatNick(P_(1), Qt::RichText), MID_(2));
            break;

        case Irc::RPL_WHOISOPERATOR:
        case Irc::RPL_WHOISMODES: // "is using modes"
        case Irc::RPL_WHOISREGNICK: // "is a registered nick"
        case Irc::RPL_WHOISHELPOP: // "is available for help"
        case Irc::RPL_WHOISSPECIAL: // "is identified to services"
        case Irc::RPL_WHOISHOST: // nick is connecting from <...>
        case Irc::RPL_WHOISSECURE: // nick is using a secure connection
            fmt.plainText = tr("%1 %2").arg(formatNick(P_(1), Qt::PlainText), formatText(MID_(2), Qt::PlainText));
            fmt.richText = tr("! %1 %2").arg(formatNick(P_(1), Qt::RichText), formatText(MID_(2), Qt::RichText));
            break;

        case Irc::RPL_WHOISUSER:
            fmt.plainText = tr("%1 is %2@%3 (%4)").arg(formatNick(P_(1), Qt::PlainText), P_(2), P_(3), formatText(MID_(5), Qt::PlainText));
            fmt.richText = tr("! %1 is %2@%3 (%4)").arg(formatNick(P_(1), Qt::RichText), P_(2), P_(3), formatText(MID_(5), Qt::RichText));
            break;

        case Irc::RPL_WHOISSERVER:
            fmt.plainText = tr("%1 connected via %2 (%3)").arg(formatNick(P_(1), Qt::PlainText), P_(2), P_(3));
            fmt.richText = tr("! %1 connected via %2 (%3)").arg(formatNick(P_(1), Qt::RichText), P_(2), P_(3));
            break;

        case Irc::RPL_WHOISACCOUNT: // nick user is logged in as
            fmt.plainText = tr("%1 %3 %2").arg(formatNick(P_(1), Qt::PlainText), P_(2), P_(3));
            fmt.richText = tr("! %1 %3 %2").arg(formatNick(P_(1), Qt::RichText), P_(2), P_(3));
            break;

        case Irc::RPL_WHOWASUSER:
            fmt.plainText = tr("%1 was %2@%3 %4 %5").arg(formatNick(P_(1), Qt::PlainText), P_(2), P_(3), P_(4), P_(5));
            fmt.richText = tr("! %1 was %2@%3 %4 %5").arg(formatNick(P_(1), Qt::RichText), P_(2), P_(3), P_(4), P_(5));
            break;

        case Irc::RPL_WHOISIDLE: {
            const QDateTime signon = QDateTime::fromTime_t(P_(3).toInt());
            const QString idle = formatDuration(P_(2).toInt());
            fmt.plainText = tr("%1 has been online since %2 (idle for %3)").arg(formatNick(P_(1), Qt::PlainText), signon.toString(), idle);
            fmt.richText = tr("! %1 has been online since %2 (idle for %3)").arg(formatNick(P_(1), Qt::RichText), signon.toString(), idle);
            break;
        }

        case Irc::RPL_WHOISCHANNELS:
            fmt.plainText = tr("%1 is on channels %2").arg(formatNick(P_(1), Qt::PlainText), P_(2));
            fmt.richText = tr("! %1 is on channels %2").arg(formatNick(P_(1), Qt::RichText), P_(2));
            break;

        case Irc::RPL_CHANNEL_URL:
            if (!repeat) {
                fmt.plainText = tr("%1 url is %2").arg(P_(1), formatText(P_(2), Qt::PlainText));
                fmt.richText = tr("! %1 url is %2").arg(P_(1), formatText(P_(2), Qt::RichText));
            }
            break;

        case Irc::RPL_CREATIONTIME:
            if (!repeat) {
                const QDateTime dateTime = QDateTime::fromTime_t(P_(2).toInt());
                fmt.plainText = tr("%1 was created %2").arg(P_(1), dateTime.toString());
                fmt.richText = tr("! %1 was created %2").arg(P_(1), dateTime.toString());
            }
            break;

        case Irc::RPL_TOPICWHOTIME:
            if (!repeat) {
                const QDateTime dateTime = QDateTime::fromTime_t(P_(3).toInt());
                fmt.plainText = tr("%1 topic was set %2 by %3").arg(P_(1), dateTime.toString(), formatNick(P_(2), Qt::PlainText));
                fmt.richText = tr("! %1 topic was set %2 by %3").arg(P_(1), dateTime.toString(), formatNick(P_(2), Qt::RichText));
            }
            break;

        case Irc::RPL_INVITING:
            fmt.plainText = tr("inviting %1 to %2").arg(formatNick(P_(1), Qt::PlainText), P_(2));
            fmt.richText = tr("! inviting %1 to %2").arg(formatNick(P_(1), Qt::RichText), P_(2));
            break;

        case Irc::RPL_VERSION:
            fmt.plainText = tr("%1 version is %2").arg(formatNick(msg->nick(), Qt::PlainText), P_(1));
            fmt.richText = tr("! %1 version is %2").arg(formatNick(msg->nick(), Qt::RichText), P_(1));
            break;

        case Irc::RPL_TIME:
            fmt.plainText = tr("%1 time is %2").arg(formatNick(P_(1), Qt::PlainText), P_(2));
            fmt.richText = tr("! %1 time is %2").arg(formatNick(P_(1), Qt::RichText), P_(2));
            break;

        case Irc::RPL_UNAWAY:
        case Irc::RPL_NOWAWAY:
            fmt.plainText = formatText(P_(1), Qt::PlainText);
            fmt.richText = tr("! %1").arg(formatText(P_(1), Qt::RichText));
            break;

        case Irc::RPL_TOPIC:
        case Irc::RPL_NAMREPLY:
        case Irc::RPL_ENDOFNAMES:
        case Irc::RPL_ENDOFWHOIS:
            break;

        default:
            fmt.plainText = formatText(MID_(1), Qt::PlainText);
            if (Irc::codeToString(msg->code()).startsWith("ERR_"))
                fmt.richText = tr("[ERROR] %1").arg(formatText(MID_(1), Qt::RichText));
            else
                fmt.richText = tr("! %1").arg(formatText(MID_(1), Qt::RichText));
            break;
    }

    return fmt;
}

MessageFormat MessageFormatter::formatPartMessage(IrcPartMessage* msg) const
{
    MessageFormat fmt;
    fmt.plainText = tr("%1 parted").arg(formatNick(msg->nick(), Qt::PlainText));
    fmt.richText = tr("! %1 parted").arg(formatNick(msg->nick(), Qt::RichText));
    if (msg->reason().isEmpty())
        fmt.detailed = tr("! %1 parted %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), msg->channel());
    else
        fmt.detailed = tr("! %1 parted %2 (%3)").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), msg->channel(), formatText(msg->reason(), Qt::RichText));
    return fmt;
}

MessageFormat MessageFormatter::formatPongMessage(IrcPongMessage* msg) const
{
    MessageFormat fmt;
    const QString secs = formatSeconds(msg->argument().toInt());
    fmt.plainText = tr("%1 replied in %2").arg(formatNick(msg->nick(), Qt::PlainText), secs);
    fmt.richText = tr("! %1 replied in %2").arg(formatNick(msg->nick(), Qt::RichText), secs);
    fmt.detailed = tr("! %1 replied in %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), secs);
    return fmt;
}

MessageFormat MessageFormatter::formatPrivateMessage(IrcPrivateMessage* msg) const
{
    MessageFormat fmt;
    if (msg->isRequest()) {
        fmt.plainText = tr("%1 requested %2").arg(formatNick(msg->nick(), Qt::PlainText),
                                                  msg->content().split(" ").value(0).toUpper());
        fmt.richText = tr("! %1 requested %2").arg(formatNick(msg->nick(), Qt::RichText),
                                                   msg->content().split(" ").value(0).toUpper());
        fmt.detailed = tr("! %1 requested %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()),
                                                   msg->content().split(" ").value(0).toUpper());
    } else if (msg->isAction()) {
        const QString rich = formatText(msg->content(), Qt::RichText);
        fmt.plainText = tr("%1 %2").arg(msg->nick(), formatText(msg->content(), Qt::PlainText));
        fmt.richText = tr("* %1 %2").arg(msg->nick(), rich);
        fmt.detailed = tr("* %1 %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), rich);
    } else {
        const QString rich = formatText(msg->content(), Qt::RichText);
        fmt.plainText = formatText(msg->content(), Qt::PlainText);
        fmt.richText = tr("&lt;%1&gt; %2").arg(formatNick(msg->nick(), Qt::RichText, !msg->isOwn()), rich);
        fmt.detailed = tr("&lt;%1&gt; %2").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), rich);
    }
    return fmt;
}

MessageFormat MessageFormatter::formatQuitMessage(IrcQuitMessage* msg) const
{
    MessageFormat fmt;
    fmt.plainText = tr("%1 has quit").arg(formatNick(msg->nick(), Qt::PlainText));
    fmt.richText = tr("! %1 has quit").arg(formatNick(msg->nick(), Qt::RichText));
    if (msg->reason().isEmpty())
        fmt.detailed = tr("! %1 has quit").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()));
    else
        fmt.detailed = tr("! %1 has quit (%2)").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), formatText(msg->reason(), Qt::RichText));
    return fmt;
}

MessageFormat MessageFormatter::formatTopicMessage(IrcTopicMessage* msg) const
{
    MessageFormat fmt;
    if (msg->isReply()) {
        if (msg->topic().isEmpty()) {
            fmt.plainText = tr("%1 has no topic").arg(msg->channel());
            fmt.richText = tr("! %1 has no topic").arg(msg->channel());
        } else {
            fmt.plainText = tr("%1 topic is \"%2\"").arg(msg->channel(), formatText(msg->topic(), Qt::PlainText));
            fmt.richText = tr("! %1 topic is \"%2\"").arg(msg->channel(), formatText(msg->topic(), Qt::RichText));
        }
    } else {
        fmt.plainText = tr("%1 changed topic").arg(formatNick(msg->nick(), Qt::PlainText));
        fmt.richText = tr("! %1 changed topic").arg(formatNick(msg->nick(), Qt::RichText));
        if (msg->topic().isEmpty())
            fmt.detailed = tr("! %1 cleared %2 topic").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), msg->channel());
        else
            fmt.detailed = tr("! %1 set topic \"%2\" on %3").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), formatText(msg->topic(), Qt::RichText), msg->channel());
    }
    return fmt;
}

MessageFormat MessageFormatter::formatUnknownMessage(IrcMessage* msg) const
{
    MessageFormat fmt;
    fmt.plainText = tr("%1 %2 %3").arg(formatNick(msg->nick(), Qt::PlainText), msg->command(), msg->parameters().join(" "));
    fmt.richText = tr("? %1 %2 %3").arg(formatNick(msg->nick(), Qt::RichText), msg->command(), msg->parameters().join(" "));
    fmt.plainText = tr("? %1 %2 %3").arg(formatPrefix(msg->prefix(), Qt::RichText, !msg->isOwn()), msg->command(), msg->parameters().join(" "));
    return fmt;
}

void MessageFormatter::indexNames(const QStringList& names)
{
    d.names.clear();
    foreach (const QString& name, names) {
        if (!name.isEmpty())
            d.names.insert(name.at(0), name);
    }
}

QString MessageFormatter::formatNames(const QStringList &names, Qt::TextFormat format, int columns) const
{
    if (format == Qt::PlainText)
        return names.join(" ");
    QString message;
    message += "<table>";
    for (int i = 0; i < names.count(); i += columns)
    {
        message += "<tr>";
        for (int j = 0; j < columns; ++j) {
            QString nick = Irc::nickFromPrefix(names.value(i+j));
            if (nick.isEmpty())
                nick = names.value(i+j);
            message += "<td>" + formatNick(nick, format, true) + "&nbsp;</td>";
        }
        message += "</tr>";
    }
    message += "</table>";
    return message;
}
