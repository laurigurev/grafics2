#include "grafics2.h"

#define ENDIAN_WORD(a) ((((0xFF00 & a) >> 8) | ((0x00FF & a) << 8)))
#define ENDIAN_DWORD(a) \
	(((0xFF000000 & a) >> 24) | ((0x00FF0000 & a) >> 8) |		\
	 ((0x0000FF00 & a) << 8) | ((0x000000FF & a) << 24))

#define U64_SIZE sizeof(u64)
#define U32_SIZE sizeof(u32)
#define U16_SIZE sizeof(u16)

#define TTF_HEAD_TAG 0x68656164
#define TTF_MAXP_TAG 0x6d617870
#define TTF_CMAP_TAG 0x636d6170
#define TTF_LOCA_TAG 0x6c6f6361
#define TTF_GLYF_TAG 0x676c7966
#define TTF_HHEA_TAG 0x68686561
#define TTF_HMTX_TAG 0x686d7478

#define TTF_WIN_PLATFORM_ID 3
#define TTF_FLAG_REPEAT 0x08

s32 f2fot14_to_float_2(u16 f2dot14);

int ttf_load(TrueTypeFont** true_type_font, const char* font_path)
{
	u32 buffer_size;
	char* buffer = file_read(font_path, &buffer_size);
	if (buffer == NULL) { return 0; }

	u16 num_tables = ENDIAN_WORD(*((u16*) (buffer + U32_SIZE)));

	u64 table_size = 4 * U32_SIZE;
	u64 offset = U32_SIZE + 4 * U16_SIZE;	// sizeof 'offset subtable'
	TrueTypeFontTable tables[] = {
		(TrueTypeFontTable) { 0, 0 },	// head
		(TrueTypeFontTable) { 0, 0 },	// maxp
		(TrueTypeFontTable) { 0, 0 },	// cmap
		(TrueTypeFontTable) { 0, 0 },	// loca
		(TrueTypeFontTable) { 0, 0 },	// glyf
		(TrueTypeFontTable) { 0, 0 },	// hhea
		(TrueTypeFontTable) { 0, 0 }	// hmtx
	};
	u32 tables_found = 0;
	for (u16 i = 0; i < num_tables; i++) {
		u32 tag = ENDIAN_DWORD(*((u32*) (buffer + offset)));
		// u32 checksum = ENDIAN_DWORD(*((u32*) (buffer + offset + U32_SIZE)));
		u32 table_offset = ENDIAN_DWORD(*((u32*) (buffer + offset + 2 * U32_SIZE)));
		u32 table_length = ENDIAN_DWORD(*((u32*) (buffer + offset + 3 * U32_SIZE)));

		switch (tag) {
		case TTF_HEAD_TAG:
			tables[0] = (TrueTypeFontTable) { table_offset, table_length };
			tables_found++;
			break;
		case TTF_MAXP_TAG:
			tables[1] = (TrueTypeFontTable) { table_offset, table_length };
			tables_found++;
			break;
		case TTF_CMAP_TAG:
			tables[2] = (TrueTypeFontTable) { table_offset, table_length };
			tables_found++;
			break;
		case TTF_LOCA_TAG:
			tables[3] = (TrueTypeFontTable) { table_offset, table_length };
			tables_found++;
			break;
		case TTF_GLYF_TAG:
			tables[4] = (TrueTypeFontTable) { table_offset, table_length };
			tables_found++;
			break;
		case TTF_HHEA_TAG:
			tables[5] = (TrueTypeFontTable) { table_offset, table_length };
			tables_found++;
			break;
		case TTF_HMTX_TAG:
			tables[6] = (TrueTypeFontTable) { table_offset, table_length };
			tables_found++;
			break;
		}
		offset += table_size;
	}

	if (tables_found != 7) { return -1; }

	// calculate allocation size
	/*
		size = sizeof(ttf) +							// sizeof of ttf struct
			   sizeof(u16) * 4 * seg_count +			// sizeof cmap stuff
			   sizeof(u32) * num_glyfs +				// sizeof loca
			   sizeof(glyf) * num_glyf +				// sizeof glyf array
			   ng * max_points * 3 * sizeof(u16)		// sizeof of glyf arrays * length
			   ng * max_points;


		NOTE:
		 - this is not very precise, because you allocate more than you need in terms of
		   glyf specific arrays, for example this implementation does not care about
		   intruction etc. So allocation will always be too big, don't know how big the
		   margin is going to be. You could go and parse all glyf num_points and sum
		   them together, but that might be too time consuming
	*/

	u16 tmp_num_glyphs = ENDIAN_WORD(*((u16*) (buffer + tables[1].offset + U32_SIZE)));
	u16 max_points = ENDIAN_WORD(*((u16*) (buffer + tables[1].offset + U32_SIZE + U16_SIZE)));

	void* format4_ptr = NULL;
	u16 seg_count;
	{
		u16 num_subtables = ENDIAN_WORD(*((u16*) (buffer + tables[2].offset + U16_SIZE)));
		u64 cmap_offset = tables[2].offset + 2 * U16_SIZE;
		u64 subtable_size = 2 * U16_SIZE + U32_SIZE;
		for (u16 i = 0; i < num_subtables; i++) {
			u16 platform_id = ENDIAN_WORD(
					  *((u16*) (buffer + cmap_offset + subtable_size * i + 0)));
			/*
			u16 platform_specific_id = ENDIAN_WORD(
 					   *((u16*) (buffer + cmap_offset + subtable_size * i + U16_SIZE)));
		   */
			u32 format_offset = ENDIAN_DWORD(
					   *((u32*) (buffer + cmap_offset + subtable_size * i + 2 * U16_SIZE)));
			u16 format = ENDIAN_WORD(*((u16*) (buffer + tables[2].offset + format_offset)));

			if (platform_id == TTF_WIN_PLATFORM_ID && format == 4) {
				format4_ptr = (void*) (buffer + tables[2].offset + format_offset);
				seg_count = (u16) *((u16*) (format4_ptr + 3 * U16_SIZE));
				seg_count = ENDIAN_WORD(seg_count);
				seg_count /= 2;
			}
		}
	}

	if (format4_ptr == NULL) { return -1; }

	// TODO: recalculate all of this, 50MB allocated, 2MB needed
	u64 size = 0;
	size += sizeof(TrueTypeFont);
	size += U16_SIZE * seg_count * 4;
	size += U32_SIZE * tmp_num_glyphs;
	size += sizeof(TrueTypeFontGlyph) * tmp_num_glyphs;
	size += U16_SIZE * 3 * tmp_num_glyphs * max_points;
	size += tmp_num_glyphs * max_points;

	TrueTypeFont* ttf = (TrueTypeFont*) malloc(size);
	*true_type_font = ttf;
	u64 alloc_tail_offset = sizeof(TrueTypeFont);

	i16 index_to_loc_format = -1;
	// fill ttf with data
	{
		// head
		u64 head_offset = tables[0].offset + 4 * U32_SIZE + U16_SIZE;
		ttf->units_per_em = ENDIAN_WORD(*((u16*) (buffer + head_offset)));
		head_offset += U16_SIZE + 2 * U64_SIZE;
		ttf->x_min = ENDIAN_WORD(*((i16*) (buffer + head_offset)));
		head_offset += U16_SIZE;
		ttf->y_min = ENDIAN_WORD(*((i16*) (buffer + head_offset)));
		head_offset += U16_SIZE;
		ttf->x_max = ENDIAN_WORD(*((i16*) (buffer + head_offset)));
		head_offset += U16_SIZE;
		ttf->y_max = ENDIAN_WORD(*((i16*) (buffer + head_offset)));
		head_offset += 2 * U16_SIZE;
		ttf->lowest_rec_ppem = ENDIAN_WORD(*((u16*) (buffer + head_offset)));
		head_offset += 2 * U16_SIZE;
		index_to_loc_format = ENDIAN_WORD(*((i16*) (buffer + head_offset)));
	}

	if (index_to_loc_format == -1) { return -1; }

	{
		// hhea
		u64 hhea_offset = tables[5].offset + U32_SIZE;
		ttf->ascent = ENDIAN_WORD(*((i16*) (buffer + hhea_offset)));
		hhea_offset += U16_SIZE;
		ttf->descent = ENDIAN_WORD(*((i16*) (buffer + hhea_offset)));
		hhea_offset += 14 * U16_SIZE;
		ttf->num_hmtx = ENDIAN_WORD(*((u16*) (buffer + hhea_offset)));
	}

	{
		// hmtx
		u64 hmtx_offset = tables[6].offset;
		ttf->hmtx = ((void*) ttf) + alloc_tail_offset;
		memcpy(ttf->hmtx, buffer + hmtx_offset, ttf->num_hmtx * sizeof(TrueTypeFontMetric));
		alloc_tail_offset += ttf->num_hmtx * sizeof(TrueTypeFontMetric);
		hmtx_offset += ttf->num_hmtx * sizeof(TrueTypeFontMetric);
		
		ttf->num_lsb = tmp_num_glyphs - ttf->num_hmtx;
		ttf->lsbs = NULL;
		if (ttf->num_lsb != 0) {
			ttf->lsbs = ((void*) ttf) + alloc_tail_offset;
			memcpy(ttf->lsbs, buffer + hmtx_offset, ttf->num_lsb * U16_SIZE);
			alloc_tail_offset += ttf->num_lsb * U16_SIZE;
		}

		for (u16 i = 0; i < ttf->num_hmtx; i++) {
			ttf->hmtx[i].aw = ENDIAN_WORD(ttf->hmtx[i].aw);
			ttf->hmtx[i].lsb = ENDIAN_WORD(ttf->hmtx[i].lsb);
		}

		for (u16 i = 0; i < ttf->num_lsb; i++) {
			ttf->lsbs[i] = ENDIAN_WORD(ttf->lsbs[i]);
		}
	}

	{
		// maxp
		ttf->num_glyphs = tmp_num_glyphs;
	}

	{
		// cmap
		u16 length = ENDIAN_WORD(*((u16*) (format4_ptr + U16_SIZE)));
		ttf->seg_count_2 = seg_count * 2;
		u64 cmap_offset = 7 * U16_SIZE;

		// end_code
		ttf->end_code = ((void*) ttf) + alloc_tail_offset;
		memcpy(ttf->end_code, format4_ptr + cmap_offset, seg_count * U16_SIZE);
		alloc_tail_offset += seg_count * U16_SIZE;
		cmap_offset += seg_count * U16_SIZE + U16_SIZE;

		// start_code
		ttf->start_code = ((void*) ttf) + alloc_tail_offset;
		memcpy(ttf->start_code, format4_ptr + cmap_offset, seg_count * U16_SIZE);
		alloc_tail_offset += seg_count * U16_SIZE;
		cmap_offset += seg_count * U16_SIZE;

		// id_delta
		ttf->id_delta = ((void*) ttf) + alloc_tail_offset;
		memcpy(ttf->id_delta, format4_ptr + cmap_offset, seg_count * U16_SIZE);
		alloc_tail_offset += seg_count * U16_SIZE;
		cmap_offset += seg_count * U16_SIZE;

		// id_range_offset
		u64 len = length - 3 * 2 * seg_count;
		ttf->id_range_offset = ((void*) ttf) + alloc_tail_offset;
		memcpy(ttf->id_range_offset, format4_ptr + cmap_offset, len);
		alloc_tail_offset += len;

		for (u16 i = 0; i < seg_count; i++) {
			ttf->end_code[i] = ENDIAN_WORD(ttf->end_code[i]);
			ttf->start_code[i] = ENDIAN_WORD(ttf->start_code[i]);
			ttf->id_delta[i] = ENDIAN_WORD(ttf->id_delta[i]);
		}

		for (u64 i = 0; i < len / 2; i++) {
			ttf->id_range_offset[i] = ENDIAN_WORD(ttf->id_range_offset[i]);
		}
	}

	{
		// loca
		u64 loca_offset = tables[3].offset;
		ttf->loca = ((void*) ttf) + alloc_tail_offset;
		alloc_tail_offset += ttf->num_glyphs * U32_SIZE;

		if (index_to_loc_format) {
			for (u16 i = 0; i < ttf->num_glyphs; i++) {
				u32* location = (u32*) (buffer + loca_offset + i * U32_SIZE);
				ttf->loca[i] = ENDIAN_DWORD(*location);
			}
		} else {
			for (u16 i = 0; i < ttf->num_glyphs; i++) {
				u16 tmp_location = *((u16*) (buffer + loca_offset + i * U16_SIZE));
				tmp_location = ENDIAN_WORD(tmp_location);
				ttf->loca[i] = (u32) tmp_location;
				ttf->loca[i] = ttf->loca[i] << 1;
			}
		}
		
	}

	{
		// glyf
		u64 glyf_offset = tables[4].offset;
		ttf->glyphs = (TrueTypeFontGlyph*) (((void*) ttf) + alloc_tail_offset);
		alloc_tail_offset += ttf->num_glyphs * sizeof(TrueTypeFontGlyph);
		// memset everything to 0
		memset(ttf->glyphs, 0, ttf->num_glyphs * sizeof(TrueTypeFontGlyph));

		for (u16 i = 0; i < ttf->num_glyphs - 1; i++) {
			u32 offset = ttf->loca[i];
			u32 next_offset = ttf->loca[i + 1];

			if (offset - next_offset == 0) {
				ttf->glyphs[i] = (TrueTypeFontGlyph) {
					0, 0, 0, 0, 0, 0, NULL, 0, NULL, NULL, NULL
				};
				continue;
			}
			
			// load glyph
			ttf_glyph_load(ttf, i, buffer + glyf_offset, &alloc_tail_offset);
		}
	}

	assert(alloc_tail_offset < size);
	file_free(buffer);
	return 1;
}

