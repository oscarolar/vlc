/*****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
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

pragma Singleton

import QtQuick

QtObject {

    function clamp(num : real, min : real, max : real) : real {
      return num <= min ? min : num >= max ? max : num;
    }

    function isValidInstanceOf(object: var, type: var) : bool {
        return (!!object && (object instanceof type))
    }

    function transferFocus(item: Item, reason: int) {
        if (item.activeFocus && item.focusReason === reason)
            return

        if (item.setCurrentItemFocus)
            item.setCurrentItemFocus(reason)
        else
            item.forceActiveFocus(reason)
    }

    // NOTE: This allows us to force another 'reason' even when the item has activeFocus.
    function enforceFocus(item: Item, reason: int) {
        if (item.activeFocus && item.focusReason === reason)
            return

        item.focus = false;

        item.forceActiveFocus(reason);

        if (item.focusReason !== undefined)
            item.focusReason = reason // Control's focus reason sometimes is not adjusted with `forceActiveFocus()`, Qt bug?
    }

    function pointInRadius(x: real, y: real, radius: real) : bool {
         return (x * x + y * y < radius * radius)
    }

    // checks if point `pos` lies in rect `rect`
    function contains(rect : rect, pos : point) : bool {
        return (clamp(pos.x, rect.x, rect.x + rect.width) === pos.x)
                && (clamp(pos.y, rect.y, rect.y + rect.height) === pos.y)
    }

    function isInteger(data : var) : bool {
        return (typeof data === 'number' && (data % 1) === 0)
    }

    function compareFloat(a: real, b: real) : bool {
        return (Math.abs(a - b) < Number.EPSILON)
    }

    function alignUp(a : real, b : int) : int {
        return Math.ceil(a / b) * b
    }

    function alignDown(a : real, b : int) : int {
        return Math.floor(a / b) * b
    }

    // Currently only supports .25, .5, .75
    function denominatorForFloat(number : real) : int {
        const fraction = number % 1

        if (fraction === 0)
            return 1
        else if (compareFloat(fraction, 0.25))
            return 4
        else if (compareFloat(fraction, 0.50))
            return 2
        else if (compareFloat(fraction, 0.75))
            return 4
        else {
            console.error("Can not find denominator for number", number, "!")
            return 1
        }
    }

    function isSortedIntegerArrayConsecutive(array : var) : bool {
        for (let i = 1; i < array.length; ++i) {
            if ((array[i] - array[i - 1]) !== 1)
                return false
        }

        return true
    }

    function itemsMovable(sortedItemIndexes : var, targetIndex : int) : bool {
        return !isSortedIntegerArrayConsecutive(sortedItemIndexes) ||
                (targetIndex > (sortedItemIndexes[sortedItemIndexes.length - 1] + 1) ||
                 targetIndex < sortedItemIndexes[0])
    }

    /**
     * calculate content y for flickable such that item with given param will be fully visible
     * @param type:Flickable flickable
     * @param type:real y
     * @param type:real height
     * @param type:real topMargin
     * @param type:real bottomMargin
     * @return type:real appropriate contentY for flickable
     */
    function flickablePositionContaining(flickable : Flickable,
                                         y : real,
                                         height : real,
                                         topMargin : real,
                                         bottomMargin : real) : real {
        const itemTopY = flickable.originY + y
        const itemBottomY = itemTopY + height

        const viewTopY = flickable.contentY
        const viewBottomY = viewTopY + flickable.height

        let newContentY

        if (itemTopY < viewTopY)
             //item above view
            newContentY = itemTopY - topMargin
        else if (itemBottomY > viewBottomY)
             //item below view
            newContentY = itemBottomY + bottomMargin - flickable.height
        else
            newContentY = flickable.contentY

        return getFlickableBoundedContentY(flickable, newContentY)
    }

    function isArray(obj : var) : bool {
        return (obj?.length !== undefined) ?? false
    }

    // Similar to Item::contains(), but accepts area (rectangle):
    function itemIntersects(item: Item, area: rect) : bool {
        if (!item)
            return false
        if (area.width <= 0.0 || area.height <= 0.0)
            return false
        return !(((item.x < (area.x + area.width)) && ((item.x + item.width) > area.x)) &&
                 ((item.y < (area.y + area.height)) && ((item.y + item.height) > area.y)))
    }

    // Similar to `ItemView::positionViewAtIndex()` with `ItemView.Contain`,
    // but only the works in the y-axis for now.
    // The behavior is undefined if the item is not a visual child of the
    // content item. However, it is not required for the item to be a direct
    // child of the content item.
    function positionFlickableToContainItem(flickable: Flickable, item: Item) {
        console.assert(flickable)
        console.assert(item)
        console.assert(item.parent)
        const mappedRect = flickable.mapFromItem(item.parent, Qt.rect(item.x, item.y, item.width, item.height))
        // Flickable does not fully contain the item:
        if ((mappedRect.y < 0) || ((mappedRect.y + mappedRect.height) > flickable.height))
            flickable.contentY = getFlickableBoundedContentY(flickable,
                                                             flickable.contentItem.mapFromItem(item.parent, item.x, item.y).y -
                                                             Math.max(0, ((flickable.height - item.height) / 2)))
    }

    function getFlickableBoundedContentX(flickable: Flickable, contentX: real) : real {
        console.assert(flickable)
        return clamp(contentX,
                     estimateFlickableMinXExtent(flickable),
                     estimateFlickableMaxXExtent(flickable))
    }

    function getFlickableBoundedContentY(flickable: Flickable, contentY: real) : real {
        console.assert(flickable)
        return clamp(contentY,
                     estimateFlickableMinYExtent(flickable),
                     estimateFlickableMaxYExtent(flickable))
    }

    /// <flickable-extents>

    // FIXME: Get rid of this in favor of `QQuickFlickable::minXExtent()`:
    function estimateFlickableMinXExtent(flickable: Flickable) : real {
        console.assert(flickable)
        return (flickable.originX - flickable.leftMargin)
    }

    // FIXME: Get rid of this in favor of `QQuickFlickable::minYExtent()`:
    function estimateFlickableMinYExtent(flickable: Flickable) : real {
        console.assert(flickable)
        return (flickable.originY - flickable.topMargin)
    }

    // FIXME: Get rid of this in favor of `QQuickFlickable::maxXExtent()`:
    function estimateFlickableMaxXExtent(flickable: Flickable) : real {
        console.assert(flickable)
        return (flickable.originX + flickable.contentWidth - flickable.width + flickable.rightMargin)
    }

    // FIXME: Get rid of this in favor of `QQuickFlickable::maxYExtent()`:
    function estimateFlickableMaxYExtent(flickable: Flickable) : real {
        console.assert(flickable)
        return (flickable.originY + flickable.contentHeight - flickable.height + flickable.bottomMargin)
    }

    /// </flickable-extents>

    /// <address-line>

    // Splits the crumbs of an address line between the visible ones and the overflow
    // menu. Crumbs are kept visible from the last one while they fit in `availableWidth`,
    // the last crumb always is. `crumbWidth(index)` gives the width that crumb `index`
    // takes. The overflow list starts with the crumb nearest to the last one.
    function splitAddressCrumbs(crumbs : var, availableWidth : real, crumbWidth : var) : var {
        const visible = []
        const overflow = []
        let leftWidth = availableWidth
        let i = crumbs.length
        while (--i >= 0) {
            const width = crumbWidth(i)
            if (i < crumbs.length - 1 && width > leftWidth)
                overflow.push(crumbs[i])
            else
                visible.unshift(crumbs[i])
            leftWidth -= width
        }
        return { visible: visible, overflow: overflow }
    }

    // What activating an address line crumb does: null for the current location,
    // { tree } to browse a network tree, { uri } to navigate to a page.
    function addressCrumbAction(crumb : var, isCurrent : bool) : var {
        if (isCurrent || !crumb)
            return null
        if (crumb.tree !== undefined)
            return { tree: crumb.tree }
        if (crumb.uri !== undefined)
            return { uri: crumb.uri }
        return null
    }

    // Builds the address line crumbs of the page at `path`: one crumb per page of the path,
    // named from `names` (the segment itself when unknown), the last one named `title`
    // when it is not empty. A last page named like its parent (a default sub page) replaces
    // it. Each crumb is { display, uri }.
    function pathCrumbs(path : var, names : var, title : string) : var {
        const crumbs = []
        for (let i = 0; i < path.length; ++i) {
            const segment = path[i]
            let display = names[segment] ?? segment
            if (i === path.length - 1 && !!title)
                display = title
            crumbs.push({ display: display, uri: path.slice(0, i + 1) })
        }
        if (crumbs.length > 1 && crumbs[crumbs.length - 1].display === crumbs[crumbs.length - 2].display)
            crumbs.splice(crumbs.length - 2, 1)
        return crumbs
    }

    /// </address-line>
}
