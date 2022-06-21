#include "grafics2.h"

#define ttf_logi(...) logi(__VA_ARGS__)
#define ttf_logd(...) logd(__VA_ARGS__)
#define ttf_logw(...) logw(__VA_ARGS__)
#define ttf_loge(...) loge(__VA_ARGS__)

#define ENDIAN_WORD(a) ((((0xFF00 & a) >> 8) | ((0x00FF & a) << 8)))
#define ENDIAN_DWORD(a) \
	(((0xFF000000 & a) >> 24) | ((0x00FF0000 & a) >> 8) |		\
	 ((0x0000FF00 & a) << 8) | ((0x000000FF & a) << 24))

#define TTF_HEAD_TAG 0x68656164
#define TTF_HHEA_TAG 0x68686561
#define TTF_MAXP_TAG 0x6d617870
#define TTF_HMTX_TAG 0x686d7478
#define TTF_CMAP_TAG 0x636d6170
#define TTF_LOCA_TAG 0x6c6f6361
#define TTF_GLYF_TAG 0x676c7966

#define TTF_WIN_PLATFORM_ID 3
#define TTF_GLYPH_TABLE_SIZE 256

// #define TTF_MAGIC_NUMBER 0x5f0f3cf5
#define TTF_MAGIC_NUMBER 0xf53c0f5f

void ttf_load(ttf_core* ttf)
{
	uint32_t file_size;
	char* file = file_read("resources/calibri.ttf", &file_size);
	
	ttf_table_directory* td = (ttf_table_directory*) malloc(sizeof(ttf_table_directory));
	memcpy(td, file, sizeof(ttf_table_directory));
	
	td->numTables = ENDIAN_WORD(td->numTables);
	td->searchRange = ENDIAN_WORD(td->searchRange);
	td->entrySelector = ENDIAN_WORD(td->entrySelector);
	td->rangeShift = ENDIAN_WORD(td->rangeShift);
	
	ttf_header* ptr = (ttf_header*) (file + sizeof(ttf_table_directory));

	// order of headers: head, hhea maxp, hmtx, cmap, loca, glyf
	ttf_header* headers = (ttf_header*) calloc(7, sizeof(ttf_header));

	for (uint16_t i = 0; i < td->numTables; i++) {
		char tag[5] = { '\0' };
		memcpy(tag, &ptr->tag, 4);
		
		ttf_header header;
		memcpy(&header, ptr, sizeof(ttf_header));

		header.tag = ENDIAN_DWORD(header.tag);
		header.checkSum = ENDIAN_DWORD(header.checkSum);
		header.offset = ENDIAN_DWORD(header.offset);
		header.length = ENDIAN_DWORD(header.length);
		
		switch (header.tag) {
			case TTF_HEAD_TAG:
				ttf_logi("[ttf_core] found 'head' table\n");
				headers[0] = header;
				break;
			case TTF_HHEA_TAG:
				ttf_logi("[ttf_core] found 'hhea' table\n");
				headers[1] = header;
				break;
			case TTF_MAXP_TAG:
				ttf_logi("[ttf_core] found 'maxp' table\n");
				headers[2] = header;
				break;
			case TTF_HMTX_TAG:
				ttf_logi("[ttf_core] found 'hmtx' table\n");
				headers[3] = header;
				break;
			case TTF_CMAP_TAG:
				ttf_logi("[ttf_core] found 'cmap' table\n");
				headers[4] = header;
				break;
			case TTF_LOCA_TAG:
				ttf_logi("[ttf_core] found 'loca' table\n");
				headers[5] = header;
				break;
			case TTF_GLYF_TAG:
				ttf_logi("[ttf_core] found 'glyf' table\n");
				headers[6] = header;
				break;
			default:
				ttf_logi("[ttf_core] found '%s' table, unsupported\n", tag);
		}

		ptr++;
	}

	flushl();

	if (headers[0].tag) { ttf_head_load(ttf, file + headers[0].offset); }
	else { ttf_loge("[ttf_core] 'head' table not found\n"); exit(0); }	
	if (headers[1].tag) { ttf_hhea_load(ttf, file + headers[1].offset); }
	else { ttf_loge("[ttf_core] 'hhea' table not found\n"); exit(0); }
	if (headers[2].tag) { ttf_maxp_load(ttf, file + headers[2].offset); }
	else { ttf_loge("[ttf_core] 'maxp' table not found\n"); exit(0); }
	if (headers[3].tag) { ttf_hmtx_load(ttf, file + headers[3].offset); }
	else { ttf_loge("[ttf_core] 'hmtx' table not found\n"); exit(0); }
	if (headers[4].tag) { ttf_cmap_load(ttf, file + headers[4].offset); }
	else { ttf_loge("[ttf_core] 'cmap' table not found\n"); exit(0); }
	if (headers[5].tag) { ttf_loca_load(ttf, file + headers[5].offset); }
	else { ttf_loge("[ttf_core] 'loca' table not found\n"); exit(0); }
	if (headers[6].tag) { ttf_glyf_load(ttf, file + headers[6].offset); }
	else { ttf_loge("[ttf_core] 'glyf' table not found\n"); exit(0); }
	
	free(headers);
	free(td);
	file_free(file);
}