void ttf_free(TrueTypeFont** true_type_font)
{
	TrueTypeFont* ttf = *true_type_font;
	if (ttf) { free(ttf); }
}

void ttf_glyph_load(TrueTypeFont* ttf, u32 glyph_index, void* buffer,
					u64* ttf_buffer_tail_offset)
{
	u32 glyph_offset = ttf->loca[glyph_index];
	TrueTypeFontGlyph* glyph = ttf->glyphs + glyph_index;

	if (glyph->parsed == 1) return;

	glyph->parsed = 1;
	glyph->num_contours = ENDIAN_WORD(*((i16*) (buffer + glyph_offset)));
	glyph_offset += U16_SIZE;
	glyph->x_min = ENDIAN_WORD(*((i16*) (buffer + glyph_offset)));
	glyph_offset += U16_SIZE;
	glyph->y_min = ENDIAN_WORD(*((i16*) (buffer + glyph_offset)));
	glyph_offset += U16_SIZE;
	glyph->x_max = ENDIAN_WORD(*((i16*) (buffer + glyph_offset)));
	glyph_offset += U16_SIZE;
	glyph->y_max = ENDIAN_WORD(*((i16*) (buffer + glyph_offset)));
	glyph_offset += U16_SIZE;

	if (0 < glyph->num_contours) {
		// simple glyph
		glyph->end_pts_of_contours = ((void*) ttf) + *ttf_buffer_tail_offset;
		memcpy(glyph->end_pts_of_contours, buffer + glyph_offset,
			   glyph->num_contours * U16_SIZE);
		glyph_offset += glyph->num_contours * U16_SIZE;
		*ttf_buffer_tail_offset += glyph->num_contours * U16_SIZE;
		
		for (i16 i = 0; i < glyph->num_contours; i++) {
			glyph->end_pts_of_contours[i] = ENDIAN_WORD(glyph->end_pts_of_contours[i]);
		}

		{
			// skip intructions
			u16 instruction_len = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
			glyph_offset += U16_SIZE;
			glyph_offset += instruction_len;
		}

		glyph->num_points = glyph->end_pts_of_contours[glyph->num_contours - 1];
		glyph->num_points++;

		glyph->flags = ((void*) ttf) + *ttf_buffer_tail_offset;
		*ttf_buffer_tail_offset += glyph->num_points;
		u8* tmp_flags = (u8*) (buffer + glyph_offset);
		u64 flags_len = 0;
		u8 tmp_flag0, repeat_count;
		for (u16 i = 0; i < glyph->num_points; i++) {
			tmp_flag0 = *(tmp_flags + flags_len);
			flags_len++;
			glyph->flags[i] = tmp_flag0;
			// if (tmp_flag0 & TTF_FLAG_REPEAT) {
			if (tmp_flag0 & 0x08) {
				repeat_count = *(tmp_flags + flags_len);
				flags_len++;
				for (u8 j = 0; j < repeat_count; j++) {
					i++;
					glyph->flags[i] = tmp_flag0;
					// assert(i < glyph->num_points);
				}
			}
		}
		glyph_offset += flags_len;

		glyph->pts_x = ((void*) ttf) + *ttf_buffer_tail_offset;
		*ttf_buffer_tail_offset += glyph->num_points * U16_SIZE;
		glyph->pts_y = ((void*) ttf) + *ttf_buffer_tail_offset;
		*ttf_buffer_tail_offset += glyph->num_points * U16_SIZE;

		i16 prev_pts = 0;
		i16 tmp_pts = 0;
		for (uint16_t i = 0; i < glyph->num_points; i++) {
			if (glyph->flags[i] & 0x10) {
				if (glyph->flags[i] & 0x02) {
					tmp_pts = (int16_t) *((uint8_t*) (buffer + glyph_offset));
					prev_pts += tmp_pts;
					glyph_offset++;
					glyph->pts_x[i] = prev_pts;
				} else {
					glyph->pts_x[i] = prev_pts;
				}
			} else {
				if (glyph->flags[i] & 0x02) {
					tmp_pts = (int16_t) *((uint8_t*) (buffer + glyph_offset));
					prev_pts -= tmp_pts;
					glyph_offset++;
					glyph->pts_x[i] = prev_pts;
				} else {
					tmp_pts = *((int16_t*) (buffer + glyph_offset));
					tmp_pts = ENDIAN_WORD(tmp_pts);
					prev_pts += tmp_pts;
					glyph_offset += 2;
					glyph->pts_x[i] = prev_pts;
				}
			}
		}

		prev_pts = 0;
		tmp_pts = 0;
		for (uint16_t i = 0; i < glyph->num_points; i++) {
			if (glyph->flags[i] & 0x20) {
				if (glyph->flags[i] & 0x04) {
					tmp_pts = (int16_t) *((uint8_t*) (buffer + glyph_offset));
					prev_pts += tmp_pts;
					glyph_offset++;
					glyph->pts_y[i] = prev_pts;
				}
				else {
					glyph->pts_y[i] = prev_pts;
				}
			}
			else {
				if (glyph->flags[i] & 0x04) {
					tmp_pts = (int16_t) *((uint8_t*) (buffer + glyph_offset));
					prev_pts -= tmp_pts;
					glyph_offset++;
					glyph->pts_y[i] = prev_pts;
				}
				else {
					tmp_pts = *((int16_t*) (buffer + glyph_offset));
					tmp_pts = ENDIAN_WORD(tmp_pts);
					prev_pts += tmp_pts;
					glyph_offset += 2;
					glyph->pts_y[i] = prev_pts;
				}
			}
		}

		// set hmtx
		ttf_glyph_get_hmtc(ttf, glyph, glyph_index);
	} else if (glyph->num_contours < 0) {
		// compound glyph

		// TODO: fix stuff involving aw, lsb
		u16 flags, tmp_glyph_index, x, y, f2dot14;
		i16 total_num_contours = 0;
		i16 total_num_points = 0;
		i32 set_hmtc = 0;
		Array glyphs;
		arr_init(&glyphs, sizeof(TrueTypeFontGlyph*));

		do {
			flags = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
			glyph_offset += U16_SIZE;
			
			tmp_glyph_index = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
			glyph_offset += U16_SIZE;

			// TODO: change the way new glyphs are loaded, because this function need a copy
			// TODO: do a deep copy of the glyf
			ttf_glyph_load(ttf, tmp_glyph_index, buffer, ttf_buffer_tail_offset);
			TrueTypeFontGlyph* component = NULL;
			ttf_glyph_create_deep_copy(ttf, tmp_glyph_index, &component);
			arr_add(&glyphs, &component);
			
			total_num_contours += component->num_contours;
			total_num_points += component->num_points;

			if (flags & 0x0001) {
				x = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				y = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
			} else {
				x = (u16) *((u8*) (buffer + glyph_offset));
				glyph_offset++;
				y = (u16) *((u8*) (buffer + glyph_offset));
				glyph_offset++;
			}

			if (flags & 0x0002) {
				for (u16 i = 0; i < component->num_points; i++) {
					component->pts_x[i] += x;
					component->pts_y[i] += y;
				}
			}

			if (flags & 0x0008) {
				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				// deal with f2dot14
				s32 scale = f2fot14_to_float_2(f2dot14);
				for (u16 i = 0; i < component->num_points; i++) {
					component->pts_x[i] = (i16) component->pts_y[i] * scale;
					component->pts_y[i] = (i16) component->pts_y[i] * scale;
				}
			} else if (flags & 0x0040) {
				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 x_scale = f2fot14_to_float_2(f2dot14);
			
				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 y_scale = f2fot14_to_float_2(f2dot14);

				for (u16 i = 0; i < component->num_points; i++) {
					component->pts_x[i] = (i16) component->pts_x[i] * x_scale;
					component->pts_y[i] = (i16) component->pts_y[i] * y_scale;
				}
			} else if (flags & 0x0080) {
				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 transform_x = f2fot14_to_float_2(f2dot14);

				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 transform_01 = f2fot14_to_float_2(f2dot14);

				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 transform_10 = f2fot14_to_float_2(f2dot14);

				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 transform_y = f2fot14_to_float_2(f2dot14);

				for (u16 i = 0; i < component->num_points; i++) {
					component->pts_x[i] = (i16) component->pts_x[i] * transform_x +
						                  component->pts_y[i] * transform_01;
					component->pts_y[i] = (i16) component->pts_y[i] * transform_y +
						                  component->pts_x[i] * transform_10;
				}
			}

			if (flags & 0x0200) {
				set_hmtc = 1;
				ttf_glyph_get_hmtc(ttf, glyph, glyph_index);
			}
		} while (flags & 0x0020);

		// update num_contours
		// update num_points
		glyph->num_contours = total_num_contours;
		glyph->end_pts_of_contours = (u16*) (((void*) ttf) + *ttf_buffer_tail_offset);
		*ttf_buffer_tail_offset += glyph->num_contours * U16_SIZE;
		
		glyph->num_points = total_num_points;
		glyph->flags = (u8*) (((void*) ttf) + *ttf_buffer_tail_offset);
		*ttf_buffer_tail_offset += glyph->num_points;
		glyph->pts_x = (i16*) (((void*) ttf) + *ttf_buffer_tail_offset);
		*ttf_buffer_tail_offset += glyph->num_points * U16_SIZE;
		glyph->pts_y = (i16*) (((void*) ttf) + *ttf_buffer_tail_offset);
		*ttf_buffer_tail_offset += glyph->num_points * U16_SIZE;

		i16 prev_num_contours = 0;
		u16 prev_num_points = 0;
		for (u32 i = 0; i < glyphs.size; i++) {
			TrueTypeFontGlyph* component = *((TrueTypeFontGlyph**) arr_get(&glyphs, i));

			for (i16 j = 0; j < component->num_contours; j++) {
				component->end_pts_of_contours[j] += prev_num_points;
			}
			memcpy(glyph->end_pts_of_contours + prev_num_contours,
				   component->end_pts_of_contours,
				   component->num_contours * U16_SIZE);
			
			memcpy(glyph->flags + prev_num_points, component->flags, component->num_points);
			memcpy(glyph->pts_x + prev_num_points, component->pts_x,
				   component->num_points * U16_SIZE);
			memcpy(glyph->pts_y + prev_num_points, component->pts_y,
				   component->num_points * U16_SIZE);
			prev_num_contours += component->num_contours;
			prev_num_points += component->num_points;

			free(component);
		}
		arr_free(&glyphs);

		if (set_hmtc) return;
		ttf_glyph_get_hmtc(ttf, glyph, glyph_index);
	} else if (glyph->num_contours == 0) {
		return;
	}
}

