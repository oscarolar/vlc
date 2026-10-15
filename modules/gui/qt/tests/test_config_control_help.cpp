/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <QTest>
#include <QApplication>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QToolButton>

#include <memory>

#include "qt.hpp"
#include "dialogs/preferences/preferences_widgets.hpp"

#include <vlc_plugin.h>

class TestConfigControlHelp : public QObject
{
    Q_OBJECT

private:
    static module_config_t makeItem( uint8_t type, const char *name,
                                     const char *longtext )
    {
        module_config_t item{};
        item.i_type = type;
        item.psz_name = name;
        item.psz_text = "Option text";
        item.psz_longtext = longtext;
        item.value.i = 1;
        item.orig.i = 1;
        item.min.i = 0;
        item.max.i = 100;
        return item;
    }

    static QToolButton *infoButton( QWidget *root )
    {
        const auto buttons = root->findChildren<QToolButton *>();
        return buttons.size() == 1 ? buttons.first() : nullptr;
    }

    /* the info button must sit in the same row widget as the anchor,
     * placed after it */
    static bool isAfter( QWidget *anchor, QWidget *button )
    {
        QWidget *row = button->parentWidget();
        if( !row || anchor->parentWidget() != row || !row->layout() )
            return false;
        return row->layout()->indexOf( button ) > row->layout()->indexOf( anchor );
    }

private slots:
    void testInfoButton()
    {
        QWidget root;
        QGridLayout *grid = new QGridLayout( &root );
        module_config_t item = makeItem( CONFIG_ITEM_BOOL, "test-bool",
                                         "Bool long help" );
        std::unique_ptr<ConfigControl> control(
                ConfigControl::createControl( &item, &root, grid, 0 ) );
        QVERIFY( control );

        QCheckBox *check = root.findChild<QCheckBox *>();
        QVERIFY( check );
        QToolButton *info = infoButton( &root );
        QVERIFY( info );
        QCOMPARE( info->accessibleName(), qtr( "More information" ) );
        QVERIFY( info->toolTip().contains( "Bool long help" ) );
        QVERIFY( info->whatsThis().contains( "Bool long help" ) );
        QVERIFY( info->focusPolicy() & Qt::TabFocus );
        QVERIFY( isAfter( check, info ) );
        /* the tooltip stays for mouse users */
        QVERIFY( check->toolTip().contains( "Bool long help" ) );
    }

    void testInfoButtonAfterLabel()
    {
        QWidget root;
        QGridLayout *grid = new QGridLayout( &root );
        module_config_t item = makeItem( CONFIG_ITEM_INTEGER, "test-int",
                                         "Integer long help" );
        std::unique_ptr<ConfigControl> control(
                ConfigControl::createControl( &item, &root, grid, 0 ) );
        QVERIFY( control );

        QLabel *label = root.findChild<QLabel *>();
        QVERIFY( label );
        QCOMPARE( label->text(), QString( "Option text" ) );
        QToolButton *info = infoButton( &root );
        QVERIFY( info );
        QVERIFY( isAfter( label, info ) );
        QVERIFY( info->whatsThis().contains( "Integer long help" ) );
    }

    void testInfoButtonOnExistingForm()
    {
        /* simple panels bind controls to widgets already laid out by a .ui */
        QWidget root;
        QGridLayout *grid = new QGridLayout( &root );
        QLabel *label = new QLabel( "Form label" );
        QSpinBox *spin = new QSpinBox;
        grid->addWidget( label, 0, 0 );
        grid->addWidget( spin, 0, 1 );

        module_config_t item = makeItem( CONFIG_ITEM_INTEGER, "test-form",
                                         "Form long help" );
        std::unique_ptr<ConfigControl> control(
                new IntegerConfigControl( &item, label, spin ) );

        QToolButton *info = infoButton( &root );
        QVERIFY( info );
        QVERIFY( isAfter( label, info ) );
        QVERIFY( info->whatsThis().contains( "Form long help" ) );
    }

    void testNoInfoButtonWithoutLongtext()
    {
        QWidget root;
        QGridLayout *grid = new QGridLayout( &root );
        module_config_t item = makeItem( CONFIG_ITEM_BOOL, "test-nohelp", nullptr );
        std::unique_ptr<ConfigControl> control(
                ConfigControl::createControl( &item, &root, grid, 0 ) );
        QVERIFY( control );
        QVERIFY( root.findChildren<QToolButton *>().isEmpty() );
    }

    void testInfoButtonFollowsVisibility()
    {
        QWidget root;
        QGridLayout *grid = new QGridLayout( &root );
        module_config_t item = makeItem( CONFIG_ITEM_INTEGER, "test-vis",
                                         "Visibility long help" );
        std::unique_ptr<ConfigControl> control(
                ConfigControl::createControl( &item, &root, grid, 0 ) );
        QVERIFY( control );
        QToolButton *info = infoButton( &root );
        QVERIFY( info );

        /* hidden before the dialog is shown, as panels do in their ctor */
        control->hide();
        root.show();
        QVERIFY( QTest::qWaitForWindowExposed( &root ) );
        QVERIFY( !info->isVisible() );

        /* no empty row is left behind */
        QVERIFY( !info->parentWidget()->isVisible() );

        control->show();
        QVERIFY( info->isVisible() );
        control->hide();
        QVERIFY( !info->isVisible() );
        QVERIFY( !info->parentWidget()->isVisible() );
    }
};

int main( int argc, char **argv )
{
    /* run offscreen as the CI doesn't have a desktop environment */
    qputenv( "QT_QPA_PLATFORM", "offscreen" );
    QApplication app( argc, argv );
    TestConfigControlHelp test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec( &test, argc, argv );
}

#include "test_config_control_help.moc"
