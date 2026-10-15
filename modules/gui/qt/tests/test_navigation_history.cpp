/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "../util/navigation_history.hpp"

#include <QCoreApplication>
#include <QTest>
#include <QSignalSpy>

class TestNavigationHistory : public QObject
{
    Q_OBJECT

private:
    // stands for the real validator (isNavigationNodeValid), which needs the
    // network and media library models: a node is invalid when it carries
    // the "gone" property, as a network item that left its tree would be
    static bool isNodeValid(const QQmlPropertyMap& properties)
    {
        return !properties.value("gone").toBool();
    }

    static QVariantMap goneNode()
    {
        return QVariantMap{{"gone", true}};
    }

    // [home] -> [video] -> [music], then back to [video]
    void goBackOnce(NavigationHistory& history)
    {
        history.update({"home"});
        history.push({"video"});
        history.push({"music"});
        history.previous();
    }

private slots:
    void testPreviousKeepsForward()
    {
        NavigationHistory history(&isNodeValid);
        goBackOnce(history);
        QCOMPARE(history.viewPath(), QStringList{"video"});
        QVERIFY(!history.isNextEmpty());
        QVERIFY(!history.isPreviousEmpty());
    }

    void testNextRestores()
    {
        NavigationHistory history(&isNodeValid);
        goBackOnce(history);

        QSignalSpy navigateSpy(&history, &NavigationHistory::navigate);
        QSignalSpy nextEmptySpy(&history, &NavigationHistory::nextEmptyChanged);
        history.next();

        QCOMPARE(history.viewPath(), QStringList{"music"});
        QVERIFY(history.isNextEmpty());
        QVERIFY(!history.isPreviousEmpty());
        QCOMPARE(navigateSpy.size(), 1);
        QCOMPARE(nextEmptySpy.size(), 1);
        QCOMPARE(nextEmptySpy.takeFirst().at(0).toBool(), true);
    }

    void testNextKeepsProperties()
    {
        NavigationHistory history(&isNodeValid);
        history.update({"home"});
        history.push({"video", "list"}, QVariantMap{{"title", "my list"}});
        history.previous();
        history.next();

        QCOMPARE(history.viewPath(), (QStringList{"video", "list"}));
        QCOMPARE(history.viewProp()->value("title").toString(), QStringLiteral("my list"));
    }

    void testPushClearsForward()
    {
        NavigationHistory history(&isNodeValid);
        goBackOnce(history);

        QSignalSpy nextEmptySpy(&history, &NavigationHistory::nextEmptyChanged);
        history.push({"network"});

        QVERIFY(history.isNextEmpty());
        QCOMPARE(nextEmptySpy.size(), 1);

        QSignalSpy navigateSpy(&history, &NavigationHistory::navigate);
        history.next();
        QCOMPARE(history.viewPath(), QStringList{"network"});
        QCOMPARE(navigateSpy.size(), 0);

        history.previous();
        QCOMPARE(history.viewPath(), QStringList{"video"});
    }

    void testSkipInvalidBothWays()
    {
        {
            // backward: the invalid node in the back stack is skipped
            NavigationHistory history(&isNodeValid);
            history.update({"home"});
            history.push({"a"});
            history.push({"bad"}, goneNode());
            history.push({"c"});

            history.previous();
            QCOMPARE(history.viewPath(), QStringList{"a"});
            QVERIFY(!history.isNextEmpty());

            // forward: the invalid node was dropped, c comes right after a
            history.next();
            QCOMPARE(history.viewPath(), QStringList{"c"});
            QVERIFY(history.isNextEmpty());
        }
        {
            // forward: an invalid node in the forward stack is skipped
            NavigationHistory history(&isNodeValid);
            history.update({"home"});
            history.push({"bad"}, goneNode());
            history.previous();
            QCOMPARE(history.viewPath(), QStringList{"home"});

            QSignalSpy navigateSpy(&history, &NavigationHistory::navigate);
            history.next();
            QCOMPARE(history.viewPath(), QStringList{"home"});
            QCOMPARE(navigateSpy.size(), 0);
            QVERIFY(history.isNextEmpty());
        }
    }

    void testForwardCapped()
    {
        NavigationHistory history(&isNodeValid);
        history.update({"home"});
        for (int i = 0; i < 60; ++i)
            history.push({QString::number(i)});
        for (int i = 0; i < 60; ++i)
            history.previous();
        QCOMPARE(history.viewPath(), QStringList{"home"});

        int forwardCount = 0;
        while (!history.isNextEmpty())
        {
            history.next();
            ++forwardCount;
        }
        QCOMPARE(forwardCount, 50);
        QCOMPARE(history.viewPath(), QStringList{"49"});
    }
};

QTEST_GUILESS_MAIN(TestNavigationHistory)
#include "test_navigation_history.moc"
