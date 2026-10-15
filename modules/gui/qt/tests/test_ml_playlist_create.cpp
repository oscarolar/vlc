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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "../medialibrary/mlplaylistlistmodel.hpp"
#include "../medialibrary/mlcustomcover.hpp"
#include "../medialibrary/medialib.hpp"
#include "../dialogs/dialogs_provider.hpp"
#include "vlc_stub_modules.hpp"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <memory>

#include <vlc_media_library.h>

#include <QTest>

// The model only needs these for renaming and covers, which are not tested
// here; provide definitions so the test does not link the whole interface.
QVariant DialogsProvider::getTextDialog(QWidget *, const QString &, const QString &,
                                        const QString &, bool *ok)
{
    if (ok)
        *ok = false;
    return {};
}

QString MLCustomCover::url(const MLItemId &, const QSize &, const QString &,
                           int, int, int, bool)
{
    return {};
}

namespace {

const int64_t CREATED_PLAYLIST_ID = 42;

std::atomic<int> createCount;
std::atomic<int> appendCount;
std::atomic<int64_t> appendPlaylistId;

int mlControl(int query, va_list args)
{
    switch (query)
    {
    case VLC_ML_PLAYLIST_CREATE:
    {
        const char *name = va_arg(args, const char *);
        vlc_ml_playlist_t **result = va_arg(args, vlc_ml_playlist_t **);
        auto *playlist = static_cast<vlc_ml_playlist_t *>(calloc(1, sizeof(vlc_ml_playlist_t)));
        if (!playlist)
            return VLC_ENOMEM;
        playlist->i_id = CREATED_PLAYLIST_ID;
        playlist->psz_name = strdup(name);
        *result = playlist;
        ++createCount;
        return VLC_SUCCESS;
    }
    case VLC_ML_PLAYLIST_APPEND:
        appendPlaylistId = va_arg(args, int64_t);
        ++appendCount;
        return VLC_SUCCESS;
    default:
        return VLC_EGENERIC;
    }
}

} // anonymous namespace

class TestMLPlaylistCreate : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        m_env = std::make_unique<VLCTestingEnv>();
        m_env->mlControl = mlControl;
        QVERIFY(m_env->init());
    }

    void cleanupTestCase() {
        m_env.reset();
    }

    void init() {
        createCount = 0;
        appendCount = 0;
        appendPlaylistId = 0;
        m_medialib = std::make_unique<MediaLib>(m_env->intf, nullptr);
        m_model = std::make_unique<MLPlaylistListModel>();
        m_model->classBegin();
        m_model->setMl(m_medialib.get());
    }

    void cleanup() {
        m_model.reset();
        m_medialib.reset();
    }

    void testCreateEmpty()
    {
        m_model->create(QStringLiteral("Empty"), {});
        QVERIFY(m_model->transactionPending());

        QVERIFY(QTest::qWaitFor([this]() { return !m_model->transactionPending(); }));
        QCOMPARE(createCount.load(), 1);
        QCOMPARE(appendCount.load(), 0);
    }

    void testCreateWithItems()
    {
        const QVariantList items {
            QVariant::fromValue(MLItemId{1, VLC_ML_PARENT_UNKNOWN}),
            QVariant::fromValue(MLItemId{2, VLC_ML_PARENT_UNKNOWN}),
        };
        m_model->create(QStringLiteral("Two"), items);

        QVERIFY(QTest::qWaitFor([]() { return appendCount == 2; }));
        QVERIFY(QTest::qWaitFor([this]() { return !m_model->transactionPending(); }));
        QCOMPARE(createCount.load(), 1);
        QCOMPARE(appendPlaylistId.load(), CREATED_PLAYLIST_ID);
    }

private:
    std::unique_ptr<VLCTestingEnv> m_env;
    std::unique_ptr<MediaLib> m_medialib;
    std::unique_ptr<MLPlaylistListModel> m_model;
};

QTEST_GUILESS_MAIN(TestMLPlaylistCreate)
#include "test_ml_playlist_create.moc"