void ttf_glyph_get_hmtc(TrueTypeFont* ttf, TrueTypeFontGlyph* glyph, u32 glyph_index)
{
	if (glyph_index < ttf->num_hmtx) {
		glyph->aw = ttf->hmtx[glyph_index].aw;
		glyph->lsb = ttf->hmtx[glyph_index].lsb;
		return;
	}
	glyph->aw = ttf->hmtx[ttf->num_hmtx - 1].aw;
	glyph->lsb = ttf->lsbs[glyph_index - ttf->num_hmtx];
}

void ttf_glyph_create_deep_copy(TrueTypeFont* ttf, u32 src_index, TrueTypeFontGlyph** dst)
{
	TrueTypeFontGlyph* src_glyph = ttf->glyphs + src_index;
	if (src_glyph->parsed != 1) { exit(0); }
	
	u64 size = sizeof(TrueTypeFontGlyph);
	size += src_glyph->num_contours * U16_SIZE;
	size += 2 * src_glyph->num_points * U16_SIZE + src_glyph->num_points;
	// assert(size != 0);

	(*dst) = (TrueTypeFontGlyph*) malloc(size);

	if (src_glyph->num_contours == 0) { memset((*dst), 0, size); (*dst)->parsed = 1; return; }
	
	void* tmp_dst = (void*) (*dst);
	u64 glyph_tail_offset = sizeof(TrueTypeFontGlyph);

	(*dst)->parsed = src_glyph->parsed;
	(*dst)->num_contours = src_glyph->num_contours;
	(*dst)->x_min = src_glyph->x_min;
	(*dst)->y_min = src_glyph->y_min;
	(*dst)->x_max = src_glyph->x_max;
	(*dst)->y_max = src_glyph->y_max;

	(*dst)->end_pts_of_contours = (u16*) (tmp_dst + glyph_tail_offset);
	glyph_tail_offset += src_glyph->num_contours * U16_SIZE;
	memcpy((*dst)->end_pts_of_contours, src_glyph->end_pts_of_contours,
		   src_glyph->num_contours * U16_SIZE);

	(*dst)->num_points = src_glyph->num_points;

	(*dst)->flags = (u8*) (tmp_dst + glyph_tail_offset);
	glyph_tail_offset += src_glyph->num_points;
	memcpy((*dst)->flags, src_glyph->flags, src_glyph->num_points);

	(*dst)->pts_x = (i16*) (tmp_dst + glyph_tail_offset);
	glyph_tail_offset += src_glyph->num_points * U16_SIZE;
	memcpy((*dst)->pts_x, src_glyph->pts_x, src_glyph->num_points * U16_SIZE);

	(*dst)->pts_y = (i16*) (tmp_dst + glyph_tail_offset);
	glyph_tail_offset += src_glyph->num_points * U16_SIZE;
	memcpy((*dst)->pts_y, src_glyph->pts_y, src_glyph->num_points * U16_SIZE);

	(*dst)->aw = src_glyph->aw;
	(*dst)->lsb = src_glyph->lsb;
}