void ttf_free(ttf_core* ttf)
{
	ttf_glyf_free(ttf);
	ttf_loca_free(ttf);
	ttf_cmap_free(ttf);
	ttf_hmtx_free(ttf);
	free(ttf->maxp);
	free(ttf->hhea);
	free(ttf->head);
}

void ttf_head_load(ttf_core* ttf, void* file)
{
	// TODO: deal with errors
	// NOTE: commented varibles are not changed to correct endian form

	ttf->head = (ttf_head*) malloc(sizeof(ttf_head));
	ttf_head* head = ttf->head;
	memcpy(head, file, sizeof(ttf_head));

	head->version = ENDIAN_DWORD(head->version);
	head->font_revision = ENDIAN_DWORD(head->font_revision);
	head->checksum_adjustment = ENDIAN_DWORD(head->checksum_adjustment);

	if (head->magic_number != TTF_MAGIC_NUMBER) {
		ttf_loge("[ttf_head] incorrect magic number\n");
	}

	head->flags = ENDIAN_WORD(head->flags);
	head->units_per_em = ENDIAN_WORD(head->units_per_em);

	// created
	// modified

	head->x_min = ENDIAN_WORD(head->x_min);
	head->y_min = ENDIAN_WORD(head->y_min);
	head->x_max = ENDIAN_WORD(head->x_max);
	head->y_max = ENDIAN_WORD(head->y_max);

	head->mac_style = ENDIAN_WORD(head->mac_style);
	head->lowest_rec_ppem = ENDIAN_WORD(head->lowest_rec_ppem);
	head->font_direction_hint = ENDIAN_WORD(head->font_direction_hint);
	head->index_to_loc_format = ENDIAN_WORD(head->index_to_loc_format);
	head->glyph_data_format = ENDIAN_WORD(head->glyph_data_format);

	ttf_logi("[ttf_core] 'head' table loaded\n");
}

void ttf_hhea_load(ttf_core* ttf, void* file)
{
	// TODO: deal with errors

	ttf->hhea = (ttf_hhea*) malloc(sizeof(ttf_hhea));
	ttf_hhea* hhea = ttf->hhea;
	memcpy(hhea, file, sizeof(ttf_hhea));

	hhea->version = ENDIAN_DWORD(hhea->version);
	hhea->ascent = ENDIAN_WORD(hhea->ascent);
	hhea->descent = ENDIAN_WORD(hhea->descent);
	hhea->line_gap = ENDIAN_WORD(hhea->line_gap);
	hhea->advance_with_max = ENDIAN_WORD(hhea->advance_with_max);
	hhea->min_left_side_bearing = ENDIAN_WORD(hhea->min_left_side_bearing);
	hhea->min_right_side_bearing = ENDIAN_WORD(hhea->min_right_side_bearing);
	hhea->x_max_extent = ENDIAN_WORD(hhea->x_max_extent);
	hhea->caret_slope_rise = ENDIAN_WORD(hhea->caret_slope_rise);
	hhea->caret_slope_run = ENDIAN_WORD(hhea->caret_slope_run);
	hhea->caret_offset = ENDIAN_WORD(hhea->caret_offset);
	
    // hhea->reserved[4]
	
	hhea->metric_date_format = ENDIAN_WORD(hhea->metric_date_format);
	hhea->num_of_long_hor_metrics = ENDIAN_WORD(hhea->num_of_long_hor_metrics);

	ttf_logi("[ttf_core] 'hhea' table loaded\n");
}

void ttf_maxp_load(ttf_core* ttf, void* file)
{
	// TODO: deal with errors

	ttf->maxp = (ttf_maxp*) malloc(sizeof(ttf_maxp));
	ttf_maxp* maxp = ttf->maxp;
	memcpy(maxp, file, sizeof(ttf_maxp));
	
	maxp->version = ENDIAN_DWORD(maxp->version);
	maxp->num_glyphs = ENDIAN_WORD(maxp->num_glyphs);
	maxp->max_points = ENDIAN_WORD(maxp->max_points);
	maxp->max_contours = ENDIAN_WORD(maxp->max_contours);
	maxp->max_component_points = ENDIAN_WORD(maxp->max_component_points);
	maxp->max_component_contours = ENDIAN_WORD(maxp->max_component_contours);
	maxp->max_zones = ENDIAN_WORD(maxp->max_zones);
	maxp->max_twilight_points = ENDIAN_WORD(maxp->max_twilight_points);
	maxp->max_storage = ENDIAN_WORD(maxp->max_storage);
	maxp->max_function_defs = ENDIAN_WORD(maxp->max_function_defs);
	maxp->max_instruction_defs = ENDIAN_WORD(maxp->max_instruction_defs);
	maxp->max_stack_elements = ENDIAN_WORD(maxp->max_stack_elements);
	maxp->max_size_of_instructions = ENDIAN_WORD(maxp->max_size_of_instructions);
	maxp->max_component_elements = ENDIAN_WORD(maxp->max_component_elements);
	maxp->max_component_depth = ENDIAN_WORD(maxp->max_component_depth);

	ttf_logi("[ttf_core] 'maxp' table loaded\n");
}

