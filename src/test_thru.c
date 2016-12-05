/**
 * tinysmf: a midifile parsing library
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

#include <tinysmf.h>
#include <stdio.h>

struct test_parser {
	struct tinysmf_parser_ctx ctx;
	struct tinysmf_writer_ctx wctx;
};

static void
on_meta_event(struct tinysmf_parser_ctx *ctx,
		const struct tinysmf_meta_event *ev)
{
	struct test_parser *p = (void *) ctx;

	if (ev->meta_type != 0x2F) { /* ignore EOT */
		tinysmf_write_meta_event(&p->wctx, ev);
	}
}

static void
on_midi_event(struct tinysmf_parser_ctx *ctx,
		const struct tinysmf_midi_event *ev)
{
	struct test_parser *p = (void *) ctx;

	tinysmf_write_midi_event(&p->wctx, ev);
}

static tinysmf_chunk_cb_ret_t
on_track_start(struct tinysmf_parser_ctx *ctx, int track_idx)
{
	struct test_parser *p = (void *) ctx;

	tinysmf_write_track_start(&p->wctx, 0);
	return TINYSMF_PARSE_CHUNK;
}

static tinysmf_chunk_cb_ret_t
on_track_end(struct tinysmf_parser_ctx *ctx, int track_idx)
{
	struct test_parser *p = (void *) ctx;

	tinysmf_write_track_end(&p->wctx);
	return 0;
}

static void
on_file_info(struct tinysmf_parser_ctx *ctx)
{
	struct test_parser *p = (void *) ctx;
	p->wctx.file_info = ctx->file_info;
	p->wctx.stream    = stdout;
	tinysmf_write_file_start(&p->wctx);
}

int
main(int argc, char **argv)
{
	struct test_parser p = {
		.ctx = {
			.file_info_cb   = on_file_info,

			.track_start_cb = on_track_start,
			.track_end_cb   = on_track_end,

			.midi_event_cb  = on_midi_event,
			.meta_event_cb  = on_meta_event
		}
	};

	tinysmf_parse_stream((void *) &p, stdin);
	fclose(p.wctx.stream);

	return 0;
}