i32 ttf_glyph_index_get(TrueTypeFont* ttf, u16 code_point)
{
	assert(ttf != NULL);
	
	u16 seg_count = ttf->seg_count_2 / 2;
	i32 index = -1;
	u16* ptr;
	for (u16 i = 0; i < seg_count; i++) {
		if (code_point <= ttf->end_code[i]) { index = i; break; }
	}
	if (index == -1) { return 0; }	// ERROR, NO SUCH CODE POINT
	if (ttf->start_code[index] < code_point) {
		if (ttf->id_range_offset != 0) {
			ptr = &ttf->id_range_offset[index] + ttf->id_range_offset[index] / 2;
			ptr += (code_point - ttf->start_code[index]);
			if (ptr == NULL) { return 0; }
			return (*ptr + ttf->id_delta[index]) % 65536;
		}
		else {
			return (code_point + ttf->id_delta[index]) % 65536;
		}
	}

	return 0;
}

void* ttf_create_bitmap(TrueTypeFont* ttf, char c, u32 width, u32 height)
{
	assert(ttf != NULL);
	assert(width != 0 && height != 0);
	
	char* bmp = (char*) malloc(width * height);
	memset(bmp, 0, width * height);

	i32 glyph_index = ttf_glyph_index_get(ttf, c);
	TrueTypeFontGlyph* glyph = ttf->glyphs + glyph_index;
	i16 bbox = MAX(ttf->x_max - ttf->x_min, ttf->y_max - ttf->y_min);
	// s32 scale = (float) height / (float) ttf->units_per_em;
	s32 scale = (float) height / (float) bbox;
	// s32 descent = scale * ttf->descent;
	s32 descent = scale * ttf->x_min;
	s32 lsb = (width / 2.0f) + 0.5f * scale * glyph->aw;

	Array lines, points;
	arr_init(&lines, sizeof(line2f));
	arr_init(&points, sizeof(vec2f));
	
	s32 x0, y0, mx, my;
	u32 min_y = height;
	u32 max_y = 0;
	u32 contour_counter = 0;
	for (u16 i = 0; i < glyph->num_points; i++) {
		x0 = width - (scale * glyph->pts_x[i]) + lsb;
		y0 = height - (scale * glyph->pts_y[i]) + descent;
		if (y0 < min_y) min_y = y0;
		if (max_y < y0) max_y = y0;
		vec2f tmp_point0 = { x0, y0 };
		if (glyph->flags[i] &0x01) { arr_add(&points, &tmp_point0); }
		else if (!(glyph->flags[i] & 0x01) && !(glyph->flags[i + 1] & 0x01)) {
			mx = (glyph->pts_x[i] + glyph->pts_x[i + 1]) / 2;
			my = (glyph->pts_y[i] + glyph->pts_y[i + 1]) / 2;
			x0 = width - (scale * mx) + lsb;
			y0 = height - (scale * my) + descent;
			vec2f tmp_point1 = { x0, y0 };
			arr_add(&points, &tmp_point1);
		}

		if (glyph->end_pts_of_contours[contour_counter] == i) {
			contour_counter++;
			for (u32 j = 0; j < points.size; j++) {
				vec2f* point0 = (vec2f*) arr_get(&points, j);
				vec2f* point1 = (vec2f*) arr_get(&points, j + 1);
				line2f line = { *point0, *point1 };
				/*
				logd("[ttf] line : { %f, %f }, { %f, %f }\n",
					 point0->x, point0->y, point1->x, point1->y);
				*/
				arr_add(&lines, &line);
			}
			arr_clean(&points);
		}
	}
	arr_free(&points);

	s32 dx, dy, intersection, smaller_y, bigger_y, m0, m1;
	Array intersections;
	arr_init(&intersections, sizeof(s32));
	for (u32 i = min_y; i < max_y; i++) {
		for (u32 j = 0; j < lines.size; j++) {
			line2f* line = (line2f*) arr_get(&lines, j);
			
			smaller_y = MIN(line->p0.y, line->p1.y);
			bigger_y = MAX(line->p0.y, line->p1.y);
			if (i <= smaller_y) continue;
			if (i > bigger_y) continue;

			dx = line->p1.x - line->p0.x;
			dy = line->p1.y - line->p0.y;
			if (dy == 0) continue;

			if (dx == 0) { intersection = line->p0.x; }
			else { intersection = (i - line->p0.y) * (dx / dy) + line->p0.x; }
			arr_add(&intersections, &intersection);
		}

		qsort(intersections.data, intersections.size, sizeof(s32), cmp_floats_callback);
		for (u32 k = 0; k < intersections.size; k += 2) {
			m0 = *((s32*) arr_get(&intersections, k));
			m1 = *((s32*) arr_get(&intersections, k + 1));
			for (u32 m = m0; m < m1; m++) {
				bmp[m + i * width] = 0xff;
			}
		}
		arr_clean(&intersections);
	}
	arr_free(&intersections);
	arr_free(&lines);
	
	return bmp;
}