void ttf_hmtx_load(ttf_core* ttf, void* file)
{
	// TODO: deal with errors

	ttf->hmtx = (ttf_hmtx*) malloc(sizeof(ttf_hmtx));
	ttf_hmtx* hmtx = ttf->hmtx;

	uint16_t num_lhm = ttf->hhea->num_of_long_hor_metrics;
	uint16_t variable = ttf->maxp->num_glyphs - num_lhm;

	hmtx->h_metrics = (ttf_long_hor_metric*) malloc(sizeof(ttf_long_hor_metric) * num_lhm);	
	ttf_long_hor_metric* h_metric = hmtx->h_metrics;
	memcpy(h_metric, file, sizeof(ttf_long_hor_metric) * num_lhm);
	
	for (uint16_t i = 0; i < num_lhm; i++) {
		h_metric->advance_width = ENDIAN_WORD(h_metric->advance_width);
		h_metric->left_side_bearing = ENDIAN_WORD(h_metric->left_side_bearing);
		h_metric++;
	}

	if (variable != 0) {
		hmtx->left_side_bearing = (int16_t*) malloc(sizeof(int16_t) * variable);
		memcpy(hmtx->left_side_bearing, file + sizeof(ttf_long_hor_metric) * num_lhm,
			   sizeof(uint32_t) * variable);

		for (uint16_t i = 0; i < variable; i++) {
			hmtx->left_side_bearing[i] = ENDIAN_WORD(hmtx->left_side_bearing[i]);
		}		
	}
	else { hmtx->left_side_bearing = NULL; }

	ttf_logi("[ttf_core] 'hmtx' table loaded\n");
}

void ttf_cmap_load(ttf_core* ttf, void* file)
{
	// TODO: deal with errors

	ttf->cmap = (ttf_cmap*) malloc(sizeof(ttf_cmap));
	ttf_cmap* cmap = ttf->cmap;
	memcpy(&cmap->index, file, sizeof(ttf_cmap_index));

	cmap->index.version = ENDIAN_WORD(cmap->index.version);
	cmap->index.number_subtables = ENDIAN_WORD(cmap->index.number_subtables);

	uint16_t num_subtables = cmap->index.number_subtables;

	uint32_t tables_size = sizeof(ttf_cmap_estable) * num_subtables;
	cmap->tables = (ttf_cmap_estable*) malloc(tables_size);
	ttf_cmap_estable* table = cmap->tables;
	memcpy(table, file + sizeof(ttf_cmap_index), tables_size);
	
	for (uint16_t i = 0; i < num_subtables; i++) {
		table[i].platform_id = ENDIAN_WORD(table[i].platform_id);
		table[i].platform_specific_id = ENDIAN_WORD(table[i].platform_specific_id);
		table[i].offset = ENDIAN_DWORD(table[i].offset);
	}

	// TODO: ttf_cmap_format?

	for (uint16_t i = 0; i < num_subtables; i++) {
		if (table[i].platform_id == TTF_WIN_PLATFORM_ID) {
			uint16_t format = *((uint16_t*) (file + table[i].offset));
			format = ENDIAN_WORD(format);

			switch (format) {
				case 4:
					ttf_logi("[ttf_cmap] format 4 subtable chosen\n");
					ttf_cmap_format4_table_load(cmap, file +  table[i].offset);
					break;
				default:
					ttf_logi("[ttf_cmap] format of type %hu not supported\n", format);
			}
		}
	}

	ttf_logi("[ttf_core] 'cmap' table loaded\n");
}

void ttf_loca_load(ttf_core* ttf, void* file)
{
	// TODO: deal with errors
	
	uint16_t num_glyphs = ttf->maxp->num_glyphs;
	ttf->loca = (uint32_t*) malloc(sizeof(uint32_t) * (num_glyphs + 1));
	memcpy(ttf->loca, file, sizeof(uint32_t) * (num_glyphs + 1));

	for (uint16_t i = 0; i < num_glyphs + 1; i++) {
		ttf->loca[i] = ENDIAN_DWORD(ttf->loca[i]);
	}

	ttf_logi("[ttf_core] 'loca' table loaded\n");
}

void ttf_glyf_load(ttf_core* ttf, void* file)
{
	// TODO: deal with errors
	
	uint16_t num_glyphs = ttf->maxp->num_glyphs;
	ttf->glyf = (ttf_glyf*) calloc(num_glyphs, sizeof(ttf_glyf));

	for (uint16_t i = 0; i < num_glyphs - 1; i++) {
		uint32_t offset = ttf->loca[i];
		uint32_t next_offset = ttf->loca[i + 1];

		if (offset - next_offset == 0) {
			continue;
		}
		
		ttf_glyf_data_load(ttf, ttf->glyf + i, file, offset);
	}

	// load last glyph
	ttf_glyf_data_load(ttf, ttf->glyf + num_glyphs - 1, file, ttf->loca[num_glyphs - 1]);
	
	ttf_logi("[ttf_core] 'glyf' table loaded\n");
}

