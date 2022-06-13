#include "grafics2.h"

#define ttf_logi(...) logi(__VA_ARGS__)
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

void ttf_load()
{
	ttf_core ttf;
	
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

	if (headers[0].tag) { ttf_head_load(&ttf, file + headers[0].offset); }
	else { ttf_loge("[ttf_core] 'head' table not found\n"); exit(0); }	
	if (headers[1].tag) { ttf_hhea_load(&ttf, file + headers[1].offset); }
	else { ttf_loge("[ttf_core] 'hhea' table not found\n"); exit(0); }
	if (headers[2].tag) { ttf_maxp_load(&ttf, file + headers[2].offset); }
	else { ttf_loge("[ttf_core] 'maxp' table not found\n"); exit(0); }
	if (headers[3].tag) { ttf_hmtx_load(&ttf, file + headers[3].offset); }
	else { ttf_loge("[ttf_core] 'hmtx' table not found\n"); exit(0); }
	if (headers[4].tag) { ttf_cmap_load(&ttf, file + headers[4].offset); }
	else { ttf_loge("[ttf_core] 'cmap' table not found\n"); exit(0); }
	if (headers[5].tag) { ttf_loca_load(&ttf, file + headers[5].offset); }
	else { ttf_loge("[ttf_core] 'loca' table not found\n"); exit(0); }
	if (headers[6].tag) { ttf_glyf_load(&ttf, file + headers[6].offset); }
	else { ttf_loge("[ttf_core] 'glyf' table not found\n"); exit(0); }

	ttf_glyf_free(&ttf);
	ttf_loca_free(&ttf);
	ttf_cmap_free(&ttf);
	ttf_hmtx_free(&ttf);
	free(ttf.maxp);
	free(ttf.hhea);
	free(ttf.head);
	
	free(headers);
	free(td);
	file_free(file);
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
		memcpy(simple->flags, file + offset, simple->num_points);

		offset += simple->num_points;
 
		simple->px = (int16_t*) malloc(sizeof(int16_t) * simple->num_points);
		simple->py = (int16_t*) malloc(sizeof(int16_t) * simple->num_points);

		// TODO: check if coordinates that are 2 bytes long, are converted
		// 		 correctly into a bigger form?

		void* x = file + offset;
		int16_t pos = 0;		// coordinates are relative
		
		for (uint16_t i = 0; i < simple->num_points; i++) {
			if (simple->flags[i] & 0x10) {
				if (simple->flags[i] & 0x02) {
					pos +=  *((int16_t*) x);
					x++;
					simple->px[i] = pos;
				}
				else {
					simple->px[i] = pos;
				}
			}
			else {
				if (simple->flags[i] & 0x02) {
					pos -=  *((int16_t*) x);
					x++;
					simple->px[i] = pos;
				}
				else {
					int16_t temp = *((int16_t*) x);
					temp = ENDIAN_WORD(temp);
					pos += temp;
					x += 2;
					simple->px[i] = pos;
				}
			}
		}

		pos = 0;		// coordinates are relative
		for (uint16_t i = 0; i < simple->num_points; i++) {
			if (simple->flags[i] & 0x20) {
				if (simple->flags[i] & 0x04) {
					pos +=  *((int16_t*) x);
					x++;
					simple->py[i] = pos;
				}
				else {
					simple->py[i] = pos;
				}
			}
			else {
				if (simple->flags[i] & 0x04) {
					pos -= *((int16_t*) x);
					x++;
					simple->py[i] = pos;
				}
				else {
					int16_t temp = *((int16_t*) x);
					temp = ENDIAN_WORD(temp);
					pos += temp;
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
		array components;
		
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
				float transform_11 = f2fot14_to_float(f2dot14);

				f2dot14 = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				float transform_12 = f2fot14_to_float(f2dot14);

				f2dot14 = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				float transform_21 = f2fot14_to_float(f2dot14);

				f2dot14 = ENDIAN_WORD(*((uint16_t*) (file + offset)));
				offset += sizeof(uint16_t);
				float transform_22 = f2fot14_to_float(f2dot14);

				for (uint16_t i = 0; i < component.data.num_points; i++) {
					component.data.px[i] = (int16_t) component.data.px[i] * transform_11 +
						component.data.py[i] * transform_21;
					component.data.px[i] = (int16_t) component.data.py[i] * transform_12 +
						component.data.px[i] * transform_22;
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
		
		for (uint32_t i = 0; i < components.size; i++) {
			memcpy(contours_ptr, tmp->data.end_contours,
				   sizeof(uint16_t) * tmp->header.num_of_contours);
			contours_ptr += tmp->header.num_of_contours;

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

	format->id_range_offset = (uint16_t*) malloc(sizeof(uint16_t) * seg_count);	
	memcpy(format->id_range_offset, file + offset, sizeof(uint16_t) * seg_count);
	offset += sizeof(uint16_t) * seg_count;

	for (uint16_t i = 0; i < seg_count; i++) {
		format->end_code[i] = ENDIAN_WORD(format->end_code[i]);
		format->start_code[i] = ENDIAN_WORD(format->start_code[i]);
		format->id_delta[i] = ENDIAN_WORD(format->id_delta[i]);
		format->id_range_offset[i] = ENDIAN_WORD(format->id_range_offset[i]);
	}

	format->glyph_index_array = (uint16_t*) malloc(sizeof(uint16_t) * TTF_GLYPH_TABLE_SIZE);
	memcpy(format->glyph_index_array, file + offset, sizeof(uint16_t) * TTF_GLYPH_TABLE_SIZE);
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
	free(ttf->cmap->format.glyph_index_array);
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
