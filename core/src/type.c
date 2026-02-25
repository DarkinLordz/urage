#include "type.h"
#include "urage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// System table name for storing types
#define SYSTEM_TYPES "_types"

// Forward declarations
static uint32_t next_type_id = 1;

// ==================== TYPE CREATION ====================

urage_result_t type_create(urage_db_t* db, const char* name, 
                           FieldDef* fields, uint32_t field_count) {
    if (!db || !name || !fields || field_count == 0) 
        return URAGE_INVALID_ARG;
    
    // Calculate total size needed
    uint32_t total_size = 0;
    uint32_t string_count = 0;
    
    for (uint32_t i = 0; i < field_count; i++) {
        fields[i].offset = total_size;
        
        if (fields[i].type == TYPE_INT) {
            total_size += 4;  // int = 4 bytes
        } else if (fields[i].type == TYPE_STRING) {
            total_size += fields[i].size;  // string size as defined
            string_count++;
        }
    }
    
    // Allocate memory for type definition
    size_t type_size = sizeof(TypeDef) + (field_count * sizeof(FieldDef));
    TypeDef* type = (TypeDef*)malloc(type_size);
    if (!type) return URAGE_ERROR;
    
    // Fill type definition
    type->id = next_type_id++;
    strncpy(type->name, name, 63);
    type->name[63] = '\0';
    type->size = total_size;
    type->field_count = field_count;
    memcpy(type->fields, fields, field_count * sizeof(FieldDef));
    
    // Store in system table using string key
    char type_key[256];
    snprintf(type_key, sizeof(type_key), "type:%s", name);
    
    urage_result_t result = urage_put_str(db, type_key, type, type_size);
    
    free(type);
    return result;
}

// ==================== TYPE RETRIEVAL ====================

TypeDef* type_get(urage_db_t* db, const char* name) {
    if (!db || !name) return NULL;
    
    char type_key[256];
    snprintf(type_key, sizeof(type_key), "type:%s", name);
    
    // First, get the size needed
    size_t size = 0;
    urage_result_t result = urage_get_str(db, type_key, NULL, &size);
    if (result != URAGE_ERROR || size == 0) {
        // Key doesn't exist or error
        return NULL;
    }
    
    // Allocate buffer for type
    TypeDef* type = (TypeDef*)malloc(size);
    if (!type) return NULL;
    
    // Get the actual data
    result = urage_get_str(db, type_key, type, &size);
    if (result != URAGE_OK) {
        free(type);
        return NULL;
    }
    
    return type;
}

// ==================== TYPE DELETION ====================

urage_result_t type_delete(urage_db_t* db, const char* name) {
    if (!db || !name) return URAGE_INVALID_ARG;
    
    char type_key[256];
    snprintf(type_key, sizeof(type_key), "type:%s", name);
    
    // Also need to delete all data of this type
    // For now, just delete the type definition
    return urage_del_str(db, type_key);
}

// ==================== TYPE LISTING ====================

urage_result_t type_list(urage_db_t* db, char*** names, uint32_t* count) {
    if (!db || !names || !count) return URAGE_INVALID_ARG;
    
    // This would require iteration over all type: keys
    // For now, return not implemented
    *names = NULL;
    *count = 0;
    return URAGE_OK;  // TODO: Implement iteration
}

// ==================== FIELD PARSING ====================

urage_result_t type_parse_fields(const char* input, FieldDef** fields, 
                                  uint32_t* field_count) {
    if (!input || !fields || !field_count) return URAGE_INVALID_ARG;
    
    // This is a complex parser - for now, return not implemented
    // We'll implement this when adding the CLI parser
    return URAGE_OK;
}

// ==================== DATA SERIALIZATION ====================

urage_result_t type_serialize(FieldDef* fields, uint32_t field_count,
                              const char* field_values, void* buffer) {
    if (!fields || !buffer) return URAGE_INVALID_ARG;
    
    // Parse field=value pairs and write to buffer at offsets
    // This will be implemented with the CLI
    return URAGE_OK;
}

urage_result_t type_deserialize(FieldDef* fields, uint32_t field_count,
                                const void* buffer, char* output, size_t output_size) {
    if (!fields || !buffer || !output) return URAGE_INVALID_ARG;
    
    // Convert binary data back to field=value strings
    // This will be implemented with the CLI
    return URAGE_OK;
}