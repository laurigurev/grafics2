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

	free(ttf.head);
	free(ttf.hhea);
	free(ttf.maxp);
	ttf_hmtx_free(&ttf);
	ttf_cmap_free(&ttf);
	ttf_loca_free(&ttf);

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
	
	ttf->glyf = (ttf_glyf*) malloc(sizeof(ttf_glyf));
	ttf_glyf* glyf = ttf->glyf;
	memcpy(glyf, file, sizeof(ttf_glyf));
	glyf->num_of_contours = ENDIAN_WORD(glyf->num_of_contours);
	glyf->x_min = ENDIAN_WORD(glyf->x_min);
	glyf->y_min = ENDIAN_WORD(glyf->y_min);
	glyf->x_max = ENDIAN_WORD(glyf->x_max);
	glyf->y_max = ENDIAN_WORD(glyf->y_max);

	// ttf_logi("[ttf_core] 'loca' table loaded\n");
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
