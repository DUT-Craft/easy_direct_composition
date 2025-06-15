#include "obj_tree.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

static Object_Hash_Map obj_key_map = { 0 };
char32_t const* const OBJ_TYPE = U"@object.type";
Object_Key OBJ_TYPE_KEY = 0;
char32_t const* const OBJ_KEY_HASH_FUNCTION = U"@object.hash_function";
Object_Key OBJ_KEY_HASH_FUNCTION_KEY = 0;
char32_t const* const OBJ_KEY_COMPARE_EQ_FUNCTION = U"@object.compare_eq_function";
Object_Key OBJ_KEY_COMPARE_EQ_FUNCTION_KEY = 0;
char32_t const* const OBJ_DESTRUCT_FUNCTION = U"@object.destruct_function";
Object_Key OBJ_DESTRUCT_FUNCTION_KEY = 0;

static Object* obj_char32_string_type_obj = NULL;

static void obj_free(Object* obj);
static void u32strcpy(char32_t* dst, const char32_t* src);
static size_t u32strlen(const char32_t* s);
static void* (*obj_mem_allocate)(size_t size) = malloc;
static void (*obj_mem_deallocate)(void*) = free;


Object* obj_get_char32_string_type() {
    return obj_char32_string_type_obj;
}

Object* obj_create_char32_string(const char32_t* str)
{
    size_t len = u32strlen(str) + 1;
    Object* obj = obj_create(len * sizeof(char32_t), _Alignof(char32_t));
    memcpy(obj->data, str, len * sizeof(char32_t));
    obj_debug_add_tag(obj, obj->data);
    return obj;
}

static void init_hash_map(Object_Hash_Map* map, size_t bucket_count_log2) {
    map->buckets = (Object_Hash_Item**)calloc((size_t)1 << bucket_count_log2, sizeof(Object_Hash_Item*));
    if (!map->buckets) return;
    map->bucket_count_log2 = bucket_count_log2;
    map->obj_count = 0;
}

static Object_Key hash_char32_append(Object_Key key1, const char32_t* str) {
    intptr_t hash = key1;
    while (*str) {
        hash *= 31;
        hash += *str++;
    }
    return hash | OBJ_KEY_HASH_FLAG;
}

static Object_Key hash_char32_string(Closure_Data* self, const char32_t* str) {
    return hash_char32_append(0, str);
}

static int compare_eq_char32_string(Closure_Data* self, void const* data1, void const* data2) {
    char32_t* a = (char32_t*)data1, * b = (char32_t*)data2;
    while (*a == *b) {
        if (*a == 0) return 1;
        a++;
        b++;
    }
    return 0;
}

