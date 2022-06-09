#include "grafics2.h"

#define ttf_logi(...) logi(__VA_ARGS__)
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

// #define TTF_MAGIC_NUMBER 0x5f0f3cf5
#define TTF_MAGIC_NUMBER 0xf53c0f5f

void ttf_load()
{
	ttf_core ttf;
	
	uint32_t file_size;
	char* file = file_read("resources/calibri.ttf", &file_size);
	
	ttf_table_directory* td = (ttf_table_directory*) malloc(sizeof(ttf_table_directory));
	td = (ttf_table_directory*) file;
	
	td->numTables = ENDIAN_WORD(td->numTables);
	td->searchRange = ENDIAN_WORD(td->searchRange);
	td->entrySelector = ENDIAN_WORD(td->entrySelector);
	td->rangeShift = ENDIAN_WORD(td->rangeShift);
	
	ttf_header* ptr = (ttf_header*) (file + sizeof(ttf_table_directory));

	for (uint16_t i = 0; i < td->numTables; i++) {
		char tag[5] = { 0, 0, 0, 0, 0 };
		memcpy(tag, &ptr->tag, 4);
		
		ttf_header header;
		memcpy(&header, ptr, sizeof(ttf_header));

		header.tag = ENDIAN_DWORD(header.tag);
		header.checkSum = ENDIAN_DWORD(header.checkSum);
		header.offset = ENDIAN_DWORD(header.offset);
		header.length = ENDIAN_DWORD(header.length);
		
		switch (header.tag) {
			case TTF_HEAD_TAG:
				memcpy(&ttf.head, file + header.offset, header.length);
				ttf_logi("[ttf_core] foung 'head' table\n");
				break;
			case TTF_HHEA_TAG:
				memcpy(&ttf.hhea, file + header.offset, header.length);
				ttf_logi("[ttf_core] foung 'hhea' table\n");
				break;
			case TTF_MAXP_TAG:
				memcpy(&ttf.maxp, file + header.offset, header.length);
				ttf_logi("[ttf_core] foung 'maxp' table\n");
				break;
			case TTF_HMTX_TAG:
				ttf.hmtx.h_metrics = (ttf_long_hor_metric*) malloc(
						  sizeof(ttf_long_hor_metric) * ttf.hhea.num_of_long_hor_metrics + 
						   sizeof(int16_t) * (ttf.maxp.num_glyphs -
											  ttf.hhea.num_of_long_hor_metrics));
				ttf.hmtx.left_side_bearing = (int16_t*) ttf.hmtx.h_metrics;
				ttf.hmtx.left_side_bearing += sizeof(int16_t) *
								(ttf.maxp.num_glyphs - ttf.hhea.num_of_long_hor_metrics);
				memcpy(ttf.hmtx.h_metrics, file + header.offset, header.length);
				ttf_logi("[ttf_core] foung 'hmtx' table\n");
				break;
			case TTF_CMAP_TAG:
				ttf.cmap.index = (ttf_cmap_index*) malloc(header.length);
				memcpy(ttf.cmap.index, file + header.offset, header.length);
				ttf_logi("[ttf_core] foung 'cmap' table\n");
				break;
			default:
				ttf_logi("[ttf_core] found '%s' table, but not added to ttf_core\n",
						 tag);
		}

		ptr++;
	}

	flushl();

	ttf_head_load(&ttf);
	ttf_hhea_load(&ttf);
	ttf_maxp_load(&ttf);
	ttf_hmtx_load(&ttf);
	ttf_cmap_load(&ttf);

	// free(td);
	file_free(file);
}

void ttf_head_load(ttf_core* ttf)
{
	// TODO: deal with errors
	// NOTE: commented varibles are not changed to correct endian form

	ttf_head* head = &ttf->head;

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
}

void ttf_hhea_load(ttf_core* ttf)
{
	// TODO: deal with errors

	ttf_hhea* hhea = &ttf->hhea;

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
}

void ttf_maxp_load(ttf_core* ttf)
{
	// TODO: deal with errors

	ttf_maxp* maxp = &ttf->maxp;
	
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
}

void ttf_hmtx_load(ttf_core* ttf)
{
	// TODO: deal with errors
	// NOTE: commented varibles are not changed to correct endian form

	// h_metrics
	// left_side_bearing
}

void ttf_cmap_load(ttf_core* ttf)
{
	// TODO: deal with errors

	ttf_cmap* cmap = &ttf->cmap;

	cmap->index->version = ENDIAN_WORD(cmap->index->version);
	cmap->index->number_subtables = ENDIAN_WORD(cmap->index->number_subtables);
	
	cmap->tables = (ttf_cmap_estable*) (cmap->index + 1);

	ttf_cmap_estable* table = cmap->tables;
	for (uint16_t i = 0; i < cmap->index->number_subtables; i++) {
		table[i].platform_id = ENDIAN_WORD(table[i].platform_id);
		table[i].platform_specific_id = ENDIAN_WORD(table[i].platform_specific_id);
		table[i].offset = ENDIAN_DWORD(table[i].offset);
	}

	// TODO: ttf_cmap_format?
	
	void* cmap_ptr = (void*) cmap->index;
	for (uint16_t i = 0; i < cmap->index->number_subtables; i++) {
		if (table[i].platform_id == 3) {
			uint16_t format = *((uint16_t*) (cmap_ptr + table[i].offset));
			format = ENDIAN_WORD(format);

			switch (format) {
				case 4:
					ttf_logi("[ttf_cmap] format4 subtable chosen\n");
					ttf_cmap_format4_table_load(cmap, table[i].offset);
					break;
				default:
					ttf_logi("[ttf_cmap] format of type %hu not supported\n", format);
			}
		}
	}
}

void ttf_cmap_format4_table_load(ttf_cmap* cmap, uint32_t offset)
{
	void* cmap_ptr = (void*) cmap->index;
	ttf_cmap_format4* format = (ttf_cmap_format4*) (cmap_ptr + offset);

	format->format = ENDIAN_WORD(format->format);
	format->length = ENDIAN_WORD(format->length);
	format->language = ENDIAN_WORD(format->language);
	format->seg_count_2 = ENDIAN_WORD(format->seg_count_2);
	format->search_range = ENDIAN_WORD(format->search_range);
	format->entry_selector = ENDIAN_WORD(format->entry_selector);
	format->range_shift = ENDIAN_WORD(format->range_shift);

	
}
