#include "obj_helper.h"
#include <cassert>
#include <cstddef>
#include <vector>

#include "obj_helper.h"
#include "obj_tree.h"

Easy_Object Easy_Object::root_obj = {};
Easy_Object Easy_Object::types_obj = {};
Easy_Object Easy_Object::map_type_obj = {};
Easy_Object Easy_Object::array_type_obj = {};
Object_Key Easy_Object::type_name_key;
std::unordered_map<std::type_index, Easy_Object> Easy_Object::type_db;

#ifndef _Alignof
#define _Alignof alignof
#endif

static Object *make_map_internal()
{
    Object *obj = obj_create(sizeof(Map_Data), alignof(Map_Data));
    obj_debug_add_tag(obj, U"map");
    new (obj->data) Map_Data();
    return obj;
}

static void destruct_map(Closure_Data *self, Object *obj)
{
    Map_Data *map = (Map_Data*)obj->data;
    for (auto &pair : *map) {
        obj_dec_ref(pair.second);
    }
    if (obj->data_size) delete map;
    else map->~Map_Data();
}

static void destruct_array(Closure_Data *self, Object* obj)
{
    Vector_Data *vec = (Vector_Data*)obj->data;
    for (auto &obj : *vec) {
        obj_dec_ref(obj);
    }
    if (obj->data_size) delete vec;
    else vec->~Vector_Data();
}

void Easy_Object::TypeSystemInit()
{
    if (!root_obj.is_null()) return;
    root_obj = make_map_internal();
    types_obj = make_map_internal();

    Map_Data *root_map = root_obj.get_map_data();
    root_map->insert(std::make_pair("types", types_obj.get_ptr()));
    obj_inc_ref(types_obj.get_ptr());
    Map_Data *types_map = types_obj.get_map_data();
    Object *char32_string_type_obj = obj_get_char32_string_type();
    types_map->insert(std::make_pair("char32_string", char32_string_type_obj));
    obj_inc_ref(char32_string_type_obj);//TODO add typemap

    map_type_obj = make_map_internal();
    map_type_obj.set_type(map_type_obj);
    types_map->insert(std::make_pair("map", map_type_obj.get_ptr()));
    obj_inc_ref(map_type_obj.get_ptr());
    obj_inc_ref(map_type_obj.get_ptr());// add extra ref
    types_obj.set_type(map_type_obj);
    root_obj.set_type(map_type_obj);

    array_type_obj = make_map();
    types_map->insert(std::make_pair("array", array_type_obj.get_ptr()));
    obj_inc_ref(array_type_obj.get_ptr());

    Object *destruct_map_obj = obj_create(sizeof(Closure_Data), _Alignof(Closure_Data));
    ((Closure_Data*)destruct_map_obj->data)->func = (void*)destruct_map;
    obj_add_attr(map_type_obj.get_ptr(), OBJ_DESTRUCT_FUNCTION_KEY, destruct_map_obj);

    Object *destruct_array_obj = obj_create(sizeof(Closure_Data), _Alignof(Closure_Data));
    ((Closure_Data*)destruct_array_obj->data)->func = (void*)destruct_array;
    obj_add_attr(array_type_obj.get_ptr(), OBJ_DESTRUCT_FUNCTION_KEY, destruct_map_obj);

    type_name_key = obj_attr_hash_string(U"@object.type_name");
    //type_base_key = obj_attr_hash_string(U"@object.type_base");
    //type_base_offset_key = obj_attr_hash_string(U"@object.base_offset");
    Easy_Object::type_register<IUnknown_Packer>(U"COM_IUnknown");
}

Easy_Object Easy_Object::make_map()
{
    Object *obj = make_map_internal();
    obj_set_type(obj, map_type_obj.get_ptr());
    return Easy_Object(obj);
}

Easy_Object Easy_Object::make_array()
{
    Object *obj = obj_create(sizeof(Vector_Data), alignof(Vector_Data));
    new (obj->data) Vector_Data();
    obj_set_type(obj, array_type_obj.get_ptr());
    return Easy_Object(obj);
}

Easy_Object Easy_Object::make_char32_string(const char32_t *str)
{
    Object *obj = obj_create_char32_string(str);
    obj_set_type(obj, obj_get_char32_string_type());
    return Easy_Object(obj);
}

Easy_Object Easy_Object::make_raw(void *data_orig, size_t copy_size, size_t align_req)
{
    Object *obj = obj_create(copy_size, align_req);
    memcpy(obj->data, data_orig, copy_size);
    return Easy_Object(obj);
}

