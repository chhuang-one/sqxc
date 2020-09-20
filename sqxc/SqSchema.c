/*
 *   Copyright (C) 2020 by C.H. Huang
 *   plushuang.tw@gmail.com
 *
 * sqxc is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#define snprintf	_snprintf
#endif

#include <stdio.h>      // snprintf

#include <SqError.h>
#include <SqBuffer.h>
#include <SqUtil.h>
#include <SqSchema.h>

#define SCHEMA_INITIAL_VERSION     1
#define DEFAULT_STRING_LENGTH    191

SqSchema*  sq_schema_new(const char* name)
{
	static int cur_version = SCHEMA_INITIAL_VERSION;
	SqSchema* schema;
	SqType* typeinfo;

	schema = malloc(sizeof(SqSchema));
	typeinfo = sq_type_new(8, (SqDestroyFunc)sq_table_free);

	sq_entry_init((SqEntry*)schema, typeinfo);
	schema->name = name ? strdup(name) : NULL;
	// table_types
	sq_ptr_array_init(&schema->table_types, 8, NULL);
	schema->table_types_sorted = false;
	//
	schema->version = cur_version++;
	return schema;
}

void  sq_schema_free(SqSchema* schema)
{
	sq_entry_final((SqEntry*)schema);
	sq_ptr_array_final(&schema->table_types);
	free(schema);
}

SqTable* sq_schema_create_full(SqSchema* schema,
                               const char* table_name,
                               const SqType* type_info,
                               const char* type_name,
                               size_t instance_size)
{
	SqTable*  table;

	table = sq_table_new(table_name, type_info);
	// if type_info == NULL,
	// table->type is dynamic type and table->bit_field has SQB_TYPE_DYNAMIC
	if (type_info == NULL) {
		table->type->size = instance_size;
		if (type_name)
			table->type->name = strdup(type_name);
	}

#ifdef SQ_HAVE_NAMING_CONVENTION
	if (table->name == NULL)
		table->name = sq_name2table(table->type->name);
	if (table->type->name == NULL)
		table->type->name = sq_name2type(table->name);
#endif

	// add table in table_types
	sq_ptr_array_append(&schema->table_types, table);
	schema->table_types_sorted = false;
	// add table in schema->type
	sq_type_insert_entry(schema->type, (SqEntry*)table);
	schema->bit_field |= SQB_CHANGE;
	return table;
}

SqTable* sq_schema_alter(SqSchema* schema, const char* name, const SqType* type_info)
{
	SqTable*  table;

	table = sq_table_new(name, type_info);
	table->bit_field |= SQB_CHANGE;

	sq_type_insert_entry(schema->type, (SqEntry*)table);
	schema->bit_field |= SQB_CHANGE;
	return table;
}

void sq_schema_drop(SqSchema* schema, const char* name)
{
	SqTable* table;

	table = calloc(1, sizeof(SqTable));
	table->old_name = strdup(name);
	table->name = NULL;
	table->bit_field = SQB_DYNAMIC;

	sq_type_insert_entry(schema->type, (SqEntry*)table);
	schema->bit_field |= SQB_CHANGE;

#if 0
	// remove table in table_types
	void** addr = sq_ptr_array_find(&schema->table_types, name, (SqCompareFunc)sq_entry_cmp_str__type_name);
	if (addr)
		sq_ptr_array_erase_addr(&schema->table_types, addr, 1);
	// remove table in schema->type
	addr = sq_type_find_entry(schema->type, name, NULL);
	if (addr)
		sq_type_erase_entry_addr(schema->type, addr, 1);
#endif
}

void sq_schema_rename(SqSchema* schema, const char* from, const char* to)
{
	SqTable* table;

	table = calloc(1, sizeof(SqTable));
	table->old_name = strdup(from);
	table->name = strdup(to);
	table->bit_field = SQB_DYNAMIC;

	sq_type_insert_entry(schema->type, (SqEntry*)table);
	schema->bit_field |= SQB_CHANGE;

#if 0
//	table = (SqTable*)sq_type_find_entry(schema->type, from,
//	                                     (SqCompareFunc)sq_entry_cmp_str__name);
	table = (SqTable*)sq_type_find_entry(schema->type, from, NULL);
	if (table) {
		table = *(SqTable**)table;
		free(table->name);
		table->name = strdup(to);
	}
#endif
}

SqTable* sq_schema_find(SqSchema* schema, const char* name)
{
	SqCompareFunc cmp_func;
	void** addr;

	// if cmp_func == NULL, sq_type_find_entry() will sort entry before finding.
	if (schema->bit_field & SQB_CHANGE)
		cmp_func = (SqCompareFunc)sq_reentry_cmp_str__name;
	else
		cmp_func = NULL;

	addr = sq_type_find_entry(schema->type, name, cmp_func);
	if (addr)
		return *addr;
	else
		return NULL;
}

SqTable* sq_schema_find_type(SqSchema* schema, const char* name)
{
	SqPtrArray* array = &schema->table_types;
	SqTable**   table;

	if (schema->table_types.data == NULL)
		return NULL;
	// sort
	if (schema->table_types_sorted == false) {
		schema->table_types_sorted = true;
		sq_ptr_array_sort(array, sq_entry_cmp_type_name);
	}
	// search
	table = sq_ptr_array_search(array, name,
			(SqCompareFunc)sq_entry_cmp_str__type_name);
	if (table)
		return *table;
	return NULL;
}

int   sq_schema_accumulate(SqSchema* schema, SqSchema* schema_src)
{	//         *schema,    *schema_src
	SqTable    *table,     *table_src;
	SqPtrArray *reentries, *reentries_src;
	// other variable
	int      index;
	void   **addr;

	reentries = sq_type_get_ptr_array(schema->type);
	reentries_src = sq_type_get_ptr_array(schema_src->type);

	// run sq_schema_accumulate() first time.
	if ((schema->bit_field & SQB_SCHEMA_ACCUMULATED) == 0) {
		// get foreign keys before calling sq_schema_trace_foreign()
		for (index = 0;  index < reentries->length;  index++) {
			table = (SqTable*)reentries->data[index];
			sq_table_init_extra(table);
			sq_table_get_foreigns(table, &table->extra->foreigns);
		}
	}

	for (index = 0;  index < reentries_src->length;  index++) {
		table_src = (SqTable*)reentries_src->data[index];
		if (table_src->bit_field & SQB_CHANGE) {
			// === ALTER TABLE ===
			// find table if table->name == table_src->name
			addr = sq_reentries_find_name(reentries, table_src->name);
			if (addr) {
				table = *(SqTable**)addr;
				sq_table_accumulate(table, table_src);
				// append changed table to tail
				*addr = NULL;
				sq_reentries_add(reentries, table);
				// It doesn't need to steal 'table_src' from 'schema_src'
				continue;
			}
			else {
				sq_table_init_extra(table_src);
				sq_table_get_foreigns(table_src, &table_src->extra->foreigns);
				// table_types
				sq_ptr_array_append(&schema->table_types, table_src);
				schema->table_types_sorted = false;
			}
		}
		else if (table_src->name == NULL) {
			// === DROP TABLE ===
			// erase original table if table->name == table_src->old_name
			sq_reentries_erase_name(reentries, table_src->old_name);
		}
		else if (table_src->old_name) {
			// === RENAME TABLE ===
			// find existing if table->name == table_src->old_name
			addr = sq_reentries_find_name(reentries, table_src->old_name);
			if (addr) {
				// rename existing table->name to table_src->name
				table = *(SqTable**)addr;
				free(table->name);
				table->name = strdup(table_src->name);
			}
		}
		else {
			// === ADD TABLE ===
			sq_table_init_extra(table_src);
			sq_table_get_foreigns(table_src, &table_src->extra->foreigns);
			// table_types
			sq_ptr_array_append(&schema->table_types, table_src);
			schema->table_types_sorted = false;
		}

		// steal 'table_src' from 'schema_src->type->entry'.
		reentries_src->data[index] = NULL;
		// add 'table_src' to schema->type->entry.
		sq_reentries_add(reentries, table_src);
	}

	// update other data in SqSchema
	schema->version = schema_src->version;
	schema->bit_field |= SQB_SCHEMA_ACCUMULATED;
	schema->type->bit_field &= ~SQB_TYPE_SORTED;

	return SQCODE_OK;
}

int     sq_schema_trace_foreign(SqSchema* schema)
{
	SqPtrArray *reentries = sq_type_get_ptr_array(schema->type);
	SqTable    *table, *table_fore;
	SqColumn   *column;
	const char *name;
	int         result = SQCODE_OK;

	for (int index = 0;  index < reentries->length;  index++) {
		table = (SqTable*)reentries->data[index];
		if (table == NULL || table->extra == NULL)
			continue;

		for (int i = 0;  i < table->extra->foreigns.length;  i++) {
			column = (SqColumn*)table->extra->foreigns.data[i];
			// trace renamed table
			name = sq_reentries_trace_renamed(reentries,
					column->foreign->table, schema->offset);
			if (name == NULL) {
				// table dropped.
				result = SQCODE_REENTRY_DROPPED;
				continue;   // error...
			}
			else if (name != column->foreign->table) {
				// table renamed. name = the newest table name
				free(column->foreign->table);
				column->foreign->table = strdup(name);
			}
			// find referenced table
			table_fore = (SqTable*)sq_reentries_find_name(reentries,
					column->foreign->table);
			if (table_fore)
				table_fore = *(SqTable**)table_fore;
			else {
				// table not found. dropped?
				result = SQCODE_ENTRY_NOT_FOUND;
				continue;  // error...
			}
			// trace renamed column
			name = sq_reentries_trace_renamed(&table_fore->type->entry,
					column->foreign->column, table_fore->offset);
			if (name == NULL) {
				// column dropped.
				result = SQCODE_REENTRY_DROPPED;
				continue;   // error...
			}
			else if (name != column->foreign->column) {
				// column renamed. name = newest column name
				free(column->foreign->column);
				column->foreign->column = strdup(name);
			}
		}
	}
	return result;
}

void  sq_schema_clear_changes(SqSchema* schema,
                              unsigned int clear_table_bit_field,
                              unsigned int set_table_bit_field)
{
	SqPtrArray* reentries;
	SqTable *table;
	int      index;

	// erase changed records and remove NULL records in schema
	reentries = sq_type_get_ptr_array(schema->type);    // schema->type->entry
	sq_reentries_erase_changes(reentries);
	sq_reentries_remove_null(reentries);
	// update 'offset' for sq_schema_trace_foreign() and migration
	schema->offset = reentries->length;
	// all changed records have removed
	schema->bit_field &= ~SQB_CHANGE;

	// erase changed records and remove NULL records in tables
	for (index = 0;  index < reentries->length;  index++) {
		table = (SqTable*)reentries->data[index];
		reentries = sq_type_get_ptr_array(table->type);    // table->type->entry
		//  table->type maybe static
		if (table->type->bit_field & SQB_TYPE_DYNAMIC) {
			sq_reentries_erase_changes(reentries);
			sq_reentries_remove_null(reentries);
		}
		// update 'offset' for sq_schema_trace_foreign() and migration
		table->offset = reentries->length;
		// all changed records have removed
		table->bit_field &= ~SQB_CHANGE;
		// set/clear table->bit_field
		table->bit_field &= ~clear_table_bit_field;
		table->bit_field |= set_table_bit_field;
	}
}

// trace column reference and count order
// before calling this function:
// 1. sort schema->type->entry by table->name
// 2. sort table->extra->foreigns by column->foreign->table
// 3. reset all table->offset = 0
static int  count_table_order(SqSchema* schema, SqTable* table, int* is_reo)
{
	SqPtrArray* foreigns;
	SqColumn*  column;
	SqTable*   fore_table;
	SqTable*   prev_table = NULL;

	if (table->offset > 0)
		return table->offset;
	table->offset = 1;
	table->bit_field |= SQB_TABLE_CHECKING;
	foreigns = &table->extra->foreigns;

	for (int index = 0;  index < foreigns->length;  index++) {
 		column = (SqColumn*)foreigns->data[index];
		if (column == NULL)
			continue;

		fore_table = sq_ptr_array_search(&schema->type->entry, column->foreign->table,
		                                 (SqCompareFunc)sq_entry_cmp_str__name);
		if (fore_table)
			fore_table = *(SqTable**)fore_table;
		else {
			// error...
			continue;
		}

		if (fore_table->bit_field & SQB_TABLE_CHECKING) {
			// if tables reference each other, retain current column for future use.
			if (is_reo)
				*is_reo = 1;
			// for SQLite (constraint reference each other)
			if (column->constraint)
				table->bit_field |= SQB_TABLE_CONSTRAINT_REO;
			continue;
		}
		else {
			if (prev_table != fore_table) {
				int  ref_each_other = 0;
				table->offset += count_table_order(schema, fore_table, &ref_each_other);
				// if tables reference each other, retain current column for future use.
				if (ref_each_other == 1) {
					if (is_reo)
						*is_reo = 1;
					// for SQLite (constraint reference each other)
					if (column->constraint)
						table->bit_field |= SQB_TABLE_CONSTRAINT_REO;
					continue;
				}
			}
			// not reference each other
			// steal column. this NULL will be removed by sq_reentries_remove_null()
			foreigns->data[index] = NULL;
		}
		prev_table = fore_table;
	}

	sq_reentries_remove_null(foreigns);
	table->bit_field &= ~SQB_TABLE_CHECKING;
	return table->offset;
}

static int  sq_entry_cmp_offset(SqEntry** entry1, SqEntry** entry2)
{
	return (*entry1)->offset - (*entry2)->offset;
}

static int  sq_column_cmp_foreign_table(SqColumn** column1, SqColumn** column2)
{
	return strcasecmp((*column1)->foreign->table, (*column2)->foreign->table);
}

void    sq_schema_arrange(SqSchema* schema, SqPtrArray* entries)
{
	SqPtrArray* tables = sq_type_get_ptr_array(schema->type);
	SqTable*    table1;

	// copy table pointers from schema->type->entry
	if (entries->length < tables->length)
		sq_ptr_array_alloc(entries, tables->length - entries->length);
	memcpy(entries->data, tables->data, sizeof(void*) * tables->length);
	entries->length = tables->length;

	// sort 'tables' (schema->type->entry) by table->name before calling count_table_order()
	sq_ptr_array_sort(tables, (SqCompareFunc)sq_entry_cmp_name);
	schema->type->bit_field |= SQB_TYPE_SORTED;
	// setup all tables before calling count_table_order()
	for (int index = 0;  index < tables->length;  index++) {
		table1 = (SqTable*)tables->data[index];
		// reset table->offset before calling count_table_order()
		table1->offset = 0;
		// sort column in tables before calling count_table_order()
		sq_ptr_array_sort(&table1->extra->foreigns, (SqCompareFunc)sq_column_cmp_foreign_table);
	}
	// count order number and set it in table->offset
	for (int index = 0;  index < tables->length;  index++)
		count_table_order(schema, (SqTable*)tables->data[index], NULL);

	// sort entries by table->offset
	sq_ptr_array_sort(entries, (SqCompareFunc)sq_entry_cmp_offset);
}
