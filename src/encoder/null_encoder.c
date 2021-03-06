/*
 * Copyright (C) 2003-2011 The Music Player Daemon Project
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

#include "config.h"
#include "encoder_api.h"
#include "encoder_plugin.h"
#include "pcm_buffer.h"

#include <assert.h>
#include <string.h>

struct null_encoder {
	struct encoder encoder;

	struct pcm_buffer buffer;
	size_t buffer_length;
};

extern const struct encoder_plugin null_encoder_plugin;

static inline GQuark
null_encoder_quark(void)
{
	return g_quark_from_static_string("null_encoder");
}

static struct encoder *
null_encoder_init(G_GNUC_UNUSED const struct config_param *param,
		  G_GNUC_UNUSED GError **error)
{
	struct null_encoder *encoder;

	encoder = g_new(struct null_encoder, 1);
	encoder_struct_init(&encoder->encoder, &null_encoder_plugin);

	return &encoder->encoder;
}

static void
null_encoder_finish(struct encoder *_encoder)
{
	struct null_encoder *encoder = (struct null_encoder *)_encoder;

	g_free(encoder);
}

static void
null_encoder_close(struct encoder *_encoder)
{
	struct null_encoder *encoder = (struct null_encoder *)_encoder;

	pcm_buffer_deinit(&encoder->buffer);
}


static bool
null_encoder_open(struct encoder *_encoder,
		  G_GNUC_UNUSED struct audio_format *audio_format,
		  G_GNUC_UNUSED GError **error)
{
	struct null_encoder *encoder = (struct null_encoder *)_encoder;

	encoder->buffer_length = 0;
	pcm_buffer_init(&encoder->buffer);

	return true;
}

static bool
null_encoder_write(struct encoder *_encoder,
		   const void *data, size_t length,
		   G_GNUC_UNUSED GError **error)
{
	struct null_encoder *encoder = (struct null_encoder *)_encoder;
	char *buffer = pcm_buffer_get(&encoder->buffer, encoder->buffer_length + length);

	memcpy(buffer+encoder->buffer_length, data, length);

	encoder->buffer_length += length;
	return true;
}

static size_t
null_encoder_read(struct encoder *_encoder, void *dest, size_t length)
{
	struct null_encoder *encoder = (struct null_encoder *)_encoder;
	char *buffer = pcm_buffer_get(&encoder->buffer, encoder->buffer_length);

	if (length > encoder->buffer_length)
		length = encoder->buffer_length;

	memcpy(dest, buffer, length);

	encoder->buffer_length -= length;
	memmove(buffer, buffer + length, encoder->buffer_length);

	return length;
}

const struct encoder_plugin null_encoder_plugin = {
	.name = "null",
	.init = null_encoder_init,
	.finish = null_encoder_finish,
	.open = null_encoder_open,
	.close = null_encoder_close,
	.write = null_encoder_write,
	.read = null_encoder_read,
};
