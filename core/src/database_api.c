#include "urage.h"
#include "database.h" 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "type.h"
#include "type_funcs.h"

struct urage_db {
    Database* internal_db;
    char last_error[256];
    char* path;
};

// ==================== LIFECYCLE ====================

URAGE_API urage_db_t* urage_open(const char* path, unsigned flags) {
    (void)flags;  // Unused for now
    
    urage_db_t* db = (urage_db_t*)calloc(1, sizeof(urage_db_t));
    if (!db) return NULL;
    
    db->path = strdup(path);
    
    // Call your existing database code
    db->internal_db = db_open(path);
    if (!db->internal_db) {
        snprintf(db->last_error, sizeof(db->last_error), 
                 "Failed to open database: %s", path);
        free(db->path);
        free(db);
        return NULL;
    }
    
    return db;
}

URAGE_API void urage_close(urage_db_t* db) {
    if (!db) return;
    
    if (db->internal_db) {
        db_close(db->internal_db);
    }
    
    free(db->path);
    free(db);
}

// ==================== CRUD ====================

URAGE_API urage_result_t urage_add(urage_db_t* db, uint32_t key,
                                   const void* value, size_t value_size) {
    if (!db || !db->internal_db) return URAGE_CLOSED;
    
    DB_Result result = db_insert(db->internal_db, key, value, value_size);
    
    switch (result) {
        case DB_SUCCESS: return URAGE_OK;
        case DB_FULL: return URAGE_FULL;
        case DB_IO_ERROR: return URAGE_IO_ERROR;
        case DB_MEMORY_ERROR: return URAGE_ERROR;
        default: return URAGE_ERROR;
    }
}

URAGE_API urage_result_t urage_get(urage_db_t* db, uint32_t key,
                                   void* buffer, size_t* buffer_size) {
    if (!db || !db->internal_db) return URAGE_CLOSED;
    if (!buffer || !buffer_size) return URAGE_INVALID_ARG;
    
    DB_Result result = db_find(db->internal_db, key, buffer, buffer_size);
    
    switch (result) {
        case DB_SUCCESS: return URAGE_OK;
        case DB_NOT_FOUND: return URAGE_NOT_FOUND;
        case DB_ERROR: return URAGE_ERROR;
        default: return URAGE_ERROR;
    }
}

URAGE_API urage_result_t urage_delete(urage_db_t* db, uint32_t key) {
    if (!db || !db->internal_db) return URAGE_CLOSED;
    
    DB_Result result = db_delete(db->internal_db, key);
    
    switch (result) {
        case DB_SUCCESS: return URAGE_OK;
        case DB_NOT_FOUND: return URAGE_NOT_FOUND;
        default: return URAGE_ERROR;
    }
}

URAGE_API int urage_exists(urage_db_t* db, uint32_t key) {
    if (!db || !db->internal_db) return 0;
    
    char dummy[256];  // ← Make it larger
    size_t size = sizeof(dummy);
    DB_Result result = db_find(db->internal_db, key, dummy, &size);
    
    // If it returns DB_SUCCESS, key exists
    // If it returns DB_NOT_FOUND, key doesn't exist
    // If it returns DB_ERROR but size > 0, key exists but buffer too small
    return (result == DB_SUCCESS) || (result == DB_ERROR && size > 0);
}

// ==================== CURSOR ====================

struct urage_cursor {
    urage_db_t* db;
    // TODO: Add cursor state when implemented in core
    uint32_t current_key;
    int valid;
};

URAGE_API urage_cursor_t* urage_cursor_create(urage_db_t* db) {
    if (!db) return NULL;
    
    urage_cursor_t* cursor = (urage_cursor_t*)calloc(1, sizeof(urage_cursor_t));
    if (!cursor) return NULL;
    
    cursor->db = db;
    cursor->valid = 0;
    cursor->current_key = 0;
    
    // TODO: Initialize cursor when core supports iteration
    // For now, just return a placeholder
    
    return cursor;
}

URAGE_API urage_result_t urage_cursor_first(urage_cursor_t* cursor) {
    if (!cursor) return URAGE_INVALID_ARG;
    // TODO: Implement when core supports iteration
    cursor->valid = 0;
    return URAGE_NOT_FOUND;
}

URAGE_API urage_result_t urage_cursor_last(urage_cursor_t* cursor) {
    if (!cursor) return URAGE_INVALID_ARG;
    // TODO: Implement when core supports iteration
    cursor->valid = 0;
    return URAGE_NOT_FOUND;
}

URAGE_API urage_result_t urage_cursor_next(urage_cursor_t* cursor) {
    if (!cursor) return URAGE_INVALID_ARG;
    // TODO: Implement when core supports iteration
    return URAGE_NOT_FOUND;
}

URAGE_API urage_result_t urage_cursor_prev(urage_cursor_t* cursor) {
    if (!cursor) return URAGE_INVALID_ARG;
    // TODO: Implement when core supports iteration
    return URAGE_NOT_FOUND;
}

URAGE_API urage_result_t urage_cursor_get(urage_cursor_t* cursor,
                                         uint32_t* key,
                                         void* buffer, size_t* buffer_size) {
    if (!cursor || !cursor->valid) return URAGE_NOT_FOUND;
    if (!key || !buffer || !buffer_size) return URAGE_INVALID_ARG;
    
    // TODO: Implement when core supports iteration
    return URAGE_NOT_FOUND;
}

