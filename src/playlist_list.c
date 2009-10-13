/*
 * Copyright (C) 2003-2009 The Music Player Daemon Project
 * http://www.musicpd.org
 *
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "playlist_list.h"
#include "playlist_plugin.h"
#include "playlist/m3u_playlist_plugin.h"
#include "input_stream.h"
#include "uri.h"
#include "utils.h"

#include <glib.h>

#include <assert.h>

static const struct playlist_plugin *const playlist_plugins[] = {
	&m3u_playlist_plugin,
	NULL
};

/** which plugins have been initialized successfully? */
static bool playlist_plugins_enabled[G_N_ELEMENTS(playlist_plugins)];

void
playlist_list_global_init(void)
{
	for (unsigned i = 0; playlist_plugins[i] != NULL; ++i)
		playlist_plugins_enabled[i] =
			playlist_plugin_init(playlist_plugins[i], NULL);
}

void
playlist_list_global_finish(void)
{
	for (unsigned i = 0; playlist_plugins[i] != NULL; ++i)
		if (playlist_plugins_enabled[i])
			playlist_plugin_finish(playlist_plugins[i]);
}

struct playlist_provider *
playlist_list_open_uri(const char *uri)
{
	char *scheme;
	struct playlist_provider *playlist = NULL;

	assert(uri != NULL);

	scheme = g_uri_parse_scheme(uri);
	if (scheme == NULL)
		return NULL;

	for (unsigned i = 0; playlist_plugins[i] != NULL; ++i) {
		const struct playlist_plugin *plugin = playlist_plugins[i];

		if (playlist_plugins_enabled[i] &&
		    stringFoundInStringArray(plugin->schemes, scheme)) {
			playlist = playlist_plugin_open_uri(plugin, uri);
			if (playlist != NULL)
				break;
		}
	}

	g_free(scheme);
	return playlist;
}

static struct playlist_provider *
playlist_list_open_stream_mime(struct input_stream *is)
{
	struct playlist_provider *playlist;

	assert(is != NULL);
	assert(is->mime != NULL);

	for (unsigned i = 0; playlist_plugins[i] != NULL; ++i) {
		const struct playlist_plugin *plugin = playlist_plugins[i];

		if (playlist_plugins_enabled[i] &&
		    stringFoundInStringArray(plugin->mime_types, is->mime)) {
			playlist = playlist_plugin_open_stream(plugin, is);
			if (playlist != NULL)
				return playlist;
		}
	}

	return NULL;
}

static struct playlist_provider *
playlist_list_open_stream_suffix(struct input_stream *is, const char *suffix)
{
	struct playlist_provider *playlist;

	assert(is != NULL);
	assert(suffix != NULL);

	for (unsigned i = 0; playlist_plugins[i] != NULL; ++i) {
		const struct playlist_plugin *plugin = playlist_plugins[i];

		if (playlist_plugins_enabled[i] &&
		    stringFoundInStringArray(plugin->suffixes, suffix)) {
			playlist = playlist_plugin_open_stream(plugin, is);
			if (playlist != NULL)
				return playlist;
		}
	}

	return NULL;
}

struct playlist_provider *
playlist_list_open_stream(struct input_stream *is, const char *uri)
{
	const char *suffix;
	struct playlist_provider *playlist;

	if (is->mime != NULL) {
		playlist = playlist_list_open_stream_mime(is);
		if (playlist != NULL)
			return playlist;
	}

	suffix = uri != NULL ? uri_get_suffix(uri) : NULL;
	if (suffix != NULL) {
		playlist = playlist_list_open_stream_suffix(is, suffix);
		if (playlist != NULL)
			return playlist;
	}

	return NULL;
}