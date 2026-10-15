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
#include "navigation_history_node.hpp"
#include <cassert>
#include <QtQml/QQmlPropertyMap>
#include "network/networkmediamodel.hpp"
#include "medialibrary/mlqmltypes.hpp"

static bool isNodeValid(const QVariant& value)
{
    if (value.canConvert<QStringList>()
        || value.canConvert<QString>()
        || value.canConvert<unsigned int>()
        || value.canConvert<int>()
        || value.canConvert<bool>()
        || value.canConvert<MLItemId>())
    {
        return true;
    }
    else if ( value.canConvert<QVariantList>() )
    {
        QVariantList valueList = value.toList();
        for (QVariant& v : valueList)
        {
            if (!isNodeValid(v))
                return false;
        }
        return true;
    }
    else if (value.canConvert<NetworkTreeItem>() )
    {
        NetworkTreeItem item = value.value<NetworkTreeItem>();
        if ( ! item.isValid() )
            return false;

        return true;
    }
    else if ( value.canConvert<QVariantMap>() )
    {
        QVariantMap valueList = value.toMap();
        for (QVariant& v : valueList.values())
        {
            if (!isNodeValid(v))
                return false;
        }
        return true;
    }
    else if ( value.isNull() )
        return true;

    assert(false);
    return false;
}

bool isNavigationNodeValid(const QQmlPropertyMap& value)
{
    for (auto it: value.keys())
    {
        if (!isNodeValid(value[it]))
            return false;
    }
    return true;
}
