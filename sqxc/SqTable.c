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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <SqError.h>
#include <SqTable.h>

#define N_CONSTRAINT_LEN    4

SqTable* sq_table_new(const char* name, const SqType* typeinfo)
{
	SqTable* table;

	table = malloc(sizeof(SqTable));
	// create dynamic SqType
	if (typeinfo == NULL)
		typeinfo = sq_type_new(8, (SqDestroyFunc)sq_column_free);
	sq_entry_init((SqEntry*)table, typeinfo);
	table->name = (name) ? strdup(name) : NULL;
	table->bit_field |= SQB_POINTER;
	table->old_name = NULL;
	table->foreigns = (SqPtrArray){NULL, 0};

	return (SqTable*)table;
}

void  sq_table_free(SqTable* table)
{
	if (table->bit_field & SQB_DYNAMIC) {
		// clear SqEntry
		sq_entry_final((SqEntry*)table);
		free(table->old_name);
		// free array
		sq_ptr_array_final(&table->foreigns);
		// free SqTable
		free(table);
	}
}

SqTable*  sq_table_copy_static(const SqTable* table_src)
{
	SqTable* table;

	table = malloc(sizeof(SqTable));
	table->type = table_src->type;
	table->name = table_src->name ? strdup(table_src->name) : NULL;
	table->offset = table_src->offset;
	table->bit_field = table_src->bit_field | SQB_DYNAMIC;
	table->old_name = table_src->old_name ? strdup(table_src->old_name) : NULL;
	return table;
}

bool  sq_table_has_column(SqTable* table, const char* column_name)
{
	if (sq_type_find_entry(table->type, column_name, NULL))
		return true;
	return false;
}

static void sq_table_free_column(SqTable* table, SqColumn* column)
{
	void**    addr;

	// if column has foreign reference, remove it's pointer from table->foreigns.
	if (column->foreign && table->foreigns.data) {
		addr = sq_reentries_replace(&table->foreigns, column, NULL);
		if (addr)
			sq_ptr_array_steal(&table->foreigns, addr - table->foreigns.data, 1);
	}
	// free column
	sq_column_free(column);
}

void  sq_table_drop_column(SqTable* table, const char* column_name)
{
	SqType* table_type = table->type;
	SqColumn*  column;

	if (table->bit_field & SQB_CHANGE) {
		// migration
		column = calloc(1, sizeof(SqColumn));
		column->old_name = strdup(column_name);
		column->bit_field = SQB_DYNAMIC;
		sq_ptr_array_append(&table_type->entry, column);
		return;
	}

	column = (SqColumn*)sq_type_find_entry(table_type, column_name, NULL);
	if (column) {
		sq_table_free_column(table, *(SqColumn**)column);
		sq_type_steal_entry_addr(table_type, (void**)column, 1);
	}
}

void  sq_table_rename_column(SqTable* table, const char* from, const char* to)
{
	SqType* table_type = table->type;
	SqColumn*  column;

	if (table->bit_field & SQB_CHANGE) {
		// migration
		column = calloc(1, sizeof(SqColumn));
		column->old_name = strdup(from);
		column->name = strdup(to);
		column->bit_field = SQB_DYNAMIC;
		sq_ptr_array_append(&table_type->entry, column);
		return;
	}

	column = (SqColumn*)sq_type_find_entry(table_type, from, NULL);
	if (column) {
		column = *(SqColumn**)column;
		free(column->name);
		column->name = strdup(to);
		table_type->bit_field &= ~SQB_TYPE_SORTED;
	}
}

SqColumn* sq_table_get_primary(SqTable* table)
{
	SqPtrArray* array;
	SqColumn*   column;

	array = sq_type_get_array(table->type);
	sq_ptr_array_foreach(array, element) {
		column = element;
		if ( (column->bit_field & SQB_PRIMARY) == 0 ||
		     SQ_TYPE_NOT_INT(column->type))
		{
			continue;
		}
		return column;
	}
	return NULL;
}

SqColumn* sq_table_add_int(SqTable* table, const char* name, size_t offset)
{
	SqColumn* column;

	column = sq_column_new(name, SQ_TYPE_INT);
	column->offset = offset;
	sq_type_insert_entry(table->type, (SqEntry*)column);
	return column;
}

