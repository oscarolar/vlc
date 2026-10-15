/*****************************************************************************
 * hotkeys_osd.h: name the key that triggered a hotkey action
 *****************************************************************************
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

#ifndef VLC_HOTKEYS_OSD_H
#define VLC_HOTKEYS_OSD_H

#include <vlc_common.h>
#include <vlc_actions.h>

#include <stdlib.h>
#include <string.h>

/**
 * Label shown under the action OSD, e.g. "Pressed Ctrl+Shift+S".
 *
 * @param keycode VLC key code, modifiers included
 * @return a heap-allocated localized string, or NULL on error
 */
static inline char *hotkeys_osd_KeyLabel(uint32_t keycode)
{
    char *name = vlc_keycode2str(keycode, true);
    if (name == NULL)
        return NULL;

    /* Letters are stored lower case in the key map; show them as printed
     * on the key cap. */
    uint32_t key = keycode & ~KEY_MODIFIER;
    if (key >= 'a' && key <= 'z')
    {
        size_t len = strlen(name);
        if (len > 0 && name[len - 1] == (char)key)
            name[len - 1] = (char)(key - 'a' + 'A');
    }

    char *label;
    if (asprintf(&label, _("Pressed %s"), name) == -1)
        label = NULL;
    free(name);
    return label;
}

/**
 * Remembers the key whose "key-pressed" event is being dispatched, so that
 * the "key-action" it resolves to (set synchronously, on the same thread,
 * by the core key map) can name it. Actions set directly on "key-action"
 * (menus, CLI, Qt) find no key.
 *
 * Not thread-safe: the caller serializes the calls.
 */
struct hotkeys_key_tracker
{
    uint32_t key;
    unsigned long thread;
    bool valid;
};

static inline void
hotkeys_key_tracker_Init(struct hotkeys_key_tracker *tracker)
{
    tracker->key = KEY_UNSET;
    tracker->thread = 0;
    tracker->valid = false;
}

static inline void
hotkeys_key_tracker_Invalidate(struct hotkeys_key_tracker *tracker)
{
    hotkeys_key_tracker_Init(tracker);
}

/**
 * Records a key press. A key that maps to no action only clears the
 * previous record, so that it cannot be blamed for a later direct action.
 */
static inline void
hotkeys_key_tracker_KeyPressed(struct hotkeys_key_tracker *tracker,
                               uint32_t key, bool mapped, unsigned long thread)
{
    hotkeys_key_tracker_Invalidate(tracker);
    if (!mapped)
        return;
    tracker->key = key;
    tracker->thread = thread;
    tracker->valid = true;
}

/**
 * Returns the key that triggered the action being dispatched on the given
 * thread, or KEY_UNSET (0). The record is consumed either way.
 */
static inline uint32_t
hotkeys_key_tracker_TakeForAction(struct hotkeys_key_tracker *tracker,
                                  unsigned long thread)
{
    uint32_t key = tracker->valid && tracker->thread == thread
                 ? tracker->key : KEY_UNSET;
    hotkeys_key_tracker_Invalidate(tracker);
    return key;
}

static inline int hotkeys_osd_CompareKeys(const void *a, const void *b)
{
    uint32_t ka = *(const uint32_t *)a, kb = *(const uint32_t *)b;
    return (ka > kb) - (ka < kb);
}

static inline void hotkeys_osd_SortKeys(uint32_t *keys, size_t count)
{
    if (count > 0)
        qsort(keys, count, sizeof (*keys), hotkeys_osd_CompareKeys);
}

/**
 * Tells whether a key code is bound to an action.
 *
 * @param keys key codes sorted with hotkeys_osd_SortKeys()
 */
static inline bool
hotkeys_osd_IsKeyMapped(const uint32_t *keys, size_t count, uint32_t key)
{
    if (count == 0)
        return false;
    return bsearch(&key, keys, count, sizeof (*keys),
                   hotkeys_osd_CompareKeys) != NULL;
}

#endif
