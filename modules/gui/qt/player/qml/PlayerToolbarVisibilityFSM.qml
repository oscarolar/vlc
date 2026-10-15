/*****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
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

import VLC.Util

/**
 * player toolbars visibility state machine
 *
 * @startuml
 * state Visible {
 *    state NormalVisible {
 *        state AudioMode {
 *        }
 *       state VideoMode {
 *       } 
 *    }
 *    state Locked {
 *    }
 * }
 * state Hidden {
 *    state NormalHidden {
 *    }
 * }
 * @enduml
 *
 */

FSM {
    id: fsm

    //incoming signals
    signal askShow()
    signal mouseMove()
    signal keyboardMove()
    signal lock()
    signal unlock()
    signal forceUnlock()
    signal timeout()
    signal updateVideoEmbed()
    // zone (Helpers.Edge) of the pointer, emitted on every pointer move
    signal pointerZone(int zone)

    //exposed internal states
    property alias isVisible: fsmVisible.active

    // edges (Helpers.Edge) whose bar is revealed while visible
    property int revealedEdge: Helpers.Edge.Both

    readonly property bool isTopVisible: isVisible && ((revealedEdge & Helpers.Edge.Top) !== 0)
    readonly property bool isBottomVisible: isVisible && ((revealedEdge & Helpers.Edge.Bottom) !== 0)

    // values of revealMode, match MainCtx.ControlsReveal
    readonly property int revealAnywhere: 0
    readonly property int revealEdges: 1

    //dependencies, bound by the user of the state machine
    property bool isInteractive: false
    property bool hasEmbededVideo: false
    property int hideTimeout: 1000
    // revealAnywhere: any pointer move reveals both bars until the timeout
    // revealEdges: a bar is revealed while the pointer is within its edge zone
    property int revealMode: revealAnywhere

    property int lockCount: 0 // Track the number of locks
    property int timeoutDuration: hideTimeout
    property int lastPointerZone: Helpers.Edge.None

    // bars revealed by the pointer in edges mode, they hide when it leaves
    readonly property bool _revealedByPointer: revealMode === revealEdges
                                               && revealedEdge !== Helpers.Edge.Both

    initialState: ((fsm.isInteractive && fsm.hasEmbededVideo))
                  ? fsmHidden : fsmVisible

    signalMap: ({
        askShow: fsm.askShow,
        mouseMove: fsm.mouseMove,
        keyboardMove: fsm.keyboardMove,
        lock: fsm.lock,
        timeout: fsm.timeout,
        unlock: fsm.unlock,
        updateVideoEmbed: fsm.updateVideoEmbed,
        pointerZone: fsm.pointerZone,
    })

    onPointerZone: (zone) => { fsm.lastPointerZone = zone }

    function _canRevealEdge(zone : int) : bool {
        return fsm.revealMode === fsm.revealEdges
            && !fsm.isInteractive
            && zone !== Helpers.Edge.None
    }

    FSMState {
        id: fsmVisible

        initialState: fsmNormalVisible

        FSMState {
            id: fsmNormalVisible

            initialState: fsm.hasEmbededVideo ? fsmVideoMode : fsmAudioMode

            transitions: ({
                lock: {
                    action: () => { fsm.lockCount++; },
                    target: fsmLocked
                }
            })

            FSMState {
                id: fsmAudioMode

                function enter() {
                    fsm.revealedEdge = Helpers.Edge.Both
                }

                transitions: ({
                    updateVideoEmbed: {
                        guard: () => fsm.hasEmbededVideo,
                        target: fsmVideoMode
                    }
                })
            }

            FSMState {
                id: fsmVideoMode

                function enter() {
                    timeout.interval = fsm.timeoutDuration;

                    if (!fsm.isInteractive && !fsm._revealedByPointer)
                        timeout.restart();
                }

                function exit() {
                  timeout.stop();
                }

                transitions: ({
                    updateVideoEmbed: {
                        guard: () => !fsm.hasEmbededVideo,
                        target: fsmAudioMode
                    },
                    askShow: {
                        target: fsmHidden,
                    },
                    timeout: {
                        target: fsmHidden,
                    },
                    mouseMove: {
                        guard: () => !fsm.isInteractive && fsm.revealMode === fsm.revealAnywhere,
                        action: () => {
                            fsm.revealedEdge = Helpers.Edge.Both
                            fsm.timeoutDuration = fsm.hideTimeout;
                        },
                        target: fsmVideoMode
                    },
                    keyboardMove: {
                        guard: () => !fsm.isInteractive,
                        action: () => {
                            fsm.revealedEdge = Helpers.Edge.Both
                            fsm.timeoutDuration = fsm.hideTimeout * 2
                        },
                        target: fsmVideoMode
                    },
                    pointerZone: [{
                        //the pointer left the edge zone: hide at once
                        guard: (zone) => fsm._revealedByPointer && zone === Helpers.Edge.None,
                        target: fsmHidden
                    }, {
                        //the pointer moved to another edge zone
                        guard: (zone) => fsm._canRevealEdge(zone) && zone !== fsm.revealedEdge,
                        action: (zone) => {
                            fsm.revealedEdge = zone
                        },
                        target: fsmVideoMode
                    }]
                })
            }
        }

        FSMState {
            id: fsmLocked

            transitions: ({
                lock: {
                    action: () => { fsm.lockCount++; }
                },
                unlock: [{
                    //bars revealed by the pointer hide once unlocked
                    //if the pointer is no longer within an edge zone
                    guard: () => fsm.lockCount === 1
                                 && fsm._revealedByPointer
                                 && fsm.lastPointerZone === Helpers.Edge.None,
                    action: () => { fsm.lockCount--; },
                    target: fsmHidden,
                }, {
                    action: () => {
                        fsm.lockCount--;
                        if (fsm._revealedByPointer)
                            fsm.revealedEdge = fsm.lastPointerZone
                    },
                    guard: () => fsm.lockCount === 1,
                    target: fsmNormalVisible,
                }, {
                    guard: () => fsm.lockCount > 1,
                    action: () => { fsm.lockCount--; }
                }],

            })
        }
    }

    FSMState {
        id: fsmHidden

        function enter() {
            fsm.lockCount = 0;
            fsm.forceUnlock();
        }

        transitions: ({
            mouseMove: {
                guard: () => !fsm.isInteractive && fsm.revealMode === fsm.revealAnywhere,
                action: () => {
                    fsm.revealedEdge = Helpers.Edge.Both
                    fsm.timeoutDuration = fsm.hideTimeout
                },
                target: fsmVisible
            },
            keyboardMove: {
                guard: () => !fsm.isInteractive,
                action: () => {
                    fsm.revealedEdge = Helpers.Edge.Both
                    fsm.timeoutDuration = fsm.hideTimeout * 2
                },
                target: fsmVisible
            },
            pointerZone: {
                guard: (zone) => fsm._canRevealEdge(zone),
                action: (zone) => {
                    fsm.revealedEdge = zone
                },
                target: fsmVisible
            },
            askShow: {
                action: () => {
                    fsm.revealedEdge = Helpers.Edge.Both
                },
                target: fsmVisible
            },
            updateVideoEmbed: {
                guard: () => !fsm.hasEmbededVideo,
                target: fsmVisible
            },
        })
    }

    Timer {
        id: timeout
        interval: fsm.timeoutDuration
        repeat: false
        onTriggered: {
            fsm.timeout();
        }
    }
}
