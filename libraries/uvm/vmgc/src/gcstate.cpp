#include "vmgc/gcstate.h"
#include "vmgc/gcobject.h"
#include <algorithm>
#include "uvm/lstring.h"

namespace vmgc {
#define DEFAULT_GC_BLOCK_SIZE 1024*1024  

//#define MAX_GC_BLOCKS_SIZE 500*1024*1024 

	GcState::GcState(ptrdiff_t max_gc_size) {
		this->_malloced_blocks = std::make_shared<std::list<std::pair<intptr_t, intptr_t> > >();
		_total_malloced_blocks_size = 0;
		_used_size = 0;
		_max_gc_size = max_gc_size;

		this->_malloced_gcbuffers = std::make_shared<std::map<intptr_t,GcBuffer>>();

		for (int i = 0; i < DEFAULT_SMALL_BUFFER_VECTOR_SIZE; i++) {
			_empty_small_buffers[i] = std::make_shared<std::list<std::pair<intptr_t, ptrdiff_t>>>();
		}

		this->_empty_big_buffers = std::make_shared<std::list<std::pair<intptr_t, ptrdiff_t> >>();

		//////////////////
		this->_malloced_str_blocks = std::make_shared<std::list<std::pair<intptr_t, intptr_t>>>();
		this->_gc_strpool = std::make_shared<std::map<unsigned int, std::pair<intptr_t, ptrdiff_t>>>();

		_empty_str_buffer.first = 0;
		_empty_str_buffer.second = 0;
	}

	void GcState::gc_free_all()
	{
		for (const auto& item : *_malloced_gcbuffers) {
			if (item.second.isGcObj) {
				auto gc_obj = (GcObject*)item.first;
				gc_obj->~GcObject();
			}
		}
		_malloced_gcbuffers->clear();

		for (int i = 0; i < DEFAULT_SMALL_BUFFER_VECTOR_SIZE; i++) {
			_empty_small_buffers[i]->clear();
		}

		_empty_big_buffers->clear();

		////////////////////////////////////
		for (const auto& item : *_gc_strpool) {
			auto gc_obj = (GcObject*)item.second.first;
			gc_obj->~GcObject();
		}
		_gc_strpool->clear();

		_empty_str_buffer.first = 0;
		_empty_str_buffer.second = 0;

		_used_size = 0;
		_total_malloced_blocks_size = 0;
	}


	GcState::~GcState() {
		gc_free_all();

		for (const auto& item : (*_malloced_blocks)) {
			free((void*)(item.first));
		}
		_malloced_blocks->clear();

		for (const auto& item : (*_malloced_str_blocks)) {
			free((void*)(item.first));
		}
		_malloced_str_blocks->clear();
	}

	static size_t align8(size_t s) {
		if ((s & 0x7) == 0)
			return s;
		return ((s >> 3) + 1) << 3;
	}

	void GcState::insert_empty_buffer(std::pair<intptr_t, ptrdiff_t> &buf) {
		auto size = buf.second;
		if (size > 0) {
			if ((size > DEFAULT_MAX_SMALL_BUFFER_SIZE) || (size % 8) != 0) {
				for (auto iter = _empty_big_buffers->begin(); iter != _empty_big_buffers->end(); iter++) {
					if (size <= iter->second) {  //inserted
						_empty_big_buffers->emplace(iter, buf);
						return;
					}
				}
				_empty_big_buffers->push_back(buf);
			}
			else {
				_empty_small_buffers[(size / 8) - 1]->push_back(buf);
			}
		}
	}

