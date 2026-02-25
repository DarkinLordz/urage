#include "type.h"
#include "urage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

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
    
    for (uint32_t i = 0; i < field_count; i++) {
        fields[i].offset = total_size;
        
        if (fields[i].type == TYPE_INT) {
            total_size += 4;  // int = 4 bytes
        } else if (fields[i].type == TYPE_STRING) {
            total_size += fields[i].size;  // string size as defined
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

// ==================== FIELD PARSING ====================

static char* trim_whitespace(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    
    return str;
}

static int parse_field_definition(const char* input, FieldDef* field) {
    char field_name[64];
    char type_str[32];
    char size_str[32] = "";
    
    // Parse: name type or name type(size)
    int matched = sscanf(input, "%63s %31[^(](%31[^)])", field_name, type_str, size_str);
    if (matched < 2) {
        matched = sscanf(input, "%63s %31s", field_name, type_str);
    }
    
    if (matched < 2) return 0;
    
    strncpy(field->name, field_name, 31);
    field->name[31] = '\0';
    
    if (strcmp(type_str, "int") == 0) {
        field->type = TYPE_INT;
        field->size = 4;  // int is 4 bytes
    } else if (strcmp(type_str, "string") == 0) {
        field->type = TYPE_STRING;
        if (strlen(size_str) > 0) {
            field->size = atoi(size_str);
        } else {
            field->size = 64;  // default string size
        }
    } else {
        return 0;  // unknown type
    }
    
    return 1;
}

urage_result_t type_parse_fields(const char* input, FieldDef** fields, 
                                  uint32_t* field_count) {
    if (!input || !fields || !field_count) return URAGE_INVALID_ARG;
    
    // Count fields first
    char* input_copy = strdup(input);
    if (!input_copy) return URAGE_ERROR;
    
    uint32_t count = 0;
    char* line = strtok(input_copy, "\n");
    while (line) {
        line = trim_whitespace(line);
        if (strlen(line) > 0 && line[0] != '#') {
            count++;
        }
        line = strtok(NULL, "\n");
    }
    free(input_copy);
    
    if (count == 0) return URAGE_INVALID_ARG;
    
    // Allocate fields array
    *fields = (FieldDef*)calloc(count, sizeof(FieldDef));
    if (!*fields) return URAGE_ERROR;
    
    // Parse each field
    input_copy = strdup(input);
    if (!input_copy) {
        free(*fields);
        return URAGE_ERROR;
    }
    
    uint32_t index = 0;
    line = strtok(input_copy, "\n");
    while (line && index < count) {
        line = trim_whitespace(line);
        if (strlen(line) > 0 && line[0] != '#') {
            if (!parse_field_definition(line, &(*fields)[index])) {
                free(input_copy);
                free(*fields);
                return URAGE_ERROR;
            }
            index++;
        }
        line = strtok(NULL, "\n");
    }
    
    free(input_copy);
    *field_count = count;
    return URAGE_OK;
}

// ==================== DATA SERIALIZATION ====================

static int parse_value(const char* value_str, uint8_t type, void* output) {
    if (type == TYPE_INT) {
        *((uint32_t*)output) = (uint32_t)atoi(value_str);
        return 1;
    } else if (type == TYPE_STRING) {
        // Remove quotes if present
        const char* start = value_str;
        char* end;
        
        if (value_str[0] == '"') {
            start = value_str + 1;
            end = strrchr(value_str, '"');
            if (end) *end = '\0';
        }
        
        strcpy((char*)output, start);
        return 1;
    }
    return 0;
}

urage_result_t type_serialize(FieldDef* fields, uint32_t field_count,
                              const char* field_values, void* buffer) {
    if (!fields || !buffer || !field_values) return URAGE_INVALID_ARG;
    
    char* values_copy = strdup(field_values);
    if (!values_copy) return URAGE_ERROR;
    
    char* token = strtok(values_copy, " \t");
    while (token) {
        // Parse field=value
        char* equals = strchr(token, '=');
        if (equals) {
            *equals = '\0';
            char* field_name = token;
            char* value_str = equals + 1;
            
            // Find matching field
            for (uint32_t i = 0; i < field_count; i++) {
                if (strcmp(fields[i].name, field_name) == 0) {
                    // Write value at field offset
                    void* dest = (char*)buffer + fields[i].offset;
                    parse_value(value_str, fields[i].type, dest);
                    break;
                }
            }
        }
        token = strtok(NULL, " \t");
    }
    
    free(values_copy);
    return URAGE_OK;
}

urage_result_t type_deserialize(FieldDef* fields, uint32_t field_count,
                                const void* buffer, char* output, size_t output_size) {
    if (!fields || !buffer || !output) return URAGE_INVALID_ARG;
    
    output[0] = '\0';
    size_t pos = 0;
    
    for (uint32_t i = 0; i < field_count; i++) {
        const void* src = (const char*)buffer + fields[i].offset;
        
        if (fields[i].type == TYPE_INT) {
            uint32_t value = *((uint32_t*)src);
            pos += snprintf(output + pos, output_size - pos, 
                           "%s=%u", fields[i].name, value);
        } else if (fields[i].type == TYPE_STRING) {
            char value[256];
            strncpy(value, (const char*)src, fields[i].size);
            value[fields[i].size - 1] = '\0';
            pos += snprintf(output + pos, output_size - pos, 
                           "%s=\"%s\"", fields[i].name, value);
        }
        
        if (i < field_count - 1 && pos < output_size - 1) {
            output[pos++] = ' ';
        }
    }
    
    return URAGE_OK;
}