Easy_Object Easy_Object::pack_COM_object(IUnknown *obj)
{
    Object *com_obj = obj_create(sizeof(IUnknown*), alignof(IUnknown*));
    *(IUnknown**)com_obj->data = obj;
    obj->AddRef();
    obj_set_type(com_obj, type_db[std::type_index(typeid(IUnknown_Packer))].get_ptr());
    return Easy_Object(com_obj);
}

bool Easy_Object::insert(const std::string &key, const Easy_Object &value)
{
    if (!obj) return false;
    assert(obj_get_type(obj) == map_type_obj.get_ptr());
    Map_Data *map = (Map_Data*)obj->data;
    auto res = map->insert(std::make_pair(key, value.get_ptr()));
    if (res.second) {
        obj_inc_ref(value.get_ptr());
    }
    return res.second;
}

Easy_Object Easy_Object::get(const std::string &key) const
{
    if (!obj) return Easy_Object();
    assert(obj_get_type(obj) == map_type_obj.get_ptr());
    Map_Data *map = (Map_Data*)obj->data;
    auto it = map->find(key);
    if (it == map->end()) return Easy_Object();
    return Easy_Object(it->second);
}

void Easy_Object::set(const std::string &key, const Easy_Object &value)
{
    if (!obj) return;
    assert(obj_get_type(obj) == map_type_obj.get_ptr());
    Map_Data *map = (Map_Data*)obj->data;
    auto it = map->find(key);
    if (it == map->end()) {
        insert(key, value);
        return;
    }
    obj_dec_ref(it->second);
    it->second = value.get_ptr();
    obj_inc_ref(value.get_ptr());
}

void Easy_Object::erase(const std::string &key)
{
    if (!obj) return;
    assert(obj_get_type(obj) == map_type_obj.get_ptr());
    Map_Data *map = (Map_Data*)obj->data;
    auto it = map->find(key);
    if (it == map->end()) return;
    obj_dec_ref(it->second);
    map->erase(it);
}

size_t Easy_Object::size() const
{
    Object* obj_type = obj_get_type(obj);
    if (obj_type == array_type_obj.get_ptr())
    {
        Vector_Data *vec = (Vector_Data*)obj->data;
        return vec->size();
    }
    else if (obj_get_type(obj) == map_type_obj.get_ptr())
    {
        return get_map_data()->size();
    }
    return 0;
}

Easy_Object Easy_Object::get(size_t index)const
{
    if (!obj) return Easy_Object();
    assert(obj_get_type(obj) == array_type_obj.get_ptr());
    Vector_Data *vec = (Vector_Data*)obj->data;
    if (index >= vec->size()) return Easy_Object();
    return Easy_Object((*vec)[index]);
}

void Easy_Object::set(size_t index, const Easy_Object &value)
{
    if (!obj) return;
    assert(obj_get_type(obj) == array_type_obj.get_ptr());
    Vector_Data *vec = (Vector_Data*)obj->data;
    if (index >= vec->size()) return;
    obj_dec_ref((*vec)[index]);
    (*vec)[index] = value.get_ptr();
    obj_inc_ref(value.get_ptr());
}

void Easy_Object::push_back(const Easy_Object &value)
{
    if (!obj) return;
    assert(obj_get_type(obj) == array_type_obj.get_ptr());
    Vector_Data *vec = (Vector_Data*)obj->data;
    vec->push_back(value.get_ptr());
    obj_inc_ref(value.get_ptr());
}

void Easy_Object::erase(size_t index)
{
    if (!obj) return;
    assert(obj_get_type(obj) == array_type_obj.get_ptr());
    Vector_Data *vec = (Vector_Data*)obj->data;
    if (index >= vec->size()) return;
    obj_dec_ref((*vec)[index]);
    vec->erase(vec->begin() + index);
}

void Easy_Object::set_type(const Easy_Object &type)
{
    obj_set_type(obj, type.get_ptr());
}

bool Easy_Object::has_COM_interface(const IID &iid)const
{
    if (!obj) return false;
    if (!is_COM_object()) return false;
    IUnknown *unk = *(IUnknown**)obj->data;
    IUnknown *unk2;
    if (unk->QueryInterface(iid, (void**)&unk2) == S_OK) {
        unk2->Release();
        return true;
    }
    return false;
}