static size_t u32strlen(const char32_t* s) {
    int len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

static void u32strcpy(char32_t* dst, const char32_t* src) {
    while (*src) {
        *dst++ = *src++;
    }
    *dst = 0;
}


static Object* obj_get_attr_internal(Object_Hash_Map* map, Object_Key key) {
    if (!map->buckets) return NULL;
    size_t index = key & ((1 << map->bucket_count_log2) - 1);
    Object_Hash_Item* item = map->buckets[index];
    while (item) {
        if (item->key == key) {
            return item->value;
        }
        item = item->next;
    }
    return NULL;
}

static void obj_add_attr_internal(Object_Hash_Map* map, Object_Key key, Object* value) {
    if (!map->buckets)
    {
        init_hash_map(map, OBJ_DEFAULT_BUCKET_COUNT_LOG2);
        assert(map->buckets);
    }
    size_t index = key & ((1 << map->bucket_count_log2) - 1);
    Object_Hash_Item* item = (Object_Hash_Item*)malloc(sizeof(Object_Hash_Item));
    if (!item) return;
    item->key = key;
    item->value = value;
    item->next = map->buckets[index];
    map->buckets[index] = item;
    map->obj_count++;
    value->ref_count++;
}

static Object_Key obj_attr_hash_internal(Closure_Data* hash_func, Closure_Data* comp_eq_func, Object* obj)
{
    assert(obj_key_map.buckets);
    Object_Key key = ((Object_Key_Hash_Function)hash_func->func)(hash_func, obj->data);
    Object* current_obj, * obj_type = obj_get_type(obj);
    while ((current_obj = obj_get_attr_internal(&obj_key_map, key)) != NULL) {
        if (obj_get_type(current_obj) != obj_type) continue;
        if (current_obj == obj) return key;
        if (comp_eq_func && ((Object_Key_Compare_Equal_Function)comp_eq_func->func)(comp_eq_func, obj->data, current_obj->data)) return key;
        key++;
    }
    obj_add_attr_internal(&obj_key_map, key, obj);
    return key;
}

void obj_inc_ref(Object *obj) {
    if (!obj) return;
    obj->ref_count++;
}

void obj_dec_ref(Object* obj) {
    if (!obj) return;
    obj->ref_count--;
    if (obj->ref_count > 0) return;
    obj_free(obj);
}

Closure_Data* obj_query_method(Object* obj, Object_Key key) {
    Object* method_obj = obj_get_attr_internal(&obj->attrs_and_children, key);
    if (method_obj) return method_obj->data;
    Object* obj_type = obj_get_type(obj);
    if (obj_type)
    {
        method_obj = obj_get_attr_internal(&obj_type->attrs_and_children, key);
        if (method_obj) return method_obj->data;
    }
    return NULL;
}

Object_Key obj_attr_key_hash(Object* obj) {
    Closure_Data* hash_func = obj_query_method(obj, OBJ_KEY_HASH_FUNCTION_KEY);
    if (!hash_func) return 0;
    Closure_Data* comp_eq_func = obj_query_method(obj, OBJ_KEY_COMPARE_EQ_FUNCTION_KEY);
    return OBJ_KEY_HASH_FLAG | obj_attr_hash_internal(hash_func, comp_eq_func, obj);
}

Object_Key obj_attr_hash_string(const char32_t* str) {
    assert(obj_key_map.buckets);
    Object_Key key = hash_char32_string(NULL, str) | OBJ_KEY_HASH_FLAG;
    Object* current_obj;
    while ((current_obj = obj_get_attr_internal(&obj_key_map, key)) != NULL) {
        if (obj_get_type(current_obj) != obj_char32_string_type_obj) continue;
        if (compare_eq_char32_string(NULL, str, current_obj->data)) return key;
        key++;
    }
    Object* obj = obj_create(sizeof(char32_t) * (u32strlen(str) + 1), _Alignof(char32_t));
    if (!obj) return 0;
    u32strcpy(obj->data, str);
    obj_debug_add_tag(obj, obj->data);
    obj_set_type(obj, obj_char32_string_type_obj);
    obj_add_attr_internal(&obj_key_map, key, obj);
    return key;
}

void obj_init_key_map() {

    // start bootstrap
    init_hash_map(&obj_key_map, OBJ_DEFAULT_BUCKET_COUNT_LOG2);
    OBJ_KEY_HASH_FUNCTION_KEY = hash_char32_string(NULL, OBJ_KEY_HASH_FUNCTION);
    Object* hash_char32_string_obj = obj_create(0, 0);
    hash_char32_string_obj->data = (void*)OBJ_KEY_HASH_FUNCTION_KEY;
    Object* hash_char32_func_obj = obj_create(sizeof(Closure_Data), _Alignof(Closure_Data));
    obj_debug_add_tag(hash_char32_string_obj, OBJ_KEY_HASH_FUNCTION);
    obj_debug_add_tag(hash_char32_func_obj, OBJ_KEY_HASH_FUNCTION);
    ((Closure_Data*)hash_char32_func_obj->data)->func = (void*)hash_char32_string;

    OBJ_KEY_COMPARE_EQ_FUNCTION_KEY = hash_char32_string(NULL, OBJ_KEY_COMPARE_EQ_FUNCTION);
    Object* compare_eq_char32_string_obj = obj_create(0, 0);
    compare_eq_char32_string_obj->data = (void*)OBJ_KEY_COMPARE_EQ_FUNCTION_KEY;
    Object* compare_eq_char32_func_obj = obj_create(sizeof(Closure_Data), _Alignof(Closure_Data));
    obj_debug_add_tag(compare_eq_char32_string_obj, OBJ_KEY_COMPARE_EQ_FUNCTION);
    obj_debug_add_tag(compare_eq_char32_func_obj, OBJ_KEY_COMPARE_EQ_FUNCTION);
    ((Closure_Data*)compare_eq_char32_func_obj->data)->func = (void*)compare_eq_char32_string;

    OBJ_TYPE_KEY = hash_char32_string(NULL, OBJ_TYPE);
    Object* obj_type_obj = obj_create(0, 0);
    obj_debug_add_tag(obj_type_obj, OBJ_TYPE);
    obj_type_obj->data = (void*)OBJ_TYPE_KEY;
    obj_add_attr_internal(&obj_key_map, OBJ_TYPE_KEY, obj_type_obj);

    obj_char32_string_type_obj = obj_create(0, 0);
    obj_debug_add_tag(obj_char32_string_type_obj, U"@object.char32_string_type_obj");
    obj_set_type(hash_char32_string_obj, obj_char32_string_type_obj);
    obj_set_type(compare_eq_char32_string_obj, obj_char32_string_type_obj);
    obj_add_attr_internal(&obj_char32_string_type_obj->attrs_and_children, OBJ_KEY_HASH_FUNCTION_KEY, hash_char32_func_obj);
    obj_add_attr_internal(&obj_char32_string_type_obj->attrs_and_children, OBJ_KEY_COMPARE_EQ_FUNCTION_KEY, compare_eq_char32_func_obj);
    // end bootstrap

    OBJ_DESTRUCT_FUNCTION_KEY = obj_attr_hash_string(OBJ_DESTRUCT_FUNCTION);

}

Object* obj_create(size_t size, size_t align_req) {
    if (align_req == 0) align_req = 1;
    size_t base_size = sizeof(Object);
    size_t padding = (align_req - (base_size % align_req)) % align_req;
    if (size == 0) padding = 0;
    Object* obj = (Object*)obj_mem_allocate(base_size + padding + size);
    if (!obj) return NULL;
    obj->attrs_and_children.buckets = NULL;
    obj->attrs_and_children.bucket_count_log2 = 0;
    obj->attrs_and_children.obj_count = 0;
    obj->parent = NULL;
    if (size) obj->data = (char*)obj + base_size + padding;
    else obj->data = NULL;
    obj->data_size = 0; // 0 for static memory
    obj->ref_count = 0;
    obj->debug_tag = NULL;
    return obj;
}

static void obj_clear_attr(Object* obj) {
    if (!obj) return;
    if (obj->attrs_and_children.buckets) {
        for (size_t i = 0; i < obj->attrs_and_children.bucket_count_log2; i++) {
            Object_Hash_Item* item = obj->attrs_and_children.buckets[i];
            while (item) {
                Object_Hash_Item* next = item->next;
                obj_dec_ref(item->value);
                item = next;
            }
        }
        free(obj->attrs_and_children.buckets);
    }
    obj->attrs_and_children.buckets = NULL;
    obj->attrs_and_children.bucket_count_log2 = 0;
    obj->attrs_and_children.obj_count = 0;
}

void obj_reset(Object* obj) {
    if (!obj) return;
    Closure_Data* destruct_func = obj_query_method(obj, OBJ_DESTRUCT_FUNCTION_KEY);
    if (destruct_func) {
        ((Object_Destruct_Function)destruct_func->func)(destruct_func, obj);
    }else if (obj->data_size > 0) {
        free(obj->data);
        obj->data_size = 0;
    }
    obj->data = NULL;
}

static void obj_free(Object* obj) {
    if (!obj) return;
    if (obj->ref_count > 0) return;
    obj_reset(obj);
    if (obj->attrs_and_children.buckets) obj_clear_attr(obj);
    obj_mem_deallocate(obj);
}

Object* obj_get_type(Object* obj) {
    return obj_get_attr_internal(&obj->attrs_and_children, OBJ_TYPE_KEY);
}

void obj_set_type(Object* obj, Object* type) {
    if (!obj) return;
    if (!type) return;
    obj_add_attr_internal(&obj->attrs_and_children, OBJ_TYPE_KEY, type);
}

void obj_add_attr(Object* obj, Object_Key key, Object* value) {
    if (!obj) return;
    if (!value) return;
    obj_add_attr_internal(&obj->attrs_and_children, key, value);
}

Object* obj_get_attr(Object* obj, Object_Key key) {
    if (!obj) return NULL;
    return obj_get_attr_internal(&obj->attrs_and_children, key);
}

Object_Attr_Iterator obj_attr_iter_begin(Object* obj) {
    Object_Attr_Iterator iter;
    iter.obj = obj;
    iter.bucket_index = 0;
    iter.item = NULL;
    return obj_attr_iter_next(iter);
}

Object_Attr_Iterator obj_attr_iter_end(Object* obj) {
    Object_Attr_Iterator iter;
    iter.obj = obj;
    iter.bucket_index = (size_t)1 << obj->attrs_and_children.bucket_count_log2;
    iter.item = NULL;
    return iter;
}

Object_Attr_Iterator obj_attr_iter_next(Object_Attr_Iterator iter) {
    if (!iter.obj) return iter;
    if (!iter.obj->attrs_and_children.buckets) return iter; // empty
    if (iter.item) {
        iter.item = iter.item->next;
        if (iter.item) return iter; // same bucket
    }
    else if (iter.bucket_index == 0)
    {
        // first bucket
        iter.item = iter.obj->attrs_and_children.buckets[iter.bucket_index];
        if (iter.item) return iter;
    }
    while (iter.bucket_index < ((size_t)1 << iter.obj->attrs_and_children.bucket_count_log2)) {
        iter.item = iter.obj->attrs_and_children.buckets[iter.bucket_index++];
        if (iter.item) return iter;
    }
    return iter;
}

int obj_attr_iter_equal(Object_Attr_Iterator iter1, Object_Attr_Iterator iter2) {
    if (iter1.obj != iter2.obj) return 0;
    if (iter1.bucket_index != iter2.bucket_index) return 0;
    if (iter1.item != iter2.item) return 0;
    return 1;
}

char* obj_str32_to_utf8(const char32_t* u32str) {
    mbstate_t state = { 0 };
    const char32_t* p = u32str;

    size_t max_len = 0;
    while (*p != U'\0') {
        max_len += 4;
        p++;
    }
    max_len += 4;

    char* utf8 = malloc(max_len);
    if (!utf8) return NULL;

    char* dst = utf8;
    p = u32str;
    while (1) {
        size_t bytes = c32rtomb(dst, *p, &state);
        if (bytes == (size_t)-1) {
            free(utf8);
            return NULL;
        }
        dst += bytes;
        if (dst > utf8 + max_len) {
            free(utf8);
            return NULL;
        }

        if (*p == U'\0') break;
        p++;
    }

    return utf8;
}

void obj_debug_set_allocator(void* (*allocate)(size_t size), void(*deallocate)(void*))
{
    obj_mem_allocate = allocate;
    obj_mem_deallocate = deallocate;
}

void obj_debug_add_tag(Object* obj, const char32_t* tag)
{
    obj->debug_tag = tag;
}

