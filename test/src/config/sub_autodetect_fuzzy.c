/*****************************************************************************
 * sub_autodetect_fuzzy.c: test the subtitle autodetection fuzziness option
 *****************************************************************************
 * Copyright (C) 2026 VideoLAN and authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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

#include "../../libvlc/test.h"

#include <string.h>

#include <vlc_common.h>
#include <vlc_configuration.h>

static void test_fuzzy_has_named_levels(const module_config_t *cfg)
{
    static const int expected[] = { 0, 1, 2, 3, 4 };

    assert(cfg->list_count == ARRAY_SIZE(expected));
    assert(cfg->list.i != NULL);
    assert(cfg->list_text != NULL);

    for (size_t i = 0; i < ARRAY_SIZE(expected); i++)
    {
        assert(cfg->list.i[i] == expected[i]);
        assert(cfg->list_text[i] != NULL);
        assert(cfg->list_text[i][0] != '\0');
    }
}

static void test_fuzzy_default_unchanged(const module_config_t *cfg)
{
    assert(cfg->orig.i == 3);
}

int main(void)
{
    test_init();

    libvlc_instance_t *vlc = libvlc_new(test_defaults_nargs,
                                        test_defaults_args);
    assert(vlc != NULL);

    const module_config_t *cfg = config_FindConfig("sub-autodetect-fuzzy");
    assert(cfg != NULL);

    test_fuzzy_has_named_levels(cfg);
    test_fuzzy_default_unchanged(cfg);

    libvlc_release(vlc);
    return 0;
}
