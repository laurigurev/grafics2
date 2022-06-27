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

#define TTF_WIN_PLATFORM_ID 3
#define TTF_FLAG_REPEAT 0x08

s32 f2fot14_to_float(u16 f2dot14);

int ttf_load2(TrueTypeFont* ttf, const char* font_path)
{
	u32 buffer_size;
	char* buffer = file_read(font_path, &buffer_size);

	u16 num_tables = ENDIAN_WORD(*((u16*) (buffer + U32_SIZE)));

	u64 table_size = 4 * U32_SIZE;
	u64 offset = U32_SIZE + 4 * U16_SIZE;	// sizeof 'offset subtable'
	TrueTypeFontTable tables[] = {
		(TrueTypeFontTable) { 0, 0 },	// head
		(TrueTypeFontTable) { 0, 0 },	// maxp
		(TrueTypeFontTable) { 0, 0 },	// cmap
		(TrueTypeFontTable) { 0, 0 },	// loca
		(TrueTypeFontTable) { 0, 0 }	// glyf
	};
	u32 tables_found = 0;
	for (u16 i = 0; i < num_tables; i++) {
		u32 tag = ENDIAN_DWORD(*((u32*) (buffer + offset)));
		// u32 checksum = ENDIAN_DWORD(*((u32*) (buffer + offset + U32_SIZE)));
		u32 offset = ENDIAN_DWORD(*((u32*) (buffer + offset + 2 * U32_SIZE)));
		u32 length = ENDIAN_DWORD(*((u32*) (buffer + offset + 3 * U32_SIZE)));

		switch (tag) {
		case TTF_HEAD_TAG:
			tables[0] = (TrueTypeFontTable) { offset, length };
			tables_found++;
			break;
		case TTF_MAXP_TAG:
			tables[1] = (TrueTypeFontTable) { offset, length };
			tables_found++;
			break;
		case TTF_CMAP_TAG:
			tables[2] = (TrueTypeFontTable) { offset, length };
			tables_found++;
			break;
		case TTF_LOCA_TAG:
			tables[3] = (TrueTypeFontTable) { offset, length };
			tables_found++;
			break;
		case TTF_GLYF_TAG:
			tables[4] = (TrueTypeFontTable) { offset, length };
			tables_found++;
			break;
		}
		offset += table_size;
	}

	if (tables_found != 5) { return -1; }

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
		u64 cmap_offset = 2 * U16_SIZE;
		u64 subtable_size = 2 * U16_SIZE + U32_SIZE;
		for (u16 i = 0; i < num_subtables; i++) {
			u16 platform_id = ENDIAN_WORD(
					  *((u16*) (buffer + cmap_offset + subtable_size * i + 0)));
			u16 platform_specific_id = ENDIAN_WORD(
 					   *((u16*) (buffer + cmap_offset + subtable_size * i + U16_SIZE)));
			u32 format_offset = ENDIAN_DWORD(
					   *((u32*) (buffer + cmap_offset + subtable_size * i + 2 * U16_SIZE)));

			if (platform_id == TTF_WIN_PLATFORM_ID && platform_specific_id == 4) {
				format4_ptr = (void*) (buffer + tables[2].offset + format_offset);
				seg_count = (u16*) (format4_ptr + 3 * U16_SIZE);
				seg_count = ENDIAN_WORD(tmp_seq_count);
				seg_count /= 2;
			}
		}
	}

	if (format4_ptr == NULL) { return -1; }

	u64 size = 0;
	size += sizeof(TrueTypeFont);
	size += U16_SIZE * tmp_seg_count * 4;
	size += U32_SIZE * tmp_num_glyphs;
	size += sizeof(TrueTypeFontGlyph) * tmp_num_glyphs;
	size += U16_SIZE * 3 * tmp_num_glyphs * max_points;
	size += tmp_num_glyphs * max_points;

	ttf = (TrueTypeFont*) malloc(size);
	u64 alloc_tail_offset = sizeof(TrueTypeFont);

	i16 index_to_loc_format = -1;
	// fill ttf with data
	{
		// head
		u64 head_offset = tables[0].offset + 3 * U32_SIZE + U16_SIZE;
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
		u64 len = lenght - 3 * 2 * seg_count;
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
		// memcpy(ttf->loca, buffer + loca_offset, ttf->num_glyphs * U32_SIZE);
		alloc_tail_offset += ttf->num_glyphs * U32_SIZE;

		if (index_to_loc_forma) {
			for (u16 i = 0; i < ttf->num_glyphs; i++) {
				u32* location = buffer + loca_offset + i * U32_SIZE
				ttf->loca[i] = ENDIAN_DWORD(*location);
			}
		} else {
			for (u16 i = 0; i < ttf->num_glyphs; i++) {
				u32 location = (u32) *(buffer + loca_offset + i * U16_SIZE)
				ttf->loca[i] = ENDIAN_DWORD(location);
			}
		}
		
	}

	{
		// glyf
		u64 glyf_offset = tables[4].offset;
		ttf->glyphs = buffer + glyf_offset;
		alloc_tail_offset += ttf->num_glyphs * sizeof(TrueTypeFontGlyph);
		// memset everything to 0
		memset(ttf->glyphs, 0, ttf->num_glyphs * sizeof(TrueTypeFontGlyph));

		for (u16 i = 0; i < ttf->num_glyphs - 1; i++) {
			u32 offset = ttf->loca[i];
			u32 next_offset = ttf->loca[i + 1];

			if (offset - next_offset == 0) {
				ttf->glyphs[0] = (TrueTypeFontGlyph) {
					0, 0, 0, 0, 0, NULL, 0, NULL, NULL, NULL
				};
				continue;
			}
			
			// load glyph
			// ttf_glyph_load(ttf, i, buffer + glyf_offset, alloc_tail_offset);
		}
	}
	
	file_free(buffer);
	return 1;
}