URAGE_API void urage_cursor_destroy(urage_cursor_t* cursor) {
    free(cursor);
}

// Simple string hash function (djb2 algorithm - very fast)
static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    
    return hash;
}


URAGE_API urage_result_t urage_put_str(urage_db_t* db, const char* key,
                                       const void* value, size_t value_size) {
    if (!db || !db->internal_db) return URAGE_CLOSED;
    if (!key) return URAGE_INVALID_ARG;
    
    // Hash the string key to uint32_t
    uint32_t hashed_key = hash_string(key);
    
    printf("Debug: String key '%s' hashed to %u\n", key, hashed_key);
    
    // Use existing urage_put with hashed key
    return urage_add(db, hashed_key, value, value_size);
}

URAGE_API urage_result_t urage_get_str(urage_db_t* db, const char* key,
                                       void* buffer, size_t* buffer_size) {
    if (!db || !db->internal_db) return URAGE_CLOSED;
    if (!key || !buffer || !buffer_size) return URAGE_INVALID_ARG;
    
    // Hash the string key to uint32_t
    uint32_t hashed_key = hash_string(key);
    
    printf("Debug: String key '%s' hashed to %u\n", key, hashed_key);
    
    // Use existing urage_get with hashed key
    return urage_get(db, hashed_key, buffer, buffer_size);
}

URAGE_API urage_result_t urage_del_str(urage_db_t* db, const char* key) {
    if (!db || !db->internal_db) return URAGE_CLOSED;
    if (!key) return URAGE_INVALID_ARG;
    
    // Hash the string key to uint32_t
    uint32_t hashed_key = hash_string(key);
    
    printf("Debug: String key '%s' hashed to %u\n", key, hashed_key);
    
    // Use existing urage_delete with hashed key
    return urage_delete(db, hashed_key);
}

URAGE_API int urage_exists_str(urage_db_t* db, const char* key) {
    if (!db || !db->internal_db) return 0;
    if (!key) return 0;
    
    // Hash the string key to uint32_t
    uint32_t hashed_key = hash_string(key);
    
    // Use existing urage_exists with hashed key
    return urage_exists(db, hashed_key);
}

// ==================== STATISTICS ====================

URAGE_API urage_result_t urage_stats(urage_db_t* db, urage_stats_t* stats) {
    if (!db || !db->internal_db) return URAGE_CLOSED;
    if (!stats) return URAGE_INVALID_ARG;
    
    Database* internal = db->internal_db;
    
    // Get page count from pager
    stats->page_count = internal->storage->pager->num_pages;
    
    // Estimate keys from B-tree root
    // This is a hack - you need proper counting
    void* root = pager_get_page(internal->index->pager, internal->index->root_page_num);
    NodeHeader* header = (NodeHeader*)root;
    
    if (header->type == NODE_LEAF) {
        LeafNode* leaf = (LeafNode*)root;
        stats->keys_count = leaf->num_cells;
    } else {
        stats->keys_count = 0;  // Would need to traverse
    }
    
    stats->btree_height = 1;  // Assume height 1
    stats->data_size = internal->storage->next_offset;
    
    return URAGE_OK;
}

URAGE_API const char* urage_error(urage_db_t* db) {
    if (!db) return "Database handle is NULL";
    return db->last_error;
}

URAGE_API urage_result_t urage_sync(urage_db_t* db) {
    if (!db || !db->internal_db) return URAGE_CLOSED;
    
    // Flush all changes to disk using your existing pager
    Database* internal = db->internal_db;
    pager_flush_all(internal->index->pager);
    pager_flush_all(internal->storage->pager);
    
    return URAGE_OK;
}

URAGE_API urage_result_t urage_add_typed(urage_db_t* db, const char* type_name,
                                         uint32_t key, const char* field_values) {
    if (!db || !db->internal_db) return URAGE_CLOSED;
    
    // Get type definition
    TypeDef* type = type_get(db, type_name);
    if (!type) return URAGE_NOT_FOUND;
    
    // Allocate buffer for the struct
    void* buffer = calloc(1, type->size);
    if (!buffer) {
        free(type);
        return URAGE_ERROR;
    }
    
    // Parse field_values and fill buffer
    urage_result_t result = type_serialize(type->fields, type->field_count, 
                                          field_values, buffer);
    
    if (result == URAGE_OK) {
        // Store with type:key prefix
        char data_key[256];
        snprintf(data_key, sizeof(data_key), "%s:%u", type_name, key);
        result = urage_put_str(db, data_key, buffer, type->size);
    }
    
    free(buffer);
    free(type);
    return result;
}

URAGE_API urage_result_t urage_get_typed(urage_db_t* db, const char* type_name,
                                         uint32_t key, char* output, size_t output_size) {
    if (!db || !db->internal_db) return URAGE_CLOSED;
    
    // Get type definition
    TypeDef* type = type_get(db, type_name);
    if (!type) return URAGE_NOT_FOUND;
    
    // Read data
    char data_key[256];
    snprintf(data_key, sizeof(data_key), "%s:%u", type_name, key);
    
    void* buffer = malloc(type->size);
    if (!buffer) {
        free(type);
        return URAGE_ERROR;
    }
    
    size_t size = type->size;
    urage_result_t result = urage_get_str(db, data_key, buffer, &size);
    
    if (result == URAGE_OK) {
        // Format output using type fields
        result = type_deserialize(type->fields, type->field_count, 
                                  buffer, output, output_size);
    }
    
    free(buffer);
    free(type);
    return result;
}