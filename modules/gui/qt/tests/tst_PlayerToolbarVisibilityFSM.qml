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

TestCase {
    id: root
    name: "PlayerToolbarVisibilityFSM"

    readonly property int hideTimeout: 50

    property var fsm: null

    function createFSM(props) {
        const component = Qt.createComponent("../player/qml/PlayerToolbarVisibilityFSM.qml")
        verify(component.status === Component.Ready, component.errorString())

        const obj = component.createObject(root, Object.assign({
            isInteractive: false,
            hasEmbededVideo: true,
            hideTimeout: root.hideTimeout
        }, props))
        verify(obj !== null, "unable to create the FSM")
        tryVerify(() => obj.started)
        return obj
    }

    function cleanup() {
        if (fsm) {
            fsm.destroy()
            fsm = null
        }
    }

    function test_anywhere_moveShowsThenTimesOut() {
        fsm = createFSM({})

        // starts visible, hides after the timeout
        verify(fsm.isVisible)
        tryCompare(fsm, "isVisible", false, hideTimeout * 20)

        fsm.mouseMove()
        verify(fsm.isVisible)
        tryCompare(fsm, "isVisible", false, hideTimeout * 20)
    }

    function test_lockCounting() {
        fsm = createFSM({})
        verify(fsm.isVisible)

        fsm.lock()
        fsm.lock()
        fsm.unlock()
        wait(hideTimeout * 3)
        verify(fsm.isVisible)

        fsm.unlock()
        verify(fsm.isVisible)
        tryCompare(fsm, "isVisible", false, hideTimeout * 20)
    }

    function test_interactiveIgnoresMouse() {
        fsm = createFSM({ isInteractive: true })
        verify(!fsm.isVisible)

        fsm.mouseMove()
        verify(!fsm.isVisible)
        wait(hideTimeout * 2)
        verify(!fsm.isVisible)
    }

    function test_edgeZone() {
        compare(Helpers.edgeZone(10, 1000, 80, 80), Helpers.Edge.Top)
        compare(Helpers.edgeZone(500, 1000, 80, 80), Helpers.Edge.None)
        compare(Helpers.edgeZone(990, 1000, 80, 80), Helpers.Edge.Bottom)
        // zones are inclusive
        compare(Helpers.edgeZone(80, 1000, 80, 80), Helpers.Edge.Top)
        compare(Helpers.edgeZone(920, 1000, 80, 80), Helpers.Edge.Bottom)
        // the bottom zone may be deeper, to cover the control bar
        compare(Helpers.edgeZone(850, 1000, 80, 150), Helpers.Edge.Bottom)
        compare(Helpers.edgeZone(849, 1000, 80, 150), Helpers.Edge.None)
    }

    // returns a hidden FSM in edges mode
    function createHiddenEdgesFSM() {
        const obj = createFSM({})
        obj.revealMode = obj.revealEdges
        tryCompare(obj, "isVisible", false, hideTimeout * 20)
        return obj
    }

    function test_edges_middleDoesNotReveal() {
        fsm = createHiddenEdgesFSM()

        fsm.mouseMove()
        fsm.pointerZone(Helpers.Edge.None)
        verify(!fsm.isVisible)
    }

    function test_edges_bottomReveals() {
        fsm = createHiddenEdgesFSM()

        fsm.pointerZone(Helpers.Edge.Bottom)
        verify(fsm.isVisible)
        compare(fsm.revealedEdge, Helpers.Edge.Bottom)
        verify(fsm.isBottomVisible)
        verify(!fsm.isTopVisible)

        // stays while the pointer is within the zone
        wait(hideTimeout * 3)
        verify(fsm.isBottomVisible)

        // moving to the other edge swaps the bars
        fsm.pointerZone(Helpers.Edge.Top)
        verify(fsm.isTopVisible)
        verify(!fsm.isBottomVisible)
    }

    function test_edges_leaveHidesImmediately() {
        fsm = createHiddenEdgesFSM()

        fsm.pointerZone(Helpers.Edge.Bottom)
        verify(fsm.isVisible)

        fsm.pointerZone(Helpers.Edge.None)
        verify(!fsm.isVisible)
    }

    function test_edges_lockKeepsVisible() {
        fsm = createHiddenEdgesFSM()

        fsm.pointerZone(Helpers.Edge.Bottom)
        fsm.lock()
        fsm.pointerZone(Helpers.Edge.None)
        verify(fsm.isBottomVisible)
        wait(hideTimeout * 3)
        verify(fsm.isBottomVisible)

        fsm.unlock()
        verify(!fsm.isVisible)
    }

    function test_edges_keyboardShowsBoth() {
        fsm = createHiddenEdgesFSM()

        fsm.keyboardMove()
        verify(fsm.isTopVisible)
        verify(fsm.isBottomVisible)

        // a pointer move outside the zones does not hide them
        fsm.pointerZone(Helpers.Edge.None)
        verify(fsm.isVisible)
        tryCompare(fsm, "isVisible", false, hideTimeout * 40)
    }

    function test_anywhere_ignoresPointerZone() {
        fsm = createFSM({})
        tryCompare(fsm, "isVisible", false, hideTimeout * 20)

        fsm.pointerZone(Helpers.Edge.Bottom)
        verify(!fsm.isVisible)

        fsm.mouseMove()
        verify(fsm.isTopVisible)
        verify(fsm.isBottomVisible)
    }
}