// NOT IN HEADER
float f2fot14_to_float(uint16_t f2dot14) {
	// TODO: deal with integer conversions
	
	int8_t tmp1 = (0xc000 & f2dot14) >> 14;
	if (tmp1 == 2) {
		tmp1 = -2;
	}
	else if (tmp1 == 3) {
		tmp1 = -1;
	}
	
	float scale = (float) tmp1;
	f2dot14 &= 0x3fff;
	scale += (float) (f2dot14 / 0x4000);
	return scale;
}

void ttf_glyf_data_load(ttf_core* ttf, ttf_glyf* glyf, void* file, uint32_t offset)
{
	ttf_glyf_header* header = &glyf->header;
	memcpy(header, file + offset, sizeof(ttf_glyf_header));

	header->num_of_contours = ENDIAN_WORD(header->num_of_contours);
	header->x_min = ENDIAN_WORD(header->x_min);
	header->y_min = ENDIAN_WORD(header->y_min);
	header->x_max = ENDIAN_WORD(header->x_max);
	header->y_max = ENDIAN_WORD(header->y_max);

	offset += sizeof(ttf_glyf_header);

	if (header->num_of_contours >= 0) {
		// simple glyf description
		ttf_glyf_data* simple = &glyf->data;
		uint16_t ec_size = sizeof(uint16_t) * header->num_of_contours;
		simple->end_contours = (uint16_t*) malloc(ec_size);
		memcpy(simple->end_contours, file + offset, ec_size);
		for (uint16_t i = 0; i < header->num_of_contours; i++) {
			simple->end_contours[i] = ENDIAN_WORD(simple->end_contours[i]);
		}

		offset += ec_size;

	    memcpy(&simple->instruction_len, file + offset, sizeof(uint16_t));
		simple->instruction_len = ENDIAN_WORD(simple->instruction_len);

		offset += sizeof(uint16_t);
		simple->instructions = (uint8_t*) malloc(simple->instruction_len);
		memcpy(simple->instructions, file + offset, simple->instruction_len);

		offset += simple->instruction_len;

		simple->num_points = simple->end_contours[header->num_of_contours - 1];
		simple->num_points++;
		// simple->num_points = ENDIAN_WORD(num_points);

		simple->flags = (uint8_t*) malloc(simple->num_points);
		// memcpy(simple->flags, file + offset, simple->num_points);
		uint8_t* flag_buffer = (uint8_t*) file + offset;
		uint32_t fb_offset = 0;
		for (uint16_t i = 0; i < simple->num_points; i++) {
			uint8_t tmpf1 = *(flag_buffer + fb_offset);
			fb_offset++;
			simple->flags[i] = tmpf1;
			if (tmpf1 & 0x08) {	// TTF_FLAG_REPEAT
				uint8_t tmpi1 = *(flag_buffer + fb_offset);
				fb_offset++;
				for (uint8_t j = 0; j < tmpi1; j++) {
					i++;
					simple->flags[i] = tmpf1;
				}
			}
		}

		offset += fb_offset;
		// offset += simple->num_points;
 
		simple->px = (int16_t*) malloc(sizeof(int16_t) * simple->num_points);
		simple->py = (int16_t*) malloc(sizeof(int16_t) * simple->num_points);

		void* x = file + offset;
		int16_t pos = 0;		// coordinates are relative
		
		for (uint16_t i = 0; i < simple->num_points; i++) {
			if (simple->flags[i] & 0x10) {
				if (simple->flags[i] & 0x02) {
					int16_t tmp = (int16_t) *((uint8_t*) x);
					pos += tmp;
					x++;
					simple->px[i] = pos;
				}
				else {
					simple->px[i] = pos;
				}
			}
			else {
				if (simple->flags[i] & 0x02) {
					int16_t tmp = (int16_t) *((uint8_t*) x);
					pos -= tmp;
					x++;
					simple->px[i] = pos;
				}
				else {
					int16_t tmp = *((int16_t*) x);
					tmp = ENDIAN_WORD(tmp);
					pos += tmp;
					x += 2;
					simple->px[i] = pos;
				}
			}
		}

		pos = 0;		// coordinates are relative
		
		for (uint16_t i = 0; i < simple->num_points; i++) {
			if (simple->flags[i] & 0x20) {
				if (simple->flags[i] & 0x04) {
					int16_t tmp = (int16_t) *((uint8_t*) x);
					pos += tmp;
					x++;
					simple->py[i] = pos;
				}
				else {
					simple->py[i] = pos;
				}
			}
			else {
				if (simple->flags[i] & 0x04) {
					int16_t tmp = (int16_t) *((uint8_t*) x);
					pos -= tmp;
					x++;
					simple->py[i] = pos;
				}
				else {
					int16_t tmp = *((int16_t*) x);
					tmp = ENDIAN_WORD(tmp);
					pos += tmp;
					x += 2;
					simple->py[i] = pos;
				}
			}
		}
	}
	else {
		// compound glyf description
		uint16_t flags;
		uint16_t num_contours = 0;
		uint16_t num_points = 0;
		uint16_t num_instructions = 0;
		Array components;
		
		arr_init(&components, sizeof(ttf_glyf));

		do {
			flags = ENDIAN_WORD(*((uint16_t*) (file + offset)));
			offset += sizeof(uint16_t);
		
			uint16_t glyph_index = ENDIAN_WORD(*((uint16_t*) (file + offset)));
			offset += sizeof(uint16_t);
		
			// get child glyph using loca
			ttf_glyf component;
			ttf_glyf_data_load(ttf, &component, file, ttf->loca[glyph_index]);
	
			// add to array
			arr_add(&components, &component);
			
			uint16_t x, y;
		
			if (flags & 0x0001) {
				x = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				y = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
			}
			else {
				uint8_t tmp = *((uint8_t*) (file + offset));
				offset++;
				x = (uint16_t) tmp;
				tmp = *((uint8_t*) (file + offset));
				offset++;
				y = (uint16_t) tmp;
			}
			// don't know why this
			if (flags & 0x0002) {
				for (uint16_t i = 0; i < component.data.num_points; i++) {
					component.data.px[i] += x;
					component.data.py[i] += y;
				}
			}

			if (flags & 0x0008) {
				uint16_t f2dot14 = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				// deal with f2dot14
				float scale = f2fot14_to_float(f2dot14);
				for (uint16_t i = 0; i < component.data.num_points; i++) {
					component.data.px[i] = (int16_t) component.data.px[i] * scale;
					component.data.px[i] = (int16_t) component.data.py[i] * scale;
				}
			}
			else if (flags & 0x0040) {
				uint16_t f2dot14 = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				float x_scale = f2fot14_to_float(f2dot14);
			
				f2dot14 = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				float y_scale = f2fot14_to_float(f2dot14);

				for (uint16_t i = 0; i < component.data.num_points; i++) {
					component.data.px[i] = (int16_t) component.data.px[i] * x_scale;
					component.data.px[i] = (int16_t) component.data.py[i] * y_scale;
				}
			}
			else if (flags & 0x0080) {
				uint16_t f2dot14 = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				float transform_x = f2fot14_to_float(f2dot14);

				f2dot14 = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				float transform_01 = f2fot14_to_float(f2dot14);

				f2dot14 = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				float transform_10 = f2fot14_to_float(f2dot14);

				f2dot14 = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				float transform_y = f2fot14_to_float(f2dot14);

				for (uint16_t i = 0; i < component.data.num_points; i++) {
					component.data.px[i] = (int16_t) component.data.px[i] * transform_x +
						component.data.py[i] * transform_01;
					component.data.px[i] = (int16_t) component.data.py[i] * transform_y +
						component.data.px[i] * transform_10;
				}
			}

			num_contours += component.header.num_of_contours;
			num_points += component.data.num_points;
			num_instructions += component.data.instruction_len;
			
		} while (flags & 0x0020);

		// copy all components to glyph data
		ttf_glyf_data* compound = &glyf->data;

		header->num_of_contours = num_contours;
		compound->end_contours = (uint16_t*) malloc(sizeof(uint16_t) * num_contours);
		compound->instruction_len = num_instructions;
		compound->instructions = (uint8_t*) malloc(num_instructions);
		compound->num_points = num_points;
		compound->flags = (uint8_t*) malloc(num_points);
		compound->px = (int16_t*) malloc(sizeof(int16_t) * num_points);
		compound->py = (int16_t*) malloc(sizeof(int16_t) * num_points);

		ttf_glyf* tmp = (ttf_glyf*) arr_get(&components, 0);
		uint16_t* contours_ptr = compound->end_contours;
		uint8_t* instructions_ptr = compound->instructions;
		uint8_t* flags_ptr = compound->flags;
		int16_t* px_ptr = compound->px;
		int16_t* py_ptr = compound->py;
		uint16_t contour_delta = 0;
		
		for (uint32_t i = 0; i < components.size; i++) {
			for (uint16_t j = 0; j < tmp->header.num_of_contours; j++) {
				tmp->data.end_contours[j] += contour_delta;
			}
			memcpy(contours_ptr, tmp->data.end_contours,
				   sizeof(uint16_t) * tmp->header.num_of_contours);
			contours_ptr += tmp->header.num_of_contours;
			contour_delta += tmp->header.num_of_contours;

			memcpy(instructions_ptr, tmp->data.instructions, tmp->data.instruction_len);
			instructions_ptr += tmp->data.instruction_len;

			memcpy(flags_ptr, tmp->data.flags, tmp->data.num_points);
			flags_ptr += tmp->data.num_points;

			memcpy(px_ptr, tmp->data.px, sizeof(int16_t) * tmp->data.num_points);
			px_ptr += tmp->data.num_points;

			memcpy(py_ptr, tmp->data.py, sizeof(int16_t) * tmp->data.num_points);
			py_ptr += tmp->data.num_points;
			
			tmp++;
		}

		tmp = (ttf_glyf*) arr_get(&components, 0);
		for (uint32_t i = 0; i < components.size; i++) {
			ttf_glyf_data_free(tmp);
			tmp++;
		}

		arr_free(&components);
	}
}

