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
#ifndef NAVIGATION_HISTORY_NODE_HPP
#define NAVIGATION_HISTORY_NODE_HPP

class QQmlPropertyMap;

/**
 * @brief tell whether the properties of a history node can still be used
 *
 * a node becomes invalid when one of its properties refers to an entity
 * that is gone, for instance a network tree item that left its tree
 *
 * kept apart from NavigationHistory so that the history can be built and
 * tested without the network and media library models
 */
bool isNavigationNodeValid(const QQmlPropertyMap& properties);

#endif // NAVIGATION_HISTORY_NODE_HPP
