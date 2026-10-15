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
import QtQuick
import QtTest
import VLC.Util

// NetworkAddressbar needs MainCtx and VLCStyle, which the QML test runner does not
// provide: these tests cover the address line logic it delegates to Helpers.
TestCase {
    name: "NetworkAddressbar"

    function makeCrumbs(count) {
        const crumbs = []
        for (let i = 0; i < count; ++i)
            crumbs.push({ display: "crumb" + i, tree: "tree" + i })
        return crumbs
    }

    function displays(crumbs) {
        return crumbs.map((crumb) => crumb.display)
    }

    function test_overflow() {
        const crumbs = makeCrumbs(5)

        // every crumb is 100 wide, there is room for 2 of them
        let split = Helpers.splitAddressCrumbs(crumbs, 200, (index) => 100)
        compare(displays(split.visible), ["crumb3", "crumb4"])
        compare(displays(split.overflow), ["crumb2", "crumb1", "crumb0"])

        // the last crumb stays visible even without room
        split = Helpers.splitAddressCrumbs(crumbs, 0, (index) => 100)
        compare(displays(split.visible), ["crumb4"])
        compare(split.overflow.length, 4)

        // everything fits
        split = Helpers.splitAddressCrumbs(crumbs, 1000, (index) => 100)
        compare(split.visible.length, 5)
        compare(split.overflow.length, 0)

        split = Helpers.splitAddressCrumbs([], 1000, (index) => 100)
        compare(split.visible.length, 0)
        compare(split.overflow.length, 0)
    }

    function test_click() {
        const crumbs = makeCrumbs(3)

        // browsing crumbs carry a network tree
        compare(Helpers.addressCrumbAction(crumbs[0], false), { tree: "tree0" })
        // the last crumb is where we are, it does nothing
        compare(Helpers.addressCrumbAction(crumbs[2], true), null)

        // library crumbs carry a page uri
        const page = { display: "Music", uri: ["music"] }
        compare(Helpers.addressCrumbAction(page, false), { uri: ["music"] })
        compare(Helpers.addressCrumbAction(page, true), null)
    }

    function test_pathCrumbs() {
        const names = { "music": "Music", "albums": "Albums" }

        let crumbs = Helpers.pathCrumbs(["music", "albums", "list"], names, "My album")
        compare(displays(crumbs), ["Music", "Albums", "My album"])
        compare(crumbs[0].uri, ["music"])
        compare(crumbs[1].uri, ["music", "albums"])
        compare(crumbs[2].uri, ["music", "albums", "list"])

        // without a title, the last crumb is named after its segment
        crumbs = Helpers.pathCrumbs(["music", "albums"], names, "")
        compare(displays(crumbs), ["Music", "Albums"])

        // unknown segments are shown as is
        crumbs = Helpers.pathCrumbs(["other", "albums"], names, "")
        compare(displays(crumbs), ["other", "Albums"])

        // a default sub page named like its parent page shows once
        crumbs = Helpers.pathCrumbs(["music", "albums", "all"], names, "Albums")
        compare(displays(crumbs), ["Music", "Albums"])
        compare(crumbs[1].uri, ["music", "albums", "all"])
    }
}
