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
#include "vlc_stub_modules.hpp"

#include <QTest>
#include <QApplication>
#include <QCheckBox>
#include <QGridLayout>
#include <QToolButton>

#include <memory>

#include "dialogs/preferences/prefs_merge.hpp"
#include "dialogs/preferences/preferences_widgets.hpp"
#include "dialogs/preferences/complete_preferences.hpp"
#include "dialogs/preferences/expert_model.hpp"

#include <vlc_configuration.h>
#include <vlc_modules.h>

namespace {

/* copy of the core option set, as the preferences views use it */
struct CoreConfig
{
    CoreConfig() { config = module_config_get( module_get_main(), &count ); }
    ~CoreConfig() { module_config_free( config ); }
    module_config_t *config = nullptr;
    unsigned count = 0;
};

QStringList itemNames( const QList<module_config_t *> &items )
{
    QStringList names;
    for( const module_config_t *item : items )
        if( item->i_type != CONFIG_SECTION )
            names << QString::fromUtf8( item->psz_name );
    return names;
}

QStringList sectionTitles( const QList<module_config_t *> &items )
{
    QStringList titles;
    for( const module_config_t *item : items )
        if( item->i_type == CONFIG_SECTION )
            titles << QString::fromUtf8( item->psz_text );
    return titles;
}

/* the options bound by the Subtitles simple panel (simple_preferences.cpp) */
const QSet<QString> subtitlesShown = {
    "osd", "video-title-show", "video-title-position", "spu",
    "subsdec-encoding", "sub-language", "freetype-rel-fontsize",
    "freetype-font", "freetype-color", "freetype-outline-thickness",
    "freetype-outline-color", "sub-margin", "secondary-sub-alignment",
    "secondary-sub-margin",
};

QCheckBox *checkBoxFor( QWidget *root, const char *option )
{
    const module_config_t *item = config_FindConfig( option );
    if( !item )
        return nullptr;
    const QString text = qfut( item->psz_text );
    for( QCheckBox *box : root->findChildren<QCheckBox *>() )
        if( box->text() == text )
            return box;
    return nullptr;
}

QToolButton *expander( QWidget *root )
{
    for( QToolButton *button : root->findChildren<QToolButton *>() )
        if( button->text() == qtr( "Advanced options" ) )
            return button;
    return nullptr;
}

}

