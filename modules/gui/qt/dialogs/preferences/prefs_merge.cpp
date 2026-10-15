/*****************************************************************************
 * prefs_merge.cpp : advanced options listed under the simple panels
 *****************************************************************************
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

#include "dialogs/preferences/prefs_merge.hpp"
#include "dialogs/preferences/preferences_widgets.hpp"

#include <QGridLayout>
#include <QGroupBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <vlc_modules.h>

QList<PrefsMerge::Source> PrefsMerge::panelSubcats( int panel )
{
    switch( panel )
    {
        case SPrefsInterface:
            return { { nullptr, SUBCAT_INTERFACE_GENERAL },
                     { "qt", SUBCAT_UNKNOWN } };
        case SPrefsAudio:
            return { { nullptr, SUBCAT_AUDIO_GENERAL } };
        case SPrefsVideo:
            return { { nullptr, SUBCAT_VIDEO_GENERAL } };
        case SPrefsSubtitles:
            return { { nullptr, SUBCAT_VIDEO_SUBPIC } };
        case SPrefsInputAndCodecs:
            return { { nullptr, SUBCAT_INPUT_GENERAL } };
        case SPrefsHotkeys:
            return { { nullptr, SUBCAT_INTERFACE_HOTKEYS } };
        case SPrefsMediaLibrary:
            return { { "medialibrary", SUBCAT_UNKNOWN } };
        default:
            return {};
    }
}

const QSet<QString> &PrefsMerge::handledByPanels()
{
    /* see SPrefsPanel::apply() */
    static const QSet<QString> names = {
        /* Input & Codecs: disc device and caching presets */
        "dvd", "vcd", "cd-audio",
        "file-caching", "network-caching", "disc-caching", "live-caching",
        /* Interface: skins layout */
        "intf",
        /* Video: fullscreen screen */
        "qt-fullscreen-screennumber",
        /* Audio: normalizer and default volume */
        "audio-filter", "volume-save",
        "alsa-gain", "jack-gain", "auhal-volume", "kai-gain",
        "mmdevice-volume", "waveout-volume",
        /* Subtitles: shadow and background */
        "freetype-shadow-opacity", "freetype-background-opacity",
    };
    return names;
}

QSet<QString> PrefsMerge::shownNames( const QList<ConfigControl *> &controls )
{
    QSet<QString> names;
    for( const ConfigControl *control : controls )
    {
        if( !qobject_cast<const KeySelectorControl *>( control ) )
        {
            names.insert( QString::fromUtf8( control->getName() ) );
            continue;
        }

        /* the hotkey table edits every key option of the core */
        unsigned count;
        module_config_t *config = module_config_get( module_get_main(), &count );
        for( unsigned i = 0; i < count; i++ )
            if( config[i].i_type == CONFIG_ITEM_KEY )
                names.insert( QString::fromUtf8( config[i].psz_name ) );
        module_config_free( config );
    }
    return names;
}

QList<module_config_t *> PrefsMerge::remainingItems( module_config_t *config,
                                                     size_t count,
                                                     enum vlc_config_subcat subcat,
                                                     const QSet<QString> &shown )
{
    QList<module_config_t *> items;
    if( !config )
        return items;

    module_config_t *p_item = config, *p_end = config + count;
    const bool whole_set = subcat == SUBCAT_UNKNOWN;

    if( !whole_set )
    {
        /* Skip to the relevant part of the core config */
        while( p_item < p_end && !( p_item->i_type == CONFIG_SUBCATEGORY &&
                                    p_item->value.i == subcat ) )
            p_item++;
        if( p_item == p_end )
            return items;
        p_item++; // Skip past the subcat entry itself
    }

    bool has_hotkey = false;
    bool section_used = true;
    for( ; p_item < p_end; ++p_item )
    {
        if( p_item->i_type == CONFIG_SUBCATEGORY )
        {
            /* a plugin's options are grouped together, a core subcat ends */
            if( whole_set )
                continue;
            break;
        }

        if( p_item->i_type == CONFIG_SECTION )
        {
            if( !section_used )
                items.removeLast();
            items.append( p_item );
            section_used = false;
            continue;
        }

        if( !CONFIG_ITEM( p_item->i_type ) )
            continue;
        if( p_item->psz_name && shown.contains( QString::fromUtf8( p_item->psz_name ) ) )
            continue;

        /* Only one hotkey control */
        if( p_item->i_type == CONFIG_ITEM_KEY )
        {
            if( has_hotkey )
                continue;
            has_hotkey = true;
        }

        items.append( p_item );
        section_used = true;
    }

    if( !section_used )
        items.removeLast();
    return items;
}