	void* GcState::gc_malloc(size_t size, bool isGcObj) {
		if (size <= 0)
			return nullptr;
		size = align8(size);

		GcBuffer b;
		b.isGcObj = isGcObj;
		b.pos = 0;
		b.size = size;
		_used_size += size;

		//find _empty_small_buffers 
		if (size <= DEFAULT_MAX_SMALL_BUFFER_SIZE) {
			auto pbuffers = _empty_small_buffers[size/8 - 1];
			if (!pbuffers->empty()) {  //finded
				const auto& buf = pbuffers->front();
				
				b.pos = buf.first;
				(*_malloced_gcbuffers)[buf.first] = b;
				pbuffers->pop_front();
				return (void*)b.pos;
			}
		}
		
		//find _empty_big_buffers  	
		auto len = _empty_big_buffers->size();
		if (len > 0) {
			const auto &frontBuf = _empty_big_buffers->front();
			if (size <= _empty_big_buffers->front().second) {
				auto bufpos = frontBuf.first;
				auto bufsz = frontBuf.second;
				b.pos = bufpos;

				_empty_big_buffers->pop_front();

				if (size < bufsz) {
					std::pair<intptr_t, ptrdiff_t> eb;
					eb.first = bufpos + size;
					eb.second = bufsz - size;
					insert_empty_buffer(eb);
				}

				//_malloced_gcbuffers->insert(std::pair<intptr_t, GcBuffer>(frontBuf.first, b));
				(*_malloced_gcbuffers)[bufpos] = b;
				return (void*)b.pos;
			}

			const auto &backBuf = _empty_big_buffers->back();
			if (size <= backBuf.second) {
				auto bufpos = backBuf.first;
				auto bufsz = backBuf.second;
				b.pos = bufpos;

				_empty_big_buffers->pop_back();

				if (size < bufsz) {
					std::pair<intptr_t, ptrdiff_t> eb;
					eb.first = bufpos + size;
					eb.second = bufsz - size;
					insert_empty_buffer(eb);
				}

				(*_malloced_gcbuffers)[bufpos] = b;
				return (void*)b.pos;
			}
		}

		//malloc new block
		//check max size
		auto mallocSize = DEFAULT_GC_BLOCK_SIZE;
		if (size > DEFAULT_GC_BLOCK_SIZE) {
			mallocSize = size;
		}
		if (mallocSize + _total_malloced_blocks_size > _max_gc_size) {
			_used_size -= size;
			return nullptr;
		}
		auto p = malloc(mallocSize);
		if (!p) {
			_used_size -= size;
			return nullptr;
		}
		_total_malloced_blocks_size += mallocSize;

		std::pair<intptr_t, intptr_t> block;
		block.first = (intptr_t)p;
		block.second = mallocSize;
		_malloced_blocks->push_back(block);

		b.pos = (intptr_t)p;
		(*_malloced_gcbuffers)[b.pos] = b;

		if (size < mallocSize) {
			//add empty buff
			std::pair<intptr_t, ptrdiff_t> eb;
			eb.first = (intptr_t)p + size;
			eb.second = mallocSize - size;
			insert_empty_buffer(eb);
		}
		return (void*)b.pos;
	}

	void GcState::gc_free(void* p) {
		if (nullptr == p)
			return;
		if (_malloced_gcbuffers->find((intptr_t)p) != _malloced_gcbuffers->end()) {
			auto buf = (*_malloced_gcbuffers)[(intptr_t)p];
			if (buf.isGcObj) {
				auto gc_obj = (GcObject*)p;
				auto objtt = gc_obj->tt;
				auto ntt = novariant(objtt);
				gc_obj->~GcObject();
			}
			_malloced_gcbuffers->erase((intptr_t)p);  

			std::pair<intptr_t, ptrdiff_t> eb;
			eb.first = buf.pos;
			eb.second = buf.size;
			insert_empty_buffer(eb);
			_used_size -= buf.size;
		}
	}

	void GcState::gc_free_array(void* p, size_t count, size_t size)
	{
		if (!p || count <= 0)
			return;
		for (size_t i = 0; i < count; i++) {
			gc_free((void*)((intptr_t)p + i * size));
		}
	}

	void* GcState::gc_realloc(void *p, size_t oldsz, size_t newsz) {
		if ((oldsz < 0) || (newsz < 0))return nullptr;

		if (newsz == 0) {
			//do some free
			if (nullptr != p) {
				gc_free(p);
			}
			return nullptr;
		}

		void *newp = nullptr;
		if (p == nullptr)   // alloc new
		{
			newp = gc_malloc(newsz);
			return newp;
		}
		else
		{
			if (newsz <= oldsz) {
				//no op
				return p;
			}
			newp = gc_malloc(newsz);
			if (newp == nullptr)return nullptr;
			//copy data
			memcpy(newp, p, oldsz * sizeof(ptrdiff_t));
			//free old 
			gc_free(p);
			return newp;
		}
	}

