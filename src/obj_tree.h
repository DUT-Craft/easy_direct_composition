/**
 * @file obj_tree.h
 * @brief Object Tree Management System
 * @version 1.0.0
 *
 * This module provides a C-based object tree system with reference counting,
 * attribute management, and type system support. It serves as the foundation
 * for the Easy Direct Composition library.
 *
 * Features:
 * - Reference counting for automatic memory management
 * - Hash-based attribute storage
 * - Type system with custom destructors
 * - UTF-32 string support
 * - Debug support with object tagging
 */

#ifndef _OBJ_TREE_H_
#define _OBJ_TREE_H_

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OBJ_DEFAULT_BUCKET_COUNT_LOG2 4
#define OBJ_KEY_HASH_FLAG INTPTR_MIN
#define OBJ_KEY_IS_HASH(key) ((key) & OBJ_KEY_HASH_FLAG)
#define OBJ_KEY_IS_CHILD(key) (!OBJ_KEY_IS_HASH(key))


    typedef intptr_t Object_Key;
    extern Object_Key OBJ_DESTRUCT_FUNCTION_KEY;

    struct s_Closure_Data;
    struct s_Object;
    typedef Object_Key(*Object_Key_Hash_Function)(struct s_Closure_Data* self, void* obj_data);
    typedef int (*Object_Key_Compare_Equal_Function)(struct s_Closure_Data* self, void* obj_data1, void* obj_data2);
    typedef void (*Object_Destruct_Function)(struct s_Closure_Data* self, struct s_Object* obj);

    typedef struct s_Closure_Data {
        void* func;
    } Closure_Data;

    typedef struct s_Object_Hash_Item {
        Object_Key key;
        struct s_Object* value;
        struct s_Object_Hash_Item* next;
    } Object_Hash_Item;

    typedef struct s_Object_Hash_Map {
        Object_Hash_Item** buckets;
        size_t bucket_count_log2;
        size_t obj_count;
    } Object_Hash_Map;

    typedef struct s_Object {
        Object_Hash_Map attrs_and_children;
        struct s_Object* parent; //nullable
        void* data;         // NULL for empty object
        size_t data_size;   // 0 for static memory
        size_t ref_count;
        const char32_t* debug_tag;
    } Object;

    typedef struct s_Object_Attr_Iterator {
        Object* obj;
        size_t bucket_index;
        Object_Hash_Item* item;
    } Object_Attr_Iterator;

    void obj_init_key_map();

    Object* obj_create(size_t size, size_t align_req);
    void obj_add_attr(Object* obj, Object_Key key, Object* value);
    Object* obj_get_attr(Object* obj, Object_Key key);
    void obj_reset(Object* obj);
    void obj_inc_ref(Object* obj);
    void obj_dec_ref(Object* obj);
    Object* obj_get_type(Object* obj);
    void obj_set_type(Object* obj, Object* type);

    Object_Key obj_attr_key_hash(Object* obj);
    Object_Key obj_attr_hash_string(const char32_t* str);

    Closure_Data* obj_query_method(Object* obj, Object_Key key);

    Object_Attr_Iterator obj_attr_iter_begin(Object* obj);
    Object_Attr_Iterator obj_attr_iter_end(Object* obj);
    Object_Attr_Iterator obj_attr_iter_next(Object_Attr_Iterator iter);
    int obj_attr_iter_equal(Object_Attr_Iterator iter1, Object_Attr_Iterator iter2);

    Object * obj_get_char32_string_type();
    Object* obj_create_char32_string(const char32_t* str);
    char* obj_str32_to_utf8(const char32_t* u32str);

    void obj_debug_set_allocator(void *(*allocate)(size_t size), void (*deallocate)(void*));
    void obj_debug_add_tag(Object* obj, const char32_t* tag);

#ifdef __cplusplus
}
#endif

#endif