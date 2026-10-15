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
}