void ttf_cmap_format4_table_load(ttf_cmap* cmap, void* file)
{
	// TODO: deal with errors
	
	ttf_cmap_format4* format = &cmap->format;
	memcpy(format, file, sizeof(uint16_t) * 7);
	
	format->format = ENDIAN_WORD(format->format);
	format->length = ENDIAN_WORD(format->length);
	format->language = ENDIAN_WORD(format->language);
	format->seg_count_2 = ENDIAN_WORD(format->seg_count_2);
	format->search_range = ENDIAN_WORD(format->search_range);
	format->entry_selector = ENDIAN_WORD(format->entry_selector);
	format->range_shift = ENDIAN_WORD(format->range_shift);

	uint16_t seg_count = format->seg_count_2 / 2;
	uint32_t offset = sizeof(uint16_t) * 7;
	
	format->end_code = (uint16_t*) malloc(sizeof(uint16_t) * seg_count);
	memcpy(format->end_code, file + offset, sizeof(uint16_t) * seg_count);

	offset += sizeof(uint16_t) * seg_count;

	format->reserved_pad = *((uint16_t*) file + offset);
	format->reserved_pad = ENDIAN_WORD(format->reserved_pad);
	if (format->reserved_pad != 0) {
		ttf_logw("[ttf_cmap] format tables reserved pad is nonZero\n");
	}

	offset += sizeof(uint16_t);

	format->start_code = (uint16_t*) malloc(sizeof(uint16_t) * seg_count);
	memcpy(format->start_code, file + offset, sizeof(uint16_t) * seg_count);
	offset += sizeof(uint16_t) * seg_count;
	
	format->id_delta = (uint16_t*) malloc(sizeof(uint16_t) * seg_count);
	memcpy(format->id_delta, file + offset, sizeof(uint16_t) * seg_count);
	offset += sizeof(uint16_t) * seg_count;

	// NOTE: i don't know why this works
	i64 len = format->length - 16 - 3 * 2 * seg_count;
	format->id_range_offset = (u16*) malloc(len);
	memcpy(format->id_range_offset, file + offset, len);
	offset += len;
	
	// format->id_range_offset = (uint16_t*) malloc(sizeof(uint16_t) * seg_count);	
	// memcpy(format->id_range_offset, file + offset, sizeof(uint16_t) * seg_count);
	// offset += sizeof(uint16_t) * seg_count;

	for (uint16_t i = 0; i < seg_count; i++) {
		format->end_code[i] = ENDIAN_WORD(format->end_code[i]);
		format->start_code[i] = ENDIAN_WORD(format->start_code[i]);
		format->id_delta[i] = ENDIAN_WORD(format->id_delta[i]);
		// format->id_range_offset[i] = ENDIAN_WORD(format->id_range_offset[i]);
	}

	// NOTE: i don't know why this works
	for (i64 i = 0; i < len / 2; i++) {
		format->id_range_offset[i] = ENDIAN_WORD(format->id_range_offset[i]);
	}

	// format->glyph_index_array = (uint16_t*) malloc(sizeof(uint16_t) *
	// TTF_GLYPH_TABLE_SIZE);
	// memcpy(format->glyph_index_array, file + offset,
	// sizeof(uint16_t) * TTF_GLYPH_TABLE_SIZE);
}

