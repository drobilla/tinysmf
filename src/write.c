/**
 * tinysmf: a midifile parsing and writing library
 * Copyright (c) 2016 David Robillard.  All rights reserved.
 * Copyright (c) 2014 William Light.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Cavear emptor, this is currently half-baked */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* for htons, htonl */
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <tinysmf.h>

#define UINT16_TO_BE(x) htons(x)
#define UINT32_TO_BE(x) htonl(x)

static size_t
write_vlv(FILE *f, uint32_t value)
{
	size_t   ret    = 0;
	uint32_t buffer = value & 0x7F;
	while ((value >>= 7)) {
		buffer <<= 8;
		buffer |= ((value & 0x7F) | 0x80);
	}

	while (1) {
		++ret;
		fputc(buffer, f);
		if (buffer & 0x80) {
			buffer >>= 8;
		} else {
			break;
		}
	}

	return ret;
}

static int
write_chunk_header(FILE* stream, const char id[4], uint32_t length)
{
	const uint32_t length_be = UINT32_TO_BE(length);
	return (fwrite(id, 1, 4, stream) != 4 ||
	        fwrite(&length_be, 4, 1, stream) != 1);
}

int
tinysmf_write_file_start(struct tinysmf_writer_ctx *ctx)
{
	const uint16_t type    = UINT16_TO_BE(ctx->file_info.format);
	const uint16_t ntracks = UINT16_TO_BE(ctx->file_info.num_tracks);
	uint16_t       division;
	switch (ctx->file_info.division_type) {
	case SMF_PPQN:
		division = UINT16_TO_BE(ctx->file_info.division.ppqn);
		break;
	case SMF_SMPTE:
		// TODO
		break;
	}

	write_chunk_header(ctx->stream, "MThd", 6);
	fwrite(&type,     2, 1, ctx->stream);
	fwrite(&ntracks,  2, 1, ctx->stream);
	fwrite(&division, 2, 1, ctx->stream);

	ctx->file_info.num_tracks = 0;

	return 0;
}

int
tinysmf_write_track_start(struct tinysmf_writer_ctx *ctx, uint32_t size)
{
	ctx->current_track_start = ftell(ctx->stream);
	++ctx->file_info.num_tracks;
	ctx->track_size = size;
	return write_chunk_header(ctx->stream, "MTrk", size);
}

int
tinysmf_write_track_end(struct tinysmf_writer_ctx *ctx)
{
	/* write EOT */
	write_vlv(ctx->stream, 0);
	const uint8_t eot[3] = { 0xFF, 0x2F, 0x00 };
	fwrite(eot, 1, 3, ctx->stream);
	ctx->track_size += 4;

	/* update number of tracks in MThd file header */
	fseek(ctx->stream, 0, SEEK_SET);
	tinysmf_write_file_start(ctx);

	/* update size in Mtrk chunk header */
	fseek(ctx->stream, ctx->current_track_start, SEEK_SET);
	write_chunk_header(ctx->stream, "MTrk", ctx->track_size);

	/* seek back to end of file for more writing */
	fseek(ctx->stream, 0, SEEK_END);
	return 0;
}

int
tinysmf_write_meta_event(struct       tinysmf_writer_ctx *ctx,
                         const struct tinysmf_meta_event *ev)
{
	const size_t  stamp_size = write_vlv(ctx->stream, ev->delta);
	const uint8_t meta_mark  = 0xFF;
	fwrite(&meta_mark,     1, 1, ctx->stream);
	fwrite(&ev->meta_type, 1, 1, ctx->stream);
	const size_t size_size = write_vlv(ctx->stream, ev->nbytes);
	fwrite(ev->raw, 1, ev->nbytes, ctx->stream);
	ctx->track_size += 2 + stamp_size + ev->nbytes + size_size;
	return 0;
}

int
tinysmf_write_midi_event(struct       tinysmf_writer_ctx *ctx,
                         const struct tinysmf_midi_event *ev)
{
	const size_t ev_size    = tinysmf_midi_msg_size(ev->bytes[0]);
	const size_t stamp_size = write_vlv(ctx->stream, ev->delta);
	fwrite(ev->bytes, 1, ev_size + 1, ctx->stream);
	ctx->track_size += stamp_size + 1 + ev_size;
	return 0;
}

