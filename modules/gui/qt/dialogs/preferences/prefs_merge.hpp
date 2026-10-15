/*****************************************************************************
 * prefs_merge.hpp : advanced options listed under the simple panels
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

#ifndef VLC_QT_PREFS_MERGE_HPP_
#define VLC_QT_PREFS_MERGE_HPP_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>

#include <QList>
#include <QSet>
#include <QString>
#include <QWidget>

class ConfigControl;
class QGridLayout;
class QToolButton;

/* The simple preferences panels */
enum {
    SPrefsInterface = 0,
    SPrefsAudio,
    SPrefsVideo,
    SPrefsSubtitles,
    SPrefsInputAndCodecs,
    SPrefsHotkeys,
    SPrefsMediaLibrary,
    SPrefsMax
};
#define SPrefsDefaultCat SPrefsInterface

namespace PrefsMerge
{

/** A part of the option tree that a simple panel stands for */
struct Source
{
    const char *module;            /**< plugin name, nullptr for the core */
    enum vlc_config_subcat subcat; /**< SUBCAT_UNKNOWN for the whole plugin */
};

/** The core subcats and plugins whose options each simple panel shows */
QList<Source> panelSubcats( int panel );

/** Options written by the simple panels' own code (caching presets, volume,
 * ...) rather than by a ConfigControl; never listed as advanced options */
const QSet<QString> &handledByPanels();

/** Option names shown by \p controls; the hotkey table shows every key */
QSet<QString> shownNames( const QList<ConfigControl *> &controls );

/**
 * Items of \p subcat in the option set \p config (the whole set, subcat
 * markers skipped, when \p subcat is SUBCAT_UNKNOWN), in config order,
 * CONFIG_SECTION markers included, minus the options named in \p shown.
 * Sections left with no option are dropped and only the first hotkey item
 * is kept (the hotkey table edits them all).
 */
QList<module_config_t *> remainingItems( module_config_t *config, size_t count,
                                         enum vlc_config_subcat subcat,
                                         const QSet<QString> &shown );

/**
 * Creates one control per item into \p layout from row \p line, a group box
 * per CONFIG_SECTION. Items before the first section go in a group box
 * titled \p leadTitle when it is not empty.
 */
QList<ConfigControl *> createControls( const QList<module_config_t *> &items,
                                       QWidget *parent, QGridLayout *layout,
                                       int &line,
                                       const QString &leadTitle = QString() );

/** Applies the controls whose value the user changed */
void applyChanged( const QList<ConfigControl *> &controls );

}

/**
 * The collapsed "Advanced options" part at the bottom of a simple panel:
 * the options of the panel's subcats it does not already show, with the
 * widgets of the All view. Controls are created on first expansion.
 */
class AdvancedOptionsSection : public QWidget
{
    Q_OBJECT
public:
    AdvancedOptionsSection( int panel, const QSet<QString> &shown,
                            QWidget *parent = nullptr );
    virtual ~AdvancedOptionsSection();

    bool isEmpty() const { return blocks.isEmpty(); }
    void apply();

private:
    void setExpanded( bool );
    void build();

    struct Block
    {
        module_config_t *config;
        QString title;
        QList<module_config_t *> items;
    };
    QList<Block> blocks;
    QList<ConfigControl *> controls;
    QToolButton *toggle;
    QWidget *content;
    bool built = false;
};

#endif