void ttf_hmtx_free(ttf_core* ttf)
{
	free(ttf->hmtx->h_metrics);
	free(ttf->hmtx->left_side_bearing);
	free(ttf->hmtx);
}

void ttf_cmap_free(ttf_core* ttf)
{
	free(ttf->cmap->tables);
	free(ttf->cmap->format.end_code);
	free(ttf->cmap->format.start_code);
	free(ttf->cmap->format.id_delta);
	free(ttf->cmap->format.id_range_offset);
	// free(ttf->cmap->format.glyph_index_array);
	free(ttf->cmap);
}

void ttf_loca_free(ttf_core* ttf)
{
	free(ttf->loca);
}

void ttf_glyf_data_free(ttf_glyf* glyf)
{
	ttf_glyf_data* data = &glyf->data;

	free(data->end_contours);
	free(data->instructions);
	free(data->flags);
	free(data->px);
	free(data->py);
}

void ttf_glyf_free(ttf_core* ttf)
{
	for (uint16_t i = 0; i < ttf->maxp->num_glyphs; i++) {
		ttf_glyf_data_free(ttf->glyf + i);
	}
	
	free(ttf->glyf);
}

int ttf_glyph_index_get(ttf_core* ttf, u16 code_point)
{
	ttf_cmap_format4* format = &ttf->cmap->format;
	u16 seg_count = format->seg_count_2 / 2;
	i32 index = -1;
	u16* ptr;
	for (u16 i = 0; i < seg_count; i++) {
		if (code_point <= format->end_code[i]) { index = i; break; }
	}

	if (index == -1) { return 0; }

	if (format->start_code[index] < code_point) {
		if (format->id_range_offset != 0) {
			ptr = &format->id_range_offset[index] + format->id_range_offset[index] / 2;
			ptr += (code_point - format->start_code[index]);
			if (ptr == NULL) { return 0; }
			return (*ptr + format->id_delta[index]) % 65536;
		}
		else {
			return (code_point + format->id_delta[index]) % 65536;
		}
	}

	return 0;
}

