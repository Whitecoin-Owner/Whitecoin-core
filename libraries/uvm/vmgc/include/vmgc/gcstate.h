#pragma once

#include <list>
#include <vector>
#include <memory>
#include <exception>
#include <string>
#include <inttypes.h>
#include <cstddef>
#include <cstring>
#include "vmgc/exceptions.h"
#include "vmgc/gcobject.h"
#include <map>


namespace vmgc {

// 100MB
#define DEFAULT_MAX_GC_HEAP_SIZE 100*1024*1024
#define DEFAULT_MAX_GC_STRPOOL_SIZE 8*1024*1024

#define	DEFAULT_GC_STR_HASHLIMIT 5
#define DEFAULT_MAX_GC_SHORT_STRING_SIZE 32   //2^DEFAULT_GC_HASHLIMIT

#define DEFAULT_MAX_SMALL_BUFFER_SIZE 128  //8µÄ±¶Êý
#define DEFAULT_SMALL_BUFFER_VECTOR_SIZE (DEFAULT_MAX_SMALL_BUFFER_SIZE/8)     

	struct GcObject;

	struct GcBuffer {
		intptr_t pos;
		ptrdiff_t size;
		bool isGcObj;
		//bool isFree;
	};

    class GcState {
	private:
		ptrdiff_t _total_malloced_blocks_size;
		ptrdiff_t _used_size;
		ptrdiff_t _max_gc_size;
		std::shared_ptr<std::list<std::pair<intptr_t, ptrdiff_t> > > _malloced_blocks; // [ [start_ptr, size], ... ]
		std::shared_ptr<std::map<intptr_t,GcBuffer>> _malloced_gcbuffers; // 
		std::shared_ptr<std::list<std::pair<intptr_t, ptrdiff_t>>> _empty_small_buffers[DEFAULT_MAX_SMALL_BUFFER_SIZE]; //empty_size => [ [start_ptr, size], ... ]
		std::shared_ptr<std::list<std::pair<intptr_t, ptrdiff_t>>> _empty_big_buffers; //order list, from small to big
		std::shared_ptr<std::list<std::pair<intptr_t, intptr_t> > > _malloced_str_blocks; // [ [start_ptr, size], ... ]
		std::shared_ptr<std::map<unsigned int,std::pair<intptr_t, ptrdiff_t>>> _gc_strpool;
		std::pair<intptr_t, ptrdiff_t>  _empty_str_buffer; // [start_ptr, size]
		void insert_empty_buffer(std::pair<intptr_t, ptrdiff_t> &buf);

	public:
		// @throws vmgc::GcException
		GcState(ptrdiff_t max_gc_heap_size = DEFAULT_MAX_GC_HEAP_SIZE);
		virtual ~GcState();

		void* gc_malloc(size_t size, bool isGcObj=false);
		void gc_free(void* p);
		void gc_free_array(void* p, size_t count, size_t element_size);
		void* gc_realloc(void *p, size_t oldsize, size_t newsize);
		void* gc_malloc_vector(size_t count, size_t element_size);
		void* gc_grow_vector(void *p, size_t nelements, size_t* size, size_t element_size, size_t limit);
		ptrdiff_t usedsize() const;
		void gc_free_all();
		void* gc_intern_strpool(size_t sz, size_t strsize, const char* str, bool* isNewStr);

		template <typename T>
		T* gc_new_object()
		{
			size_t sz = sizeof(T);
			auto p = gc_malloc(sz,true);
			if (!p) {
				return nullptr;
			}
			GcObject* obj_p = static_cast<GcObject*>(p);
			new (obj_p)T();
			obj_p->tt = T::type;
			//_gc_objects->push_back((intptr_t) p - (intptr_t) _pstart);
			return static_cast<T*>(obj_p);
		}

		template <typename T>
		T* gc_new_object_vector(size_t count)
		{
			if (count <= 0)
				return nullptr;
			size_t sz = sizeof(T);
			std::vector<T*> malloced_items;
			for (size_t i = 0; i < count; i++) {
				auto p = gc_malloc(sz,true);
				if (!p) {
					for (const auto& item : malloced_items) {
						gc_free(item);
					}
					return nullptr;
				}
				GcObject* obj_p = static_cast<GcObject*>(p);
				new (obj_p)T();
				obj_p->tt = T::type;
				//_gc_objects->push_back((intptr_t)p - (intptr_t)_pstart);
				T* t_p = static_cast<T*>(obj_p);
				malloced_items.push_back(t_p);
			}
			return *(malloced_items.begin());
		}


		#define GC_MINSIZEARRAY 4
		template <typename T>
		T* gc_grow_object_vector(T* p, size_t nelements, size_t* size, size_t limit)
		{
			if (nelements + 1 <= *size)
				return p;
			size_t newsize;
			if ((*size) >= limit / 2) {  /* cannot double it? */
				if ((*size) >= limit)  /* cannot grow even a little? */
					throw GcException(std::string("too many objects in gc vector (limit is ") + std::to_string(limit) + ")");
				newsize = limit;  /* still have at least one free place */
			}
			else {
				newsize = (*size) * 2;
				if (newsize < GC_MINSIZEARRAY)
					newsize = GC_MINSIZEARRAY;  /* minimum size */
			}
			T* new_p = gc_new_object_vector<T>(newsize);
			if (!new_p) {
				return nullptr;
			}
			if (p) {
				for (size_t i = 0; i < (*size); i++) {
					*(new_p + i) = *(p + i);
				}
			}
			if (p || (*size) > 0) {
				gc_free_array(p, *size, sizeof(T));
			}
			*size = newsize;
			return new_p;
		}

		void fill_gc_string(GcObject* p, const char* str, size_t size);

		// short string into str pool, reused
		template <typename T>
		T* gc_intern_string(const char* str, size_t size)
		{
			T* ts = nullptr;
			bool isNewStr = true;
			if (size < DEFAULT_MAX_GC_SHORT_STRING_SIZE) { 
				size_t sz = sizeof(T);
				auto p = gc_intern_strpool(sz, size, str, &isNewStr);
				if (!p) {
					return nullptr;
				}
				GcObject* obj_p = static_cast<GcObject*>(p);
				if (isNewStr) {
					new (obj_p)T();
					fill_gc_string(obj_p, str, size);
					obj_p->tt = T::type;
				}
				ts = static_cast<T*>(obj_p);
			}
			else {
				ts = gc_new_object<T>();
				fill_gc_string(ts, str, size);
			}
			return ts;
		}


    };
}