SqColumn* sq_table_add_uint(SqTable* table, const char* name, size_t offset)
{
	SqColumn* column;

	column = sq_column_new(name, SQ_TYPE_UINT);
	column->offset = offset;
	sq_type_insert_entry(table->type, (SqEntry*)column);
	return column;
}

SqColumn* sq_table_add_int64(SqTable* table, const char* name, size_t offset)
{
	SqColumn* column;

	column = sq_column_new(name, SQ_TYPE_INT64);
	column->offset = offset;
	sq_type_insert_entry(table->type, (SqEntry*)column);
	return column;
}

SqColumn* sq_table_add_uint64(SqTable* table, const char* name, size_t offset)
{
	SqColumn* column;

	column = sq_column_new(name, SQ_TYPE_UINT64);
	column->offset = offset;
	sq_type_insert_entry(table->type, (SqEntry*)column);
	return column;
}

SqColumn* sq_table_add_double(SqTable* table, const char* name, size_t offset)
{
	SqColumn* column;

	column = sq_column_new(name, SQ_TYPE_DOUBLE);
	column->offset = offset;
	sq_type_insert_entry(table->type, (SqEntry*)column);
	return column;
}

SqColumn* sq_table_add_timestamp(SqTable* table, const char* name, size_t offset)
{
	SqColumn* column;

	column = sq_column_new(name, SQ_TYPE_TIME);
	column->offset = offset;
	sq_type_insert_entry(table->type, (SqEntry*)column);
	return column;
}

SqColumn* sq_table_add_string(SqTable* table, const char* name, size_t offset, int length)
{
	SqColumn* column;

	column = sq_column_new(name, SQ_TYPE_STRING);
	column->offset = offset;
	column->size = length;
	sq_type_insert_entry(table->type, (SqEntry*)column);
	return column;
}

SqColumn* sq_table_add_custom(SqTable* table, const char* name,
                              size_t offset, const SqType* sqtype)
{
	SqColumn* column;

	column = sq_column_new(name, sqtype);
	column->offset = offset;
	sq_type_insert_entry(table->type, (SqEntry*)column);
	return column;
}

SqColumn* sq_table_add_foreign(SqTable* table, const char* name)
{
	SqColumn* column;

	column = calloc(1, sizeof(SqColumn));
	column->bit_field = SQB_DYNAMIC | SQB_FOREIGN;
	column->name = malloc(strlen(table->name) + strlen(name) + 10);  // + '_' + "_foreign" + '\0'
	column->name[0] = 0;
	strcat(column->name, table->name);
	strcat(column->name, "_");
	strcat(column->name, name);
	strcat(column->name, "_foreign");
	sq_column_set_constraint(column, name, NULL);

	return column;
}

void   sq_table_drop_foreign(SqTable* table, const char* name)
{
	SqColumn* column;
	void**    addr;

	if (table->bit_field & SQB_CHANGE) {
		// migration
		column = calloc(1, sizeof(SqColumn));
		column->old_name = strdup(name);
		column->bit_field = SQB_DYNAMIC | SQB_FOREIGN;
		return;
	}

	addr = sq_type_find_entry(table->type, name, NULL);
	if (addr)
		sq_type_erase_entry_addr(table->type, addr, 1);
}

