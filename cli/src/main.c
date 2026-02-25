#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "urage.h"

void print_help() {
    printf("\nURAGE Database CLI\n");
    printf("===================\n");
    
    printf("\n📦 Type System Commands:\n");
    printf("  define <name> { <fields> }  - Define a new struct type\n");
    printf("  undefine <name>             - Delete a type definition\n");
    printf("  structs                     - List all defined types\n");
    printf("  desc <name>                  - Describe a type's fields\n");
    
    printf("\n📝 Data Commands (with types):\n");
    printf("  add <type> <key> <field=value...>  - Add typed data\n");
    printf("  get <type> <key>                   - Get typed data\n");
    
    printf("\n🔢 Numeric Key Commands:\n");
    printf("  addn <key> <value>           - Insert with numeric key\n");
    printf("  getn <key>                    - Retrieve with numeric key\n");
    printf("  deln <key>                    - Delete with numeric key\n");
    printf("  existsn <key>                  - Check numeric key\n");

    printf("\n🔤 String Key Commands:\n");
    printf("  adds <key> <value>        - Insert with string key\n");
    printf("  gets <key>                 - Retrieve with string key\n");
    printf("  dels <key>                 - Delete with string key\n");
    printf("  exists_str <key>            - Check string key\n");
    
    printf("\n📊 General Commands:\n");
    printf("  stats                       - Show database stats\n");
    printf("  sync                        - Flush to disk\n");
    printf("  help                        - Show this help\n");
    printf("  exit                        - Exit program\n");
}

int main(int argc, char* argv[]) {
    const char* db_path = argc > 1 ? argv[1] : "mydb";
    
    printf("Opening database: %s\n", db_path);
    
    urage_db_t* db = urage_open(db_path, 0);
    if (!db) {
        printf("Failed to open database!\n");
        return 1;
    }
    
    printf("URAGE Database ready. Type 'help' for commands.\n");
    
    char line[512];
    char cmd[32];
    char type_name[64];
    char field_name[64];
    char field_value[256];
    uint32_t key;
    char value[256];
    
    while (1) {
        printf("\nurage> ");
        fflush(stdout);
        
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) == 0) continue;
        
        // Parse command
        if (sscanf(line, "%s", cmd) != 1) continue;
        
        // ===== TYPE SYSTEM COMMANDS =====
        if (strcmp(cmd, "define") == 0) {
            // Parse: define name { fields }
            char name[64];
            char fields[256];
            
            // Very simple parsing - in reality you'd want better
            if (sscanf(line, "define %63s { %[^}]", name, fields) == 2) {
                printf("Defining type '%s' with fields: %s\n", name, fields);
                printf("Type system not fully implemented yet\n");
                // TODO: Parse fields and call urage_define_type
            } else {
                printf("Usage: define <name> { field1 type field2 type ... }\n");
                printf("Example: define game { id int score int winner string(64) }\n");
            }
        }
        else if (strcmp(cmd, "undefine") == 0) {
            if (sscanf(line, "undefine %63s", type_name) == 1) {
                printf("Deleting type '%s'\n", type_name);
                // TODO: Call urage_undefine_type
            } else {
                printf("Usage: undefine <name>\n");
            }
        }
        else if (strcmp(cmd, "structs") == 0) {
            printf("Defined types:\n");
            // TODO: Call urage_list_types
            printf("  (type listing not yet implemented)\n");
        }
        else if (strcmp(cmd, "desc") == 0) {
            if (sscanf(line, "desc %63s", type_name) == 1) {
                printf("Description of type '%s':\n", type_name);
                // TODO: Call urage_get_type and display fields
            } else {
                printf("Usage: desc <name>\n");
            }
        }
        
        // ===== TYPED DATA COMMANDS =====
        else if (strcmp(cmd, "add") == 0) {
            // Parse: add type key field=value field=value ...
            char type[64];
            unsigned int key_val;
            char field_data[256];
            
            // This is simplified - you'd need better parsing
            if (sscanf(line, "add %63s %u %[^\n]", type, &key_val, field_data) >= 2) {
                urage_result_t r = urage_add_typed(db, type, key_val, field_data);
                if (r == URAGE_OK)
                    printf("OK: %s:%u stored\n", type, key_val);
                else if (r == URAGE_NOT_FOUND)
                    printf("Type '%s' not defined\n", type);
                else
                    printf("Error: %d\n", r);
            } else {
                printf("Usage: add <type> <key> field=value ...\n");
            }
        }
        else if (strcmp(cmd, "get") == 0) {
            // Parse: get type key
            if (sscanf(line, "get %63s %u", type_name, &key) == 2) {
                char output[512];
                urage_result_t r = urage_get_typed(db, type_name, key, output, sizeof(output));
                if (r == URAGE_OK) {
                    printf("%s:%u -> %s\n", type_name, key, output);
                } else if (r == URAGE_NOT_FOUND) {
                    printf("%s:%u not found\n", type_name, key);
                } else {
                    printf("Error: %d\n", r);
                }
            } else {
                printf("Usage: get <type> <key>\n");
            }
        }
        
        // ===== NUMERIC KEY COMMANDS =====
        else if (strcmp(cmd, "addn") == 0) {
            if (sscanf(line, "%*s %u %255s", &key, value) == 2) {
                urage_result_t r = urage_add(db, key, value, strlen(value) + 1);
                if (r == URAGE_OK)
                    printf("OK: %u -> %s\n", key, value);
                else
                    printf("Error: %d\n", r);
            } else {
                printf("Usage: addn <key> <value>\n");
            }
        }
        else if (strcmp(cmd, "getn") == 0) {
            if (sscanf(line, "%*s %u", &key) == 1) {
                char buffer[256];
                size_t size = sizeof(buffer);
                urage_result_t r = urage_get(db, key, buffer, &size);
                if (r == URAGE_OK) {
                    buffer[size] = '\0';
                    printf("%u -> %s\n", key, buffer);
                } else if (r == URAGE_NOT_FOUND) {
                    printf("Key %u not found\n", key);
                } else {
                    printf("Error: %d\n", r);
                }
            } else {
                printf("Usage: getn <key>\n");
            }
        }
        else if (strcmp(cmd, "deln") == 0) {
            if (sscanf(line, "%*s %u", &key) == 1) {
                urage_result_t r = urage_delete(db, key);
                if (r == URAGE_OK)
                    printf("Key %u deleted\n", key);
                else if (r == URAGE_NOT_FOUND)
                    printf("Key %u not found\n", key);
                else
                    printf("Error: %d\n", r);
            } else {
                printf("Usage: deln <key>\n");
            }
        }
        else if (strcmp(cmd, "existsn") == 0) {
            if (sscanf(line, "%*s %u", &key) == 1) {
                int exists = urage_exists(db, key);
                printf("Key %u %s\n", key, exists ? "exists" : "does not exist");
            } else {
                printf("Usage: existsn <key>\n");
            }
        }

        // ===== STRING KEY COMMANDS =====
