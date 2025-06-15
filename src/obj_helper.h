#pragma once

#include <atlbase.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <typeinfo>
#include <typeindex>
#include <comdef.h>
#include <atlcomcli.h>

#include "obj_tree.h"

typedef std::unordered_map<std::string, Object*> Map_Data;
typedef std::vector<Object*> Vector_Data;

class IUnknown_Packer
{
public:
    IUnknown_Packer(IUnknown* obj) : obj(obj) { obj->AddRef(); }
    ~IUnknown_Packer() { obj->Release(); }
    IUnknown* obj;
};



class Easy_Object {
public:
    Easy_Object() = default;
    ~Easy_Object()
    {
        if (obj) {
            obj_dec_ref(obj);
        }
    }
    Easy_Object(Object *obj) : obj(obj) {if (obj)obj_inc_ref(obj);}
    Easy_Object(const Easy_Object &other) : obj(other.obj) {if (obj)obj_inc_ref(obj);}
    Easy_Object(Easy_Object &&other)noexcept : obj(other.obj) {other.obj = nullptr;}
    Easy_Object &operator=(const Easy_Object &other)
    {
        if (obj == other.obj) {
            return *this;
        }
        if (obj) {
            obj_dec_ref(obj);
        }
        obj = other.obj;
        if (obj) {
            obj_inc_ref(obj);
        }
        return *this;
    }

    Object *get_ptr() const {return obj;}
    void *get_data_ptr() const {return obj->data;}
    bool is_null() const {return obj == nullptr;}

    static Easy_Object make_array();
    static Easy_Object make_map();
    static Easy_Object make_char32_string(const char32_t *str);
    static Easy_Object make_raw(void *data_orig, size_t copy_size, size_t align_req);
    static Easy_Object pack_COM_object(IUnknown *obj);
    static void TypeSystemInit();
    static inline Easy_Object get_root() { return root_obj; }

    bool insert(const std::string &key, const Easy_Object &value);
    Easy_Object get(const std::string &key) const;
    void set(const std::string &key, const Easy_Object &value);
    void erase(const std::string &key);

    Easy_Object operator[](const std::string &key) const {return get(key);}

    size_t size()const;
    Easy_Object get(size_t index)const;
    void set(size_t index, const Easy_Object &value);
    void push_back(const Easy_Object &value);
    void erase(size_t index);
    Map_Data *get_map_data() const {return (Map_Data*)obj->data;}

    template<typename T, typename Destructor = std::default_delete<T>>static Easy_Object type_register(char32_t const* name);

    bool is_COM_object()const {return obj && obj_get_type(obj) == type_db[std::type_index(typeid(IUnknown_Packer))].get_ptr();}
    bool has_COM_interface(const IID &iid)const;
    template<typename T> HRESULT get_COM_interface(CComPtr<T> &result)const;


private:
    void set_type(const Easy_Object &type);

    Object *obj = nullptr;
    static Easy_Object root_obj, types_obj, map_type_obj, array_type_obj;
    static std::unordered_map<std::type_index, Easy_Object> type_db;

    static Object_Key type_name_key;

};


template<typename T, typename Destructor>
inline Easy_Object Easy_Object::type_register(char32_t const* name)
{
    auto type_map = make_map();
    char* utf8_name = obj_str32_to_utf8(name);
    assert(utf8_name);
    Object* name_obj = obj_create_char32_string(name);
    Object_Destruct_Function func = [](struct s_Closure_Data* self, struct s_Object* obj)->void {
            if (obj->data_size)
            {
                Destructor()(static_cast<T*>(obj->data));
            }else {
                static_cast<T*>(obj->data)->~T();
            }
        };
    Object* destructor_obj = obj_create(sizeof(s_Closure_Data), alignof(Closure_Data));
    destructor_obj->data = (void*)func;
    obj_add_attr(type_map.get_ptr(), type_name_key, name_obj);
    obj_add_attr(type_map.get_ptr(), OBJ_DESTRUCT_FUNCTION_KEY, destructor_obj);
    types_obj.insert(utf8_name, type_map);
    type_db.insert(std::make_pair(std::type_index(typeid(T)), type_map));
    free(utf8_name);
    return type_map;
}

template<typename T>
inline HRESULT Easy_Object::get_COM_interface(CComPtr<T> &result)const
{
    if (!obj) return E_FAIL;
    if (!is_COM_object()) return E_FAIL;
    IUnknown *unk = *(IUnknown**)obj->data;
    return unk->QueryInterface(__uuidof(T), (void**)&result);
}