// This used by migration. It will steal columns from table_src
int   sq_table_accumulate(SqTable* table, SqTable* table_src)
{	//       *table,  *table_src
	SqColumn *column, *column_src;
	SqType   *type,   *type_src;
	int                index_src;
	void    **addr;

	type = table->type;
	type_src = table_src->type;

	if ((table->bit_field & SQB_DYNAMIC) == 0 ||
	    (type->bit_field  & SQB_TYPE_DYNAMIC) == 0)
	{
		return SQCODE_STATIC_DATA;
	}
	// if table is empty table
	if (type->n_entry == 0) {
		// set SQB_CHANGE if it is "ALTER TABLE"
		if (table_src->bit_field &  SQB_CHANGE)
			table->bit_field |= SQB_CHANGE;
	}

	for (index_src = 0;  index_src < type_src->n_entry;  index_src++) {
		column_src = (SqColumn*)type_src->entry[index_src];
		if (column_src->bit_field & SQB_CHANGE) {
			// === ALTER COLUMN ===
			// free column if column->name == column_src->name
			addr = sq_reentries_find_name(&type->entry, column_src->name);
			if (addr) {
				sq_table_free_column(table, *addr);
				*addr = NULL;
			}
			sq_reentries_add(&type->entry, column_src);
			// steal 'column_src' if 'type_src' is not static.
			if (type_src->bit_field & SQB_TYPE_DYNAMIC)
				type_src->entry[index_src] = NULL;
		}
		else if (column_src->name == NULL) {
			// === DROP COLUMN / CONSTRAINT / KEY ===
			// free column if column->name == column_src->old_name
			addr = sq_reentries_find_name(&type->entry, column_src->old_name);
			if (addr) {
				sq_table_free_column(table, *addr);
				*addr = NULL;
			}
			sq_reentries_add(&type->entry, column_src);
			// steal 'column_src' if 'type_src' is not static.
			if (type_src->bit_field & SQB_TYPE_DYNAMIC)
				type_src->entry[index_src] = NULL;
			// "dropped record" doesn't need to check foreign
			continue;
		}
		else if (column_src->old_name) {
			// === RENAME COLUMN ===
			// find column if column->name == column_src->old_name
			addr = sq_reentries_find_name(&type->entry, column_src->old_name);
			// rename existing column->name to column_src->name
			if (addr) {
				column = *(SqColumn**)addr;
				if ((column->bit_field & SQB_DYNAMIC) == 0) {
					// create dynamic column to replace static one
					sq_table_free_column(table, column);
					column = sq_column_copy_static(column);
					*addr  = column;
				}
				free(column->name);
				column->name = strdup(column_src->name);
			}
			// steal 'column_src' if 'type_src' is not static.
			if (type_src->bit_field & SQB_TYPE_DYNAMIC)
				type_src->entry[index_src] = NULL;
			// append column_src to table
			sq_reentries_add(&type->entry, column_src);
			// "renamed record" doesn't need to check foreign
			continue;
		}
		else {
			// === ADD COLUMN / CONSTRAINT / KEY ===
			// steal 'column_src' if 'type_src' is not static.
			if (type_src->bit_field & SQB_TYPE_DYNAMIC)
				type_src->entry[index_src] = NULL;
			// append 'column_src' to table
			sq_reentries_add(&type->entry, column_src);
		}

		// ADD or ALTER COLUMN that having foreign reference
		if (column_src->foreign) {
			if (table->foreigns.data == NULL)
				sq_ptr_array_init(&table->foreigns, 4, NULL);
			sq_ptr_array_append(&table->foreigns, column_src);
		}
	}

	// remove NULL column (it was stolen) if table_src is not static.
//	if (type_src->bit_field & SQB_TYPE_DYNAMIC)
//		sq_reentries_remove_null(&type_src->entry);
	// remove NULL record in table
	sq_reentries_remove_null(&type->entry);
	type->bit_field &= ~SQB_TYPE_SORTED;
	return SQCODE_OK;
}

// ----------------------------------------------------------------------------
//

static void  sq_foreign_free(SqForeign* foreign)
{
	free(foreign->table);
	free(foreign->column);
	free(foreign->on_delete);
	free(foreign->on_update);
	free(foreign);
}

static SqForeign* sq_foreign_copy(SqForeign* src)
{
	SqForeign*  foreign;

	foreign = malloc(sizeof(SqForeign));
	foreign->table  = strdup(src->table);
	foreign->column = strdup(src->column);
	foreign->on_delete = strdup(src->on_delete);
	foreign->on_update = strdup(src->on_update);
	return foreign;
}

#define sq_constraint_allocated(constraint)    *( (intptr_t*) ((constraint)-1) )
#define sq_constraint_alloc(n)                 ( (char**)malloc(sizeof(char*) *(n+1)) +1)
#define sq_constraint_realloc(constraint, n)   ( (char**)realloc((constraint)-1, sizeof(char*) *(n+1)) +1)
#define sq_constraint_free(constraint)         free((constraint)-1)

// ----------------------------------------------------------------------------
// SqColumn C functions

SqColumn*  sq_column_new(const char* name, const SqType* typeinfo)
{
	SqColumn*  column;

	column = calloc(1, sizeof(SqColumn));
	// init SqEntry members
	sq_entry_init((SqEntry*)column, typeinfo);
	column->name = strdup(name);

	// init SqColumn members
	return column;
}