void ttf_free2(TrueTypeFont* ttf)
{
	if (ttf) { free(ttf); }
}

void ttf_glyph_load(TrueTypeFont* ttf, u32 glyph_index, void* buffer,
					u64* ttf_buffer_tail_offset)
{
	u32 glyph_offset = ttf->loca[glyph_index];
	TrueTypeFontGlyph* glyph = ttf->glyphs + glyph_index;

	if (glyph->parser == 1) { return; }

	glyph->parsed = 1;
	glyph->num_contours = ENDIAN_WORD(*((i16*) (buffer + glyph_offset)));
	glyp_offset += U16_SIZE;
	glyph->x_min = ENDIAN_WORD(*((i16*) (buffer + glyph_offset)));
	glyp_offset += U16_SIZE;
	glyph->y_min = ENDIAN_WORD(*((i16*) (buffer + glyph_offset)));
	glyp_offset += U16_SIZE;
	glyph->x_max = ENDIAN_WORD(*((i16*) (buffer + glyph_offset)));
	glyp_offset += U16_SIZE;
	glyph->y_max = ENDIAN_WORD(*((i16*) (buffer + glyph_offset)));
	glyp_offset += U16_SIZE;

	if (0 <= glyph->num_contours) {
		// simple glyph
		glyph->end_pts_of_contours = ((void*) ttf) + *ttf_buffer_tail_offset;
		memcpy(glyph->end_pts_of_contours, buffer + glyph_offset,
			   glyph->num_contours * U16_SIZE);
		glyph_offset += glyph->num_contours * U16_SIZE;
		*ttf_buffer_tail_offset += glyph->num_contours * U16_SIZE;
		
		for (i16 i = 0; i < glyph->num_contours; i++) {
			glyph->end_ptr_of_contours[i] = ENDIAN_WORD(glyph->end_ptr_of_contours[i]);
		}

		{
			// skip intructions
			u16 instruction_len = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
			glyph_offset += U16_SIZE;
			glyph_offset += instruction_len * U16_SIZE;
		}

		glyph->num_points = glyph->end_pts_of_contours[glyph->num_contours - 1] + 1;

		glyph->flags = ((void*) ttf) + *ttf_buffer_tail_offset;
		*ttf_buffer_tail_offset += glyph->num_points;
		u8* tmp_flags = (u8*) (buffer + glyph_offset);
		u64 flags_len = 0;
		u8 tmp_flag0, repeat_count;
		for (u16 i = 0; i < glyph->num_points; i++) {
			tmp_flag0 = *(tmp_flags + flags_len);
			flags_len++;
			glyph->flags[i] = tmp_flag0;
			if (tmp_flag0 & TTF_FLAG_REPEAT) {
				repeat_count = *(tmp_flags + flags_len);
				flags_len++;
				for (u8 j = 0; j < repeat_count; j++) {
					i++;
					glyph->flags[i] = tmp_flag0;
				}
			}
		}
		glyph_offset += flags_len;

		glyph->pts_x = ((void*) ttf) + *ttf_buffer_tail_offset;
		*ttf_buffer_tail_offset += glyph->num_points * U16_SIZE;
		glyph->pts_y = ((void*) ttf) + *ttf_buffer_tail_offset;
		*ttf_buffer_tail_offset += glyph->num_points * U16_SIZE;

		// void* pts_pos = buffer + glyph_offset;
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
	} else if (glyph->num_contours < 0) {
		// compound glyph

		// TODO: fix stuff involving aw, lsb
		/*
		u16 flags, tmp_glyph_index, x, y, f2dot14;
		Array glyph_indices;
		arr_init(&glyph_indices, U16_SIZE);

		do {
			flags = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
			glyph_offset += U16_SIZE;
			
			tmp_glyph_index = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
			glyph_offset += U16_SIZE;

			// TODO: change the way new glyphs are loaded, because this function need a copy
			// TODO: do a deep copy of the glyf
			ttf_glyph_load(ttf, new_glyph_index, buffer, ttf_buffer_tail_offset);
			TrueTypeFontGlyph* component = ttf->glyphs + new_glyph_index;
			arr_add(&glyph_indices, &tmp_glyph_index);

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
				s32 scale = f2fot14_to_float(f2dot14);
				for (u16 i = 0; i < component->num_points; i++) {
					component->pts_x[i] = (i16) component->pts_y[i] * scale;
					component->pts_y[i] = (i16) component->pts_y[i] * scale;
				}
			} else if (flags & 0x0040) {
				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 x_scale = f2fot14_to_float(f2dot14);
			
				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 y_scale = f2fot14_to_float(f2dot14);

				for (u16 i = 0; i < component->num_points; i++) {
					component->pts_x[i] = (i16) component->pts_x[i] * x_scale;
					component->pts_y[i] = (i16) component->pts_y[i] * y_scale;
				}
			} else if (flags & 0x0080) {
				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 transform_x = f2fot14_to_float(f2dot14);

				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 transform_01 = f2fot14_to_float(f2dot14);

				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 transform_10 = f2fot14_to_float(f2dot14);

				f2dot14 = ENDIAN_WORD(*((u16*) (buffer + glyph_offset)));
				glyph_offset += U16_SIZE;
				s32 transform_y = f2fot14_to_float(f2dot14);

				for (u16 i = 0; i < component->num_points; i++) {
					component->pts_x[i] = (i16) component->pts_x[i] * transform_x +
						                  component->pts_y[i] * transform_01;
					component->pts_y[i] = (i16) component->pts_y[i] * transform_y +
						                  component->pts_x[i] * transform_10;
				}
			}
		} while (flags & 0x0020);

		for (u32 i = 0; i < glyph_indices.size; i++) {
			
		}
		*/
	}
}

s32 f2fot14_to_float(u16 f2dot14) {
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