QList<ConfigControl *> PrefsMerge::createControls( const QList<module_config_t *> &items,
                                                   QWidget *parent, QGridLayout *layout,
                                                   int &line, const QString &leadTitle )
{
    QList<ConfigControl *> controls;
    QGroupBox *box = nullptr;
    QGridLayout *boxlayout = nullptr;
    int boxline = 0;

    const auto closeBox = [&]() {
        if( !box )
            return;
        if( boxline > 0 )
        {
            box->setLayout( boxlayout );
            box->show();
            layout->addWidget( box, line, 0, 1, -1 );
            line++;
        }
        else
        {
            delete boxlayout;
            delete box;
        }
        box = nullptr;
        boxlayout = nullptr;
        boxline = 0;
    };
    const auto openBox = [&]( const QString &title ) {
        closeBox();
        box = new QGroupBox( title, parent );
        box->hide();
        boxlayout = new QGridLayout();
    };

    if( !leadTitle.isEmpty() && !items.isEmpty()
     && items.first()->i_type != CONFIG_SECTION )
        openBox( leadTitle );

    for( module_config_t *p_item : items )
    {
        if( p_item->i_type == CONFIG_SECTION )
        {
            openBox( qfut( p_item->psz_text ) );
            continue;
        }

        ConfigControl *control;
        if( !box )
            control = ConfigControl::createControl( p_item, parent,
                                                    layout, line );
        else
            control = ConfigControl::createControl( p_item, parent,
                                                    boxlayout, boxline );
        if( !control )
            continue;

        if( box ) boxline++;
        else line++;
        controls.append( control );
    }
    closeBox();

    return controls;
}

void PrefsMerge::applyChanged( const QList<ConfigControl *> &controls )
{
    for( ConfigControl *control : controls )
        if( control->isModified() )
            control->doApply();
}

/*********************************************************************
 * The "Advanced options" section of a simple panel
 *********************************************************************/
AdvancedOptionsSection::AdvancedOptionsSection( int panel, const QSet<QString> &shownNames,
                                                QWidget *parent )
    : QWidget( parent )
{
    QSet<QString> shown = shownNames;
    shown.unite( PrefsMerge::handledByPanels() );

    for( const PrefsMerge::Source &source : PrefsMerge::panelSubcats( panel ) )
    {
        module_t *module = source.module ? module_find( source.module )
                                         : module_get_main();
        if( !module )
            continue;

        unsigned count;
        module_config_t *config = module_config_get( module, &count );
        const auto items = PrefsMerge::remainingItems( config, count,
                                                       source.subcat, shown );
        if( items.isEmpty() )
        {
            module_config_free( config );
            continue;
        }
        /* a plugin's options get its name as heading */
        const QString title = source.module ? qfut( module_GetLongName( module ) )
                                            : QString();
        blocks.append( { config, title, items } );
    }

    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );

    toggle = new QToolButton( this );
    toggle->setText( qtr( "Advanced options" ) );
    toggle->setToolTip( qtr( "Show the other options of this category" ) );
    toggle->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    toggle->setArrowType( Qt::RightArrow );
    toggle->setCheckable( true );
    toggle->setAutoRaise( true );
    toggle->setFocusPolicy( Qt::StrongFocus );
    layout->addWidget( toggle, 0, Qt::AlignLeft );

    content = new QWidget( this );
    content->hide();
    layout->addWidget( content );

    connect( toggle, &QToolButton::toggled, this, &AdvancedOptionsSection::setExpanded );
}

AdvancedOptionsSection::~AdvancedOptionsSection()
{
    qDeleteAll( controls );
    controls.clear();
    for( const Block &block : blocks )
        module_config_free( block.config );
}

void AdvancedOptionsSection::setExpanded( bool expanded )
{
    if( expanded && !built )
        build();
    toggle->setArrowType( expanded ? Qt::DownArrow : Qt::RightArrow );
    content->setVisible( expanded );
}

void AdvancedOptionsSection::build()
{
    built = true;
    QGridLayout *grid = new QGridLayout( content );
    grid->setContentsMargins( 0, 0, 0, 0 );
    int line = 0;
    for( const Block &block : blocks )
        controls += PrefsMerge::createControls( block.items, content, grid,
                                                line, block.title );
}

void AdvancedOptionsSection::apply()
{
    PrefsMerge::applyChanged( controls );
}