void  sq_column_free(SqColumn* column)
{
	if (column->bit_field & SQB_DYNAMIC) {
		// clear SqEntry
		sq_entry_final((SqEntry*)column);
		// free SqColumn
		free(column->default_value);
		free(column->check);
//		free(column->comment);
		free(column->extra);
		free(column->old_name);
		if (column->foreign)
			sq_foreign_free(column->foreign);
		if (column->constraint)
			sq_constraint_free(column->constraint);
		free(column);
	}
}

SqColumn* sq_column_copy_static(const SqColumn* column_src)
{
	SqColumn* column;
	int       index, length;

	column = malloc(sizeof(SqColumn));
	column->type      = column_src->type;
	column->offset    = column_src->offset;
	column->bit_field = column_src->bit_field | SQB_DYNAMIC;
	column->size      = column_src->size;
	column->digits    = column_src->digits;

	column->name          = column_src->name ? strdup(column_src->name) : NULL;
	column->default_value = column_src->default_value ? strdup(column_src->default_value) : NULL;
	column->check         = column_src->check ? strdup(column_src->check) : NULL;
//	column->comment       = column_src->comment ? strdup(column_src->comment) : NULL;
	column->extra         = column_src->extra ? strdup(column_src->extra) : NULL;
	column->old_name      = column_src->old_name ? strdup(column_src->old_name) : NULL;

	if (column_src->foreign)
		column->foreign = sq_foreign_copy(column_src->foreign);
	else
		column->foreign = NULL;

	column->constraint = NULL;
	if (column_src->constraint) {
		for (index = 0;  column_src->constraint[index];  index++)
			;
		if (index > 0) {
			length = index + 1;
			column->constraint = sq_constraint_alloc(length);
			column->constraint[index] = NULL;
			for (index = 0;  index < length;  index++)
				column->constraint[index] = strdup(column_src->constraint[index]);
		}
	}
	return column;
}

// foreign key references
void  sq_column_reference(SqColumn* column,
                          const char* foreign_table_name,
                          const char* foreign_column_name)
{
	if ((column->bit_field & SQB_DYNAMIC) == 0)
		return;

	// remove foreign key if foreign_table_name == NULL
	if (foreign_table_name == NULL) {
		if (column)
			sq_foreign_free(column->foreign);
		column->foreign = NULL;
		return;
	}

	if (column->foreign == NULL) {
		column->foreign = malloc(sizeof(SqForeign));
		column->foreign->on_delete = NULL;
		column->foreign->on_update = NULL;
	}
	else {
		free(column->foreign->table);
		free(column->foreign->column);
	}

	column->foreign->table = strdup(foreign_table_name);
	column->foreign->column = strdup(foreign_column_name);
}

// foreign key on delete
void  sq_column_on_delete(SqColumn* column, const char* act)
{
	if ((column->bit_field & SQB_DYNAMIC) == 0)
		return;
	if (column->foreign)
		column->foreign->on_delete = strdup(act);
}

// foreign key on update
void  sq_column_on_update(SqColumn* column, const char* act)
{
	if ((column->bit_field & SQB_DYNAMIC) == 0)
		return;
	if (column->foreign)
		column->foreign->on_update = strdup(act);
}

void  sq_column_set_constraint(SqColumn* column, ...)
{
	va_list  arg_list;

	va_start(arg_list, column);
	sq_column_set_constraint_va(column, arg_list);
	va_end(arg_list);
}

void  sq_column_set_constraint_va(SqColumn* column, va_list arg_list)
{
	const char*  name;
	int   index, allocated;

	if ((column->bit_field & SQB_DYNAMIC) == 0)
		return;

	if (column->constraint == NULL) {
		column->constraint = sq_constraint_alloc(N_CONSTRAINT_LEN);
		allocated = N_CONSTRAINT_LEN;
	}
	else {
		allocated = sq_constraint_allocated(column->constraint);
		for (index = 0;  column->constraint[index];  index++)
			free(column->constraint[index]);
	}

	index = 0;
	do {
		// add string to null terminated string array 
		name = va_arg(arg_list, const char*);
		if (index == allocated) {
			allocated *= 2;
			column->constraint = sq_constraint_realloc(column->constraint, allocated);
			sq_constraint_allocated(column->constraint) = allocated;
		}
		column->constraint[index++] = name ? strdup(name) : NULL;
	} while (name);
}
