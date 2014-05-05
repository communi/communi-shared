/*
 * Copyright (C) 2008-2014 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "messageformatter.h"
#include "tst_ircclientserver.h"
#include "tst_ircdata.h"
#include <IrcConnection>
#include <IrcBufferModel>
#include <QtTest/QtTest>

class tst_MessageFormatter : public tst_IrcClientServer
{
    Q_OBJECT

private slots:
    void testFormatHtml_data();
    void testFormatHtml();
};

void tst_MessageFormatter::testFormatHtml_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QString>("channel");
    QTest::addColumn<QString>("content");
    QTest::addColumn<QString>("output");

    QTest::newRow("freenode") << QByteArray("freenode") << "#freenode"
                              << "Vestibulum destiny ow <a href='http://wow.fi'>wow.fi</a> <a href='www.tt.fi'>TT</a> libero tt eget metus"
                              << "<span class='message'><span class='timestamp'>[00:00:00]</span> &lt;<b><a href='nick:communi' style='text-decoration:none; color:#747474'>communi</a></b>&gt; Vestibulum <b><a href='nick:destiny' style='text-decoration:none; color:#b73157'>destiny</a></b> <b><a href='nick:ow' style='text-decoration:none; color:#b73176'>ow</a></b> &lt;a href='<a href='http://wow.fi'>http://wow.fi</a>'>wow.fi&lt;/a> &lt;a href='<a href='http://www.tt.fi'>www.tt.fi</a>'>TT&lt;/a> libero <b><a href='nick:tt' style='text-decoration:none; color:#31b736'>tt</a></b> eget metus</span>";

    QTest::newRow("ircnet") << QByteArray("ircnet") << "#uptimed"
                            << "Phasellus enim mynic, sodales 'jazz' tincidunt quis, ultricies loco."
                            << "<span class='message'><span class='timestamp'>[00:00:00]</span> &lt;<b><a href='nick:communi' style='text-decoration:none; color:#747474'>communi</a></b>&gt; Phasellus enim <b><a href='nick:mynic' style='text-decoration:none; color:#4ab731'>mynic</a></b>, sodales '<b><a href='nick:jazz' style='text-decoration:none; color:#7b31b7'>jazz</a></b>' tincidunt quis, ultricies <b><a href='nick:loco' style='text-decoration:none; color:#a7b731'>loco</a></b>.</span>";

    QTest::newRow("euirc") << QByteArray("euirc") << "#euirc"
                           << "Jerry: porttitor netsplit tristique. Aenean semper ligula eget nulla condimentum tempor in quis felis. Sed sem scaba, tincidunt amet sb."
                           << "<span class='message'><span class='timestamp'>[00:00:00]</span> &lt;<b><a href='nick:communi' style='text-decoration:none; color:#747474'>communi</a></b>&gt; <b><a href='nick:Jerry' style='text-decoration:none; color:#3fb731'>Jerry</a></b>: porttitor <b><a href='nick:netsplit' style='text-decoration:none; color:#31b757'>netsplit</a></b> tristique. Aenean semper ligula eget nulla condimentum tempor in quis felis. Sed sem <b><a href='nick:scaba' style='text-decoration:none; color:#b7ac31'>scaba</a></b>, tincidunt amet <b><a href='nick:sb' style='text-decoration:none; color:#9ab731'>sb</a></b>.</span>";

    QTest::newRow("special chars") << QByteArray("freenode") << "#freenode"
                                   << "[MortiKi] \\mSg Ricardo__ `- RDash[AW] s1lent_1"
                                   << "<span class='message'><span class='timestamp'>[00:00:00]</span> &lt;<b><a href='nick:communi' style='text-decoration:none; color:#747474'>communi</a></b>&gt; <b><a href='nick:[MortiKi]' style='text-decoration:none; color:#3831b7'>[MortiKi]</a></b> <b><a href='nick:\\mSg' style='text-decoration:none; color:#4831b7'>\\mSg</a></b> <b><a href='nick:Ricardo__' style='text-decoration:none; color:#6db731'>Ricardo__</a></b> <b><a href='nick:`-' style='text-decoration:none; color:#31b772'>`-</a></b> <b><a href='nick:RDash[AW]' style='text-decoration:none; color:#b231b7'>RDash[AW]</a></b> <b><a href='nick:s1lent_1' style='text-decoration:none; color:#3176b7'>s1lent_1</a></b></span>";
}

void tst_MessageFormatter::testFormatHtml()
{
    QFETCH(QByteArray, key);
    QFETCH(QString, channel);
    QFETCH(QString, content);
    QFETCH(QString, output);

    IrcBufferModel model;
    model.setConnection(connection);

    connection->open();
    QVERIFY(waitForOpened());
    QVERIFY(waitForWritten(tst_IrcData::welcome(key)));
    QVERIFY(waitForWritten(tst_IrcData::join(key)));

    IrcBuffer* buffer = model.find(channel);
    QVERIFY(buffer);

    MessageFormatter formatter;
    formatter.setBuffer(buffer);

    IrcMessage* message = IrcMessage::fromData(":communi!~communi@hidd.en PRIVMSG " + channel.toUtf8() + " :" + content.toUtf8(), connection);
    QVERIFY(message);
    QDateTime timestamp = QDateTime::currentDateTime();
    timestamp.setTime(QTime(0, 0, 0));
    message->setTimeStamp(timestamp);
    QBENCHMARK {
        formatter.formatMessage(message);
    }
    QCOMPARE(formatter.formatMessage(message), output);
}

QTEST_MAIN(tst_MessageFormatter)

#include "tst_messageformatter.moc"