void* ttf_to_bmp(char ch, uint32_t width, uint32_t height, ttf_core* ttf)
{
	pixel* bmp = (pixel*) calloc(width * height, sizeof(pixel));

	pixel* bmp_ptr = bmp;
	for (uint32_t i = 0; i < height; i++) {
		for (uint32_t j = 0; j < width; j++) {
			bmp_ptr->a = 0xff;
			bmp_ptr++;
		}
	}

	i32 glyph_id = ttf_glyph_index_get(ttf, ch);
	ttf_glyf* glyf0 = ttf->glyf + glyph_id;

	int16_t num_contours = glyf0->header.num_of_contours;
	uint16_t* contours = glyf0->data.end_contours;
	// uint16_t num_points = glyf0->data.num_points;
	uint8_t* flags = glyf0->data.flags;
	int16_t* px = glyf0->data.px;
	int16_t* py = glyf0->data.py;

	// int16_t x_dimensions = abs(glyf0->header.x_min) + abs(glyf0->header.x_max);
	// int16_t y_dimensions = abs(glyf0->header.y_min) + abs(glyf0->header.y_max);

	// float x_scale = (float) width / (float) x_dimensions;
	// float y_scale = (float) height / (float) y_dimensions;
	// float scale;

	int16_t x_min = glyf0->header.x_min;
	int16_t y_min = glyf0->header.y_min;

	// if (x_scale <= y_scale) { scale = x_scale; }
	// else 					{ scale = y_scale; }

	float scale = (float) height / (float) ttf->head->units_per_em;

	// ttf_logd("[ttf_core] x_min %i, y_min %i, scale %f\n", x_min, y_min, scale);

	Array contours_arr;
	arr_init(&contours_arr, sizeof(Array));
	int16_t firsti, lasti;
	for (uint16_t ci = 0; ci < num_contours; ci++) {
		// Array contour2;
		// arr_init(&contour2, sizeof(vectorf));
		Array contour;
		arr_init(&contour, sizeof(ttf_vector2f));

		// TODO: fix how contours are setup
		if (ci == 0) {
			firsti = 0;
			lasti = contours[ci]; 
		}
		else {
			firsti = contours[ci - 1] + 1;
			lasti = contours[ci];
			// lasti = contours[ci] + 1; // previous version
		}
		
		// for (uint16_t i = firsti; i <= lasti; i++) {
		for (uint16_t i = firsti; i < lasti; i++) {
			// for (uint16_t i = 0; i <= contours[1]; i++) {
			ttf_vector2f vec1 = (ttf_vector2f) {
				width - ((scale * px[i]) + abs(x_min * scale)),
				height - ((scale * py[i]) + abs(y_min * scale)),
				flags[i]
			};
			// arr_add(&contour2, &vec1);
			arr_add(&contour, &vec1);
			if (!(flags[i] & 0x01) && !(flags[i + 1] & 0x01)) {
				float mid_x = (px[i] + px[i + 1]) / 2;
				float mid_y = (py[i] + py[i + 1]) / 2;
				ttf_vector2f vec2 = (ttf_vector2f) {
					width - ((scale * mid_x) + abs(x_min * scale)),
					height - ((scale * mid_y) + abs(y_min * scale)),
					0x01
				};
				// arr_add(&contour2, &vec2);
				/*
				if (vec2.y <= 0) {
					ttf_loge("[ttf_contour] ci %i, i %i\n", ci, i);
					ttf_loge("[ttf_contour] mid_x %f, mid_y %f\n", mid_x, mid_y);
					ttf_loge("[ttf_contour] x %i, y %i, flag %c\n",
							 vec2.x, vec2.y, vec2.flag);
					flushl();
					assert(false);
				}
				*/
				arr_add(&contour, &vec2);
			}
		}
		arr_add(&contours_arr, &contour);
	}

	bmp_ptr = bmp;
	float x00, x01, x1, x2, x3, y0, y1, y2, y3, t0, t1;
	float a, b, c, d;
	int b0, b1;
	int16_t tmp0, tmp1, i0, i1;
	uint32_t size, mod;
	Array intersections;
	arr_init(&intersections, sizeof(int16_t));
	
	for (uint32_t i = 0; i < height; i++) {
	// for (uint32_t i = 76; i < 79; i++) {
   	// for (uint32_t i = 60; i < 63; i++) {
		for (uint32_t ci = 0; ci < contours_arr.size; ci++) {
			Array* contour = (Array*) arr_get(&contours_arr, ci);
			for (uint32_t j = 0; j < contour->size; j++) {
				ttf_vector2f* vec1 = (ttf_vector2f*) arr_get(contour, j);
				if (vec1->flag & 0x01) {
					ttf_vector2f* vec2 = (ttf_vector2f*) arr_get(contour, j + 1);
					if (vec2->flag & 0x01) {
						y0 = (float) i;
						// y0 += 0.5f;
						y1 = (float) vec1->y;
						y2 = (float) vec2->y;

						a = y0 - y1;
						b = y2 - y1;

						t0 = a / b;

						/*
						// what if (y0 - y1) and/or (y2 - y1) is zero
						// so lines are horizontal together
						if (a==0 && b==0) {
							// continue;
						}
						*/

						// t must be : 0 <= t <= 1
						if (!(0.0f <= t0 && t0 <= 1.0f)) { continue; }
						if (isinf(t0) || isnan(t0)) { continue; }

						x1 = (float) vec1->x;
						x2 = (float) vec2->x;
						x00 = (1.0f - t0) * x1 + t0 * x2;

						tmp0 = (int16_t) x00;
						arr_add(&intersections, &tmp0);
						/*
						ttf_logi("[ttf_core0] j = %i, intersections.size %i\n",
								 j, intersections.size);
						*/
					}
					else {
						ttf_vector2f* vec3 = (ttf_vector2f*) arr_get(contour, j + 2);
						y0 = (float) i;
						// y0 += 0.5f;
						y1 = (float) vec1->y;
						y2 = (float) vec2->y;
						y3 = (float) vec3->y;

						a = y1 - 2 * y2 + y3;
						b = (2 * y2) - (2 * y1);
						c = y1 - y0;

						d = b * b - 4 * a * c;

						// ttf_logd("[ttf_core1] a %f, b %f, c %f, d %f\n",a, b, c, d);
					
						if (d < 0) { continue; }

						if (a == 0.0f) {
							t0 = - (c / b);
							t1 = 0.0f / 0.0f;
						}
						else {
							t0 = (-b + sqrt(d)) / (2 * a);
							t1 = (-b - sqrt(d)) / (2 * a);
						}
					
						if (!(0.0f <= t0 && t0 <= 1.0f) && !(0.0f <= t1 && t1 <= 1.0f)) {
							continue;
						}

						x1 = (float) vec1->x;
						x2 = (float) vec2->x;
						x3 = (float) vec3->x;

						a = x1 - 2 * x2 + x3;
						b = (2 * x2) - (2 * x1);
						c = x1;

						if (0.0f <= t0 && t0 <= 1.0f) {
							x00 = a * t0 * t0 + b * t0 + c;
							b0 = 1;
						}
						else { b0 = 0; }
						if (0.0f <= t1 && t1 <= 1.0f) {
							x01 = a * t1 * t1 + b * t1 + c;
							b1 = 1;
						}
						else { b1 = 0; }
						if (t0 == t1 && t0 != 1.0f) { b1 = 0; }

						tmp0 = (int16_t) x00;
						tmp1 = (int16_t) x01;
					
						if (b0 && b1) {
							if (tmp0 < tmp1) {
								arr_add(&intersections, &tmp0);
								arr_add(&intersections, &tmp1);
							}
							else {
								arr_add(&intersections, &tmp1);
								arr_add(&intersections, &tmp0);
							}
							j++;
							j++;
						}
						else if (b0) {
							arr_add(&intersections, &tmp0);

							if (t0 == 1.0f) { j++, j++; }
						}
						else if (b1) {
							arr_add(&intersections, &tmp1);

							if (t1 == 1.0f) { j++, j++; }
						}

						/*
						ttf_logi("[ttf_core1] j = %i, intersections.size %i\n",
								  j, intersections.size);
						ttf_logd("[ttf_core1] a %f, b %f, c %f, d %f, t0 %f, t1 %f, b0 %i, b1 %i\n",
								 a, b, c, d, t0, t1, b0, b1);
						ttf_logd("[ttf_core1] y0 %f, y1 %f, y2 %f, y3 %f\n",
								 y0, y1, y2, y3);
						ttf_logd("[ttf_core1] y0 %i, y1 %i, y2 %i, y3 %i\n",
								 i, vec1->y, vec2->y, vec3->y);
						*/
					}
				}
			}
		}

		/*
		ttf_logi("[ttf_core - %i] intersections.size %i\n", i, intersections.size);
		flushl();
		*/

		size = intersections.size;
		mod = size % 2;
		size -= mod;
		bubble_sorts(intersections.data, intersections.size);

		for (uint32_t k = 0; k < size; k += 2) {
			i0 = *((int16_t*) arr_get(&intersections, k));
			i1 = *((int16_t*) arr_get(&intersections, k + 1));

			assert(i0 <= i1);
			assert(0 <= i0);
			assert(0 <= i1);
			
			for (uint32_t m = 0; m < i1 - i0; m++) {
				bmp[(m + i0) + (i * width)] = (pixel) { 0xff, 0xff, 0xff, 0xff };
			}
		}
		arr_clean(&intersections);
	}
	arr_free(&intersections);
	for (uint32_t ci = 0; ci < contours_arr.size; ci++) {
		Array* arr = arr_get(&contours_arr, ci);
		arr_free(arr);
	}
	arr_free(&contours_arr);

	ttf_logi("[ttf_core] ttf glyph 0 converted to bitmap\n");
	return (void*) bmp;
}
