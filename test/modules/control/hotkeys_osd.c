/*****************************************************************************
 * hotkeys_osd.c: test the key attribution of the hotkeys OSD
 *****************************************************************************
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <vlc_common.h>
#include "../../../modules/control/hotkeys_osd.h"

static void check_label(uint32_t keycode, const char *expected)
{
    char *label = hotkeys_osd_KeyLabel(keycode);
    if (label == NULL || strcmp(label, expected) != 0)
    {
        fprintf(stderr, "keycode 0x%"PRIx32": got \"%s\", expected \"%s\"\n",
                keycode, label ? label : "(null)", expected);
        assert(!"wrong label");
    }
    free(label);
}

static void test_label_letter(void)
{
    check_label('c', "Pressed C");
}

static void test_label_modifiers(void)
{
    check_label(KEY_MODIFIER_CTRL | KEY_MODIFIER_SHIFT | 's',
                "Pressed Ctrl+Shift+S");
}

static void test_label_wheel(void)
{
    check_label(KEY_MOUSEWHEELUP, "Pressed Mouse Wheel Up");
}

#define THREAD_A 1
#define THREAD_B 2

static void test_tracker_consumes_once(void)
{
    struct hotkeys_key_tracker tracker;
    hotkeys_key_tracker_Init(&tracker);

    hotkeys_key_tracker_KeyPressed(&tracker, 'c', true, THREAD_A);
    assert(hotkeys_key_tracker_TakeForAction(&tracker, THREAD_A) == 'c');
    assert(hotkeys_key_tracker_TakeForAction(&tracker, THREAD_A) == 0);
}

static void test_tracker_direct_action(void)
{
    struct hotkeys_key_tracker tracker;
    hotkeys_key_tracker_Init(&tracker);

    /* An action set directly on "key-action" (menus, CLI) */
    assert(hotkeys_key_tracker_TakeForAction(&tracker, THREAD_A) == 0);

    /* A key pressed on another thread (vout window) is not the origin of an
     * action dispatched on this thread */
    hotkeys_key_tracker_KeyPressed(&tracker, 'c', true, THREAD_B);
    assert(hotkeys_key_tracker_TakeForAction(&tracker, THREAD_A) == 0);
}

static void test_tracker_unmapped_key(void)
{
    struct hotkeys_key_tracker tracker;
    hotkeys_key_tracker_Init(&tracker);

    hotkeys_key_tracker_KeyPressed(&tracker, 'q', false, THREAD_A);
    assert(hotkeys_key_tracker_TakeForAction(&tracker, THREAD_A) == 0);

    /* A mapped key followed by an unmapped one: the unmapped one clears it */
    hotkeys_key_tracker_KeyPressed(&tracker, 'c', true, THREAD_A);
    hotkeys_key_tracker_KeyPressed(&tracker, 'q', false, THREAD_A);
    assert(hotkeys_key_tracker_TakeForAction(&tracker, THREAD_A) == 0);

    /* Invalidate drops a key whose action was never dispatched */
    hotkeys_key_tracker_KeyPressed(&tracker, 'c', true, THREAD_A);
    hotkeys_key_tracker_Invalidate(&tracker);
    assert(hotkeys_key_tracker_TakeForAction(&tracker, THREAD_A) == 0);
}

static void test_keymap_lookup(void)
{
    uint32_t keys[] = { 'x', KEY_MOUSEWHEELUP, 'c', KEY_MODIFIER_CTRL | 's' };
    hotkeys_osd_SortKeys(keys, ARRAY_SIZE(keys));

    assert(hotkeys_osd_IsKeyMapped(keys, ARRAY_SIZE(keys), 'c'));
    assert(hotkeys_osd_IsKeyMapped(keys, ARRAY_SIZE(keys), 'x'));
    assert(hotkeys_osd_IsKeyMapped(keys, ARRAY_SIZE(keys), KEY_MOUSEWHEELUP));
    assert(hotkeys_osd_IsKeyMapped(keys, ARRAY_SIZE(keys),
                                   KEY_MODIFIER_CTRL | 's'));
    assert(!hotkeys_osd_IsKeyMapped(keys, ARRAY_SIZE(keys), 's'));
    assert(!hotkeys_osd_IsKeyMapped(keys, ARRAY_SIZE(keys), 'q'));
    assert(!hotkeys_osd_IsKeyMapped(NULL, 0, 'c'));
}

static const struct
{
    const char *name;
    void (*run)(void);
} tests[] = {
    { "test_label_letter", test_label_letter },
    { "test_label_modifiers", test_label_modifiers },
    { "test_label_wheel", test_label_wheel },
    { "test_tracker_consumes_once", test_tracker_consumes_once },
    { "test_tracker_direct_action", test_tracker_direct_action },
    { "test_tracker_unmapped_key", test_tracker_unmapped_key },
    { "test_keymap_lookup", test_keymap_lookup },
};

int main(int argc, char *argv[])
{
    for (size_t i = 0; i < ARRAY_SIZE(tests); i++)
    {
        if (argc > 1 && strcmp(argv[1], tests[i].name) != 0)
            continue;
        fprintf(stderr, "%s\n", tests[i].name);
        tests[i].run();
    }
    return 0;
}