	void* GcState::gc_malloc_vector(size_t count, size_t size)
	{
		if (count <= 0)
			return nullptr;
		return gc_malloc(count * size);
	}

	void* GcState::gc_grow_vector(void *p, size_t nelements, size_t* size, size_t element_size, size_t limit)
	{
		if (nelements + 1 <= *size)
			return p;
		size_t newsize;
		if ((*size) >= limit / 2) {  // cannot double it? 
			if ((*size) >= limit)  // cannot grow even a little? 
				throw GcException(std::string("too many objects in gc vector (limit is ") + std::to_string(limit) + ")");
			newsize = limit;  // still have at least one free place 
		}
		else {
			newsize = (*size) * 2;
			if (newsize < GC_MINSIZEARRAY)
				newsize = GC_MINSIZEARRAY;  // minimum size 
		}
		auto new_p = gc_malloc_vector(newsize, element_size);
		if (!new_p) {
			return nullptr;
		}
		if (p) {
			memcpy(new_p, p, (*size) * element_size);
		}
		if (p || (*size) > 0) {
			gc_free_array(p, *size, element_size);
		}
		*size = newsize;
		return new_p;
	}

	ptrdiff_t GcState::usedsize() const {
		return _used_size;
	}

	void GcState::fill_gc_string(GcObject* p, const char* str, size_t size) {
		auto sp = static_cast<uvm_types::GcString*>(p);
		sp->value = std::string(str,size);
		sp->tt = sp->tt_;
	}

	//支持长度小于2^5的hash，大于等于此数字会较大概率发生碰撞
	static unsigned int gc_str_hash(const char *str, size_t l, unsigned int seed) {
		unsigned int h = seed ^ lua_cast(unsigned int, l);
		size_t step = (l >> DEFAULT_GC_STR_HASHLIMIT) + 1;
		for (; l >= step; l -= step)
			h ^= ((h << 5) + (h >> 2) + cast_byte(str[l - 1]));
		return h;
	}

	void* GcState::gc_intern_strpool(size_t sz, size_t strsize, const char* str, bool* isNewStr) {
		void* p = nullptr;
		unsigned int seed = 1;
		unsigned int h = gc_str_hash(str, strsize, seed);

		auto it = _gc_strpool->find(h);
		if (it == _gc_strpool->end()) {
			*isNewStr = true;
		}
		else {
			*isNewStr = false;
			p = (void *)((*_gc_strpool)[h].first);
			uvm_types::GcString* gcstr = static_cast<uvm_types::GcString*>(p);
			if (strncmp(gcstr->value.c_str(), str, strsize) != 0) { //碰撞
				//new it 
				*isNewStr = true;
			}
		}

		if (*isNewStr) {
			//add 
			size_t align8sz = align8(sz);

			if (align8sz <= _empty_str_buffer.second) {
				(*_gc_strpool)[h] = std::pair<intptr_t, ptrdiff_t>(_empty_str_buffer.first, align8sz);
				p = (void*) _empty_str_buffer.first;

				_empty_str_buffer.first = _empty_str_buffer.first + align8sz;
				_empty_str_buffer.second = _empty_str_buffer.second - align8sz;

			}
			else {
				//malloc new block
				if (DEFAULT_GC_BLOCK_SIZE + _total_malloced_blocks_size > _max_gc_size) {
					return nullptr;
				}
				p = malloc(DEFAULT_GC_BLOCK_SIZE);
				if (!p) {
					return nullptr;
				}
				_total_malloced_blocks_size += DEFAULT_GC_BLOCK_SIZE;

				std::pair<intptr_t, intptr_t> block;
				block.first = (intptr_t)p;
				block.second = DEFAULT_GC_BLOCK_SIZE;
				_malloced_str_blocks->push_back(block);

				(*_gc_strpool)[h] = std::pair<intptr_t, ptrdiff_t>((intptr_t)p, align8sz);

				if (align8sz < DEFAULT_GC_BLOCK_SIZE) {
					_empty_str_buffer.first = (intptr_t)p + align8sz;
					_empty_str_buffer.second = DEFAULT_GC_BLOCK_SIZE - align8sz;
				}
			}
			_used_size += align8sz;
		}
		
		return p;
	}

}