void* ttf_create_font_atlas(TrueTypeFont* ttf, const char* characters, u32 point_size)
{
	/*
	  	USAGE:

		// TODO
		
	 */
	
	u32 width = 0;
	u32 height = point_size;

	s32 scale = (s32) point_size / (s32) ttf->units_per_em;
	s32 descent = scale * ttf->descent;
	s32 x0, y0, mx, my;
	u32 min_y = height;
	u32 max_y = 0;
	u32 contour_counter;
	u32 total_aw = 0;

	Array lines, points, p_glyphs;
	arr_init(&lines, sizeof(line2f));
	arr_init(&points, sizeof(vec2f));
	arr_init(&p_glyphs, sizeof(TrueTypeFontGlyph*));
	
	for (u32 i = 0; i < strlen(characters); i++) {
		TrueTypeFontGlyph* glyph = ttf->glyphs + ttf_glyph_index_get(ttf, characters[i]);
		arr_add(&p_glyphs, &glyph);
		width += glyph->aw;
	}
	width = width * scale;

	for (u32 i = 0; i < p_glyphs.size; i++) {
		contour_counter = 0;
		TrueTypeFontGlyph* glyph = *((TrueTypeFontGlyph**) arr_get(&p_glyphs, i));

		for (u16 i = 0; i < glyph->num_points; i++) {
			x0 = (width - total_aw) - (scale * glyph->pts_x[i]) + (scale * glyph->x_min);
			y0 = height - (scale * glyph->pts_y[i]) + descent;
			if (y0 < min_y) min_y = y0;
			if (max_y < y0) max_y = y0;
			vec2f tmp_point0 = { x0, y0 };
			if (glyph->flags[i] &0x01) { arr_add(&points, &tmp_point0); }
			else if (!(glyph->flags[i] & 0x01) && !(glyph->flags[i + 1] & 0x01)) {
				mx = (glyph->pts_x[i] + glyph->pts_x[i + 1]) / 2;
				my = (glyph->pts_y[i] + glyph->pts_y[i + 1]) / 2;
				x0 = (width - total_aw) - (scale * mx) + (scale * glyph->x_min);
				y0 = height - (scale * my) + descent;
				vec2f tmp_point1 = { x0, y0 };
				arr_add(&points, &tmp_point1);
			}

			if (glyph->end_pts_of_contours[contour_counter] == i) {
				contour_counter++;
				for (u32 j = 0; j < points.size; j++) {
					vec2f* point0 = (vec2f*) arr_get(&points, j);
					vec2f* point1 = (vec2f*) arr_get(&points, j + 1);
					line2f line = { *point0, *point1 };
					arr_add(&lines, &line);
				}
				arr_clean(&points);
			}
		}
		arr_clean(&points);
		total_aw += scale * glyph->aw;
	}
	arr_free(&p_glyphs);
	arr_free(&points);

	// width = width * scale;
	char* bmp = (char*) malloc(width * height);

	s32 dx, dy, intersection, smaller_y, bigger_y, m0, m1;
	Array intersections;
	arr_init(&intersections, sizeof(s32));
	for (u32 i = min_y; i < max_y; i++) {
		for (u32 j = 0; j < lines.size; j++) {
			line2f* line = (line2f*) arr_get(&lines, j);
			
			smaller_y = MIN(line->p0.y, line->p1.y);
			bigger_y = MAX(line->p0.y, line->p1.y);
			if (i <= smaller_y) continue;
			if (i > bigger_y) continue;

			dx = line->p1.x - line->p0.x;
			dy = line->p1.y - line->p0.y;
			if (dy == 0) continue;

			if (dx == 0) { intersection = line->p0.x; }
			else { intersection = (i - line->p0.y) * (dx / dy) + line->p0.x; }
			arr_add(&intersections, &intersection);
		}

		qsort(intersections.data, intersections.size, sizeof(s32), cmp_floats_callback);
		for (u32 k = 0; k < intersections.size; k += 2) {
			m0 = *((s32*) arr_get(&intersections, k));
			m1 = *((s32*) arr_get(&intersections, k + 1));
			for (u32 m = m0; m < m1; m++) {
				bmp[m + i * width] = 0xff;
			}
		}
		arr_clean(&intersections);
	}
	arr_free(&intersections);
	arr_free(&lines);

	// bmp_save("resources/atlas.bmp", bmp, width, height, 1);
	return bmp;
}

s32 f2fot14_to_float_2(u16 f2dot14) {
	i8 tmp1 = (0xc000 & f2dot14) >> 14;
	if (tmp1 == 2) {
		tmp1 = -2;
	}
	else if (tmp1 == 3) {
		tmp1 = -1;
	}
	
	s32 scale = (s32) tmp1;
	f2dot14 &= 0x3fff;
	scale += (s32) (f2dot14 / 0x4000);
	return scale;
}