else if (strcmp(cmd, "adds") == 0) {
    char str_key[256];
    if (sscanf(line, "%*s %255s %255s", str_key, value) == 2) {
        urage_result_t r = urage_put_str(db, str_key, value, strlen(value) + 1);
        if (r == URAGE_OK)
            printf("OK: '%s' -> %s\n", str_key, value);
        else
            printf("Error: %d\n", r);
    } else {
        printf("Usage: adds <str_key> <value>\n");
    }
}
else if (strcmp(cmd, "gets") == 0) {
    char str_key[256];
    if (sscanf(line, "%*s %255s", str_key) == 1) {
        char buffer[256];
        size_t size = sizeof(buffer);
        urage_result_t r = urage_get_str(db, str_key, buffer, &size);
        if (r == URAGE_OK) {
            buffer[size] = '\0';
            printf("'%s' -> %s\n", str_key, buffer);
        } else if (r == URAGE_NOT_FOUND) {
            printf("Key '%s' not found\n", str_key);
        } else {
            printf("Error: %d\n", r);
        }
    } else {
        printf("Usage: gets <str_key>\n");
    }
}
else if (strcmp(cmd, "dels") == 0) {
    char str_key[256];
    if (sscanf(line, "%*s %255s", str_key) == 1) {
        urage_result_t r = urage_del_str(db, str_key);
        if (r == URAGE_OK)
            printf("Key '%s' deleted\n", str_key);
        else if (r == URAGE_NOT_FOUND)
            printf("Key '%s' not found\n", str_key);
        else
            printf("Error: %d\n", r);
    } else {
        printf("Usage: dels <str_key>\n");
    }
}
else if (strcmp(cmd, "exists_str") == 0) {
    char str_key[256];
    if (sscanf(line, "%*s %255s", str_key) == 1) {
        int exists = urage_exists_str(db, str_key);
        printf("Key '%s' %s\n", str_key, exists ? "exists" : "does not exist");
    } else {
        printf("Usage: exists_str <str_key>\n");
    }
}
        
        // ===== GENERAL COMMANDS =====
        else if (strcmp(cmd, "stats") == 0) {
            urage_stats_t stats;
            if (urage_stats(db, &stats) == URAGE_OK) {
                printf("Database Statistics:\n");
                printf("  Keys: %llu\n", (unsigned long long)stats.keys_count);
                printf("  B-tree height: %u\n", stats.btree_height);
                printf("  Data size: %llu bytes\n", (unsigned long long)stats.data_size);
                printf("  Pages: %u\n", stats.page_count);
            } else {
                printf("Failed to get stats\n");
            }
        }
        else if (strcmp(cmd, "sync") == 0) {
            if (urage_sync(db) == URAGE_OK)
                printf("Synced to disk\n");
            else
                printf("Sync failed\n");
        }
        else if (strcmp(cmd, "help") == 0) {
            print_help();
        }
        else if (strcmp(cmd, "exit") == 0) {
            break;
        }
        else {
            printf("Unknown command. Type 'help'\n");
        }
    }
    
    urage_close(db);
    printf("Database closed.\n");
    return 0;
}