class TestPrefsMerge : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        m_env = std::make_unique<VLCTestingEnv>();
        QVERIFY( m_env->init() );
        for( const char *name : touched )
            m_saved[name] = config_GetInt( name );
    }

    void cleanup()
    {
        for( auto it = m_saved.cbegin(); it != m_saved.cend(); ++it )
            config_PutInt( it.key(), it.value() );
    }

    void cleanupTestCase()
    {
        m_env.reset();
    }

    /* row 1 */
    void testSubtitlesRemaining()
    {
        CoreConfig core;
        const auto items = PrefsMerge::remainingItems( core.config, core.count,
                                                       SUBCAT_VIDEO_SUBPIC,
                                                       subtitlesShown );
        const QStringList names = itemNames( items );
        QVERIFY( names.contains( "sub-autodetect-fuzzy" ) );
        QVERIFY( names.contains( "sub-autodetect-path" ) );
        QVERIFY( names.contains( "sub-autodetect-file" ) );
        for( const QString &shown : subtitlesShown )
            QVERIFY2( !names.contains( shown ), qPrintable( shown ) );
    }

    /* row 2 */
    void testEmptyShownIsWholeSubcat()
    {
        CoreConfig core;
        QStringList expected;
        bool inside = false;
        for( unsigned i = 0; i < core.count; i++ )
        {
            const module_config_t *item = core.config + i;
            if( item->i_type == CONFIG_SUBCATEGORY )
            {
                if( inside )
                    break;
                inside = item->value.i == SUBCAT_VIDEO_GENERAL;
                continue;
            }
            if( inside && CONFIG_ITEM( item->i_type ) )
                expected << QString::fromUtf8( item->psz_name );
        }
        QVERIFY( expected.size() > 10 );

        const auto items = PrefsMerge::remainingItems( core.config, core.count,
                                                       SUBCAT_VIDEO_GENERAL, {} );
        QCOMPARE( itemNames( items ), expected );

        /* a subcat absent from the set gives nothing */
        QVERIFY( PrefsMerge::remainingItems( core.config, core.count,
                                             SUBCAT_SOUT_VOD, {} ).isEmpty() );
    }

    /* row 3 */
    void testEmptySectionsDropped()
    {
        CoreConfig core;
        const auto all = PrefsMerge::remainingItems( core.config, core.count,
                                                     SUBCAT_VIDEO_SUBPIC, {} );
        const QStringList allSections = sectionTitles( all );
        QCOMPARE( allSections, ( QStringList{ "On Screen Display", "Subtitles",
                                              "Overlays", "Multiple Subtitles" } ) );

        /* the Subtitles panel shows both "Multiple Subtitles" options */
        const auto items = PrefsMerge::remainingItems( core.config, core.count,
                                                       SUBCAT_VIDEO_SUBPIC,
                                                       subtitlesShown );
        QCOMPARE( sectionTitles( items ), ( QStringList{ "On Screen Display",
                                                         "Subtitles", "Overlays" } ) );
        /* no section is left without an option after it */
        for( int i = 0; i < items.size(); i++ )
            if( items[i]->i_type == CONFIG_SECTION )
                QVERIFY( i + 1 < items.size() && items[i + 1]->i_type != CONFIG_SECTION );
    }

    /* row 4 */
    void testPanelMappingDisjoint()
    {
        QSet<QString> seen;
        for( int panel = 0; panel < SPrefsMax; panel++ )
        {
            const auto sources = PrefsMerge::panelSubcats( panel );
            QVERIFY2( !sources.isEmpty(), qPrintable( QString::number( panel ) ) );
            for( const PrefsMerge::Source &source : sources )
            {
                const QString key = QString( "%1/%2" )
                        .arg( source.module ? source.module : "core" )
                        .arg( source.subcat );
                QVERIFY2( !seen.contains( key ), qPrintable( key ) );
                seen.insert( key );
            }
        }
        const auto hotkeys = PrefsMerge::panelSubcats( SPrefsHotkeys );
        QCOMPARE( hotkeys.size(), 1 );
        QVERIFY( hotkeys.first().module == nullptr );
        QCOMPARE( hotkeys.first().subcat, SUBCAT_INTERFACE_HOTKEYS );
    }

    void testHotkeysAdvancedIsJumpSizes()
    {
        /* the Hotkeys panel: the key table and the two wheel modes */
        QWidget root;
        QList<ConfigControl *> controls;
        controls << new KeySelectorControl( &root );
        controls << new IntegerListConfigControl( config_FindConfig( "hotkeys-y-wheel-mode" ), &root );
        controls << new IntegerListConfigControl( config_FindConfig( "hotkeys-x-wheel-mode" ), &root );

        /* the key table is only applied once changed */
        controls.first()->markClean();
        QVERIFY( !controls.first()->isModified() );

        const QSet<QString> shown = PrefsMerge::shownNames( controls );
        QVERIFY( shown.contains( "key-play-pause" ) );
        QVERIFY( shown.contains( "hotkeys-y-wheel-mode" ) );

        /* no key is repeated: only the jump sizes are left */
        CoreConfig core;
        const auto items = PrefsMerge::remainingItems( core.config, core.count,
                                                       SUBCAT_INTERFACE_HOTKEYS,
                                                       shown );
        QCOMPARE( sectionTitles( items ), QStringList{ "Jump sizes" } );
        QCOMPARE( itemNames( items ), ( QStringList{ "extrashort-jump-size",
                                                     "short-jump-size",
                                                     "medium-jump-size",
                                                     "long-jump-size" } ) );
        qDeleteAll( controls );
    }

    /* row 5 */
    void testAdvancedControlApplied()
    {
        const bool before = config_GetInt( "sub-autodetect-file" );

        AdvancedOptionsSection section( SPrefsSubtitles, subtitlesShown );
        QVERIFY( !section.isEmpty() );
        QToolButton *toggle = expander( &section );
        QVERIFY( toggle );
        /* collapsed by default */
        QVERIFY( !toggle->isChecked() );
        QVERIFY( !checkBoxFor( &section, "sub-autodetect-file" ) );

        toggle->click();
        QCheckBox *box = checkBoxFor( &section, "sub-autodetect-file" );
        QVERIFY( box );
        QCOMPARE( box->isChecked(), before );
        /* a basic option is not repeated */
        QVERIFY( !checkBoxFor( &section, "spu" ) );

        box->setChecked( !before );
        section.apply();
        QCOMPARE( bool( config_GetInt( "sub-autodetect-file" ) ), !before );
    }

    /* row 6: the dialog applies the simple panels, then the All tree, then
     * the Expert table; a control the user did not touch must not write back
     * the value it was built with */
    void testSaveAppliesAllBuiltViews()
    {
        const bool autodetect = config_GetInt( "sub-autodetect-file" );
        const bool fill = config_GetInt( "spu-fill" );

        /* both views are built before any change */
        AdvancedOptionsSection section( SPrefsSubtitles, subtitlesShown );
        QToolButton *toggle = expander( &section );
        QVERIFY( toggle );
        toggle->click();

        PrefsTreeItem node( PrefsTreeItem::SUBCATEGORY_NODE );
        node.subcat_id = SUBCAT_VIDEO_SUBPIC;
        node.p_module = module_get_main();
        AdvPrefsPanel panel( m_env->intf, nullptr, &node );

        size_t count;
        module_t **list = module_list_get( &count );
        ExpertPrefsTableModel expert( list, count );

        /* change in the Advanced group, change in the All view */
        QCheckBox *inSection = checkBoxFor( &section, "sub-autodetect-file" );
        QCheckBox *inPanel = checkBoxFor( &panel, "spu-fill" );
        QVERIFY( inSection && inPanel );
        inSection->setChecked( !autodetect );
        inPanel->setChecked( !fill );

        section.apply();
        panel.apply();
        expert.submit();
        module_list_free( list );

        QCOMPARE( bool( config_GetInt( "sub-autodetect-file" ) ), !autodetect );
        QCOMPARE( bool( config_GetInt( "spu-fill" ) ), !fill );
    }

    void testExpertSubmitsEditedOnly()
    {
        const bool fill = config_GetInt( "spu-fill" );
        size_t count;
        module_t **list = module_list_get( &count );
        ExpertPrefsTableModel expert( list, count );

        int row = -1;
        for( int i = 0; i < expert.rowCount(); i++ )
            if( !strcmp( expert.itemAt( i )->getConfig()->psz_name, "spu-fill" ) )
                row = i;
        QVERIFY( row >= 0 );

        /* changed elsewhere after the table was built */
        config_PutInt( "spu-fill", !fill );
        expert.submit();
        QCOMPARE( bool( config_GetInt( "spu-fill" ) ), !fill );

        /* edited in the table: the copy goes from fill to !fill */
        expert.toggleBoolean( expert.index( row, 0 ) );
        config_PutInt( "spu-fill", fill );
        expert.submit();
        QCOMPARE( bool( config_GetInt( "spu-fill" ) ), !fill );
        module_list_free( list );
    }

private:
    static constexpr const char *touched[] = { "sub-autodetect-file", "spu-fill" };
    std::unique_ptr<VLCTestingEnv> m_env;
    QHash<const char *, int64_t> m_saved;
};

int main( int argc, char **argv )
{
    /* run offscreen as the CI doesn't have a desktop environment */
    qputenv( "QT_QPA_PLATFORM", "offscreen" );
    QApplication app( argc, argv );
    TestPrefsMerge test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec( &test, argc, argv );
}

#include "test_prefs_merge.moc"
