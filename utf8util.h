#pragma once
#include <stddef.h>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>

//TODO
//normalize non-UTF characters:
//    https://github.com/voku/portable-utf8/blob/master/src/voku/helper/UTF8.php#1160


namespace utf8util
{

	//set default memory allocation functions
	struct allocator
	{
		static void* allocate(size_t size) { return ::malloc(size); };
		static void free(void* ptr) { return ::free(ptr); };
	};

	namespace list
	{

		template<typename T>
		class Node
		{
		public:
			typedef T value_type;
		private:
			Node*      _next;
			Node*      _prev;
			value_type _value;
		};

		template<typename T>
		class List
		{
		public:
			typedef T value_type;
			typedef T& ref_value_type;
			typedef const T const_value_type;
			typedef const T& const_ref_value_type;
			typedef Node<T>    node_type;
			typedef node_type* node_pointer;

			List() :_head(0), _tail(0) {};

			//append to end
			void* push(const_ref_value_type item)
			{
				node_pointer node = new node_type;
				if (!node) return 0;//on error
				node->_next = 0;
				node->_prev = _tail;
				memcpy(&node->_value, &item, sizeof(value_type));
				if (_head == 0)
				{
					_head = node;
				}
				else
				{
					_tail->_next = node;
				}
				_tail = node;
				return node;
			};
			void remove_item(node_pointer p)
			{
				if (p == _head)
				{
					_head = _head->_next;
					_head->_prev = 0;
				}
				else if (p == _tail)
				{
					_tail = _tail->_prev;
					_tail->_next = 0;
				}
				else
				{
					p->_prev->_next = p->_next;
					p->_next->_prev = p->_prev;
				}
				delete p;
			}



			//get
			void* pop(ref_value_type item)
			{
				node_pointer node = new node_type;
				if (!node) return 0;//on error
				node->_next = 0;
				node->_prev = _tail;
				memcpy(&node->_value, &item, sizeof(value_type));
				if (_head == 0)
				{
					_head = node;
				}
				else
				{
					_tail->_next = node;
				}
				_tail = node;
				return node;
			}

		private:
			node_pointer _head, _tail;
		};
	}





	namespace memory
	{

		//All data blocks inside pages have data_alignment = sizeof(void*) = 4 bytes (x86-32) or 8 bytes (x86-64).
		//data_size_bits: 12..15, number bits of data_header {size} and {prev_size} fields.
		//max_page_size = 2^14...2^17 bytes (x86-32) or 2^15...2^18 bytes (x86-64).
		//For data sizes > max_page_size, allocated new page without unused block.
		//All pages have end_header at end of page.
		//page structure:
		//        |page header    |unused|data header 0 | data block 0 |data header 1 | data block 1 | ... |data header n | unused block   |end header |
		//x86-32: |4+4+2=10 bytes |  2   | 4 bytes      |              |  4 bytes     |              | ... | 4 bytes      |                | 4 bytes   |
		//x86-64: |8+8+2=18 bytes |  2   | 4 bytes      |              |  4 bytes     |              | ... | 4 bytes      |                | 4 bytes   |
		//        |-------------------------------------|-----------------------------|-----------------------------------|----------------------------|
		//   ptr: |page_ptr                             |ptr0                         |ptr1                               |page_ptr +                  |page_ptr +
		//        |                                     |                             |                                   | last_offset*data_alignment | default_page_size
		//        --------------------------------------------------------------------------------------------------------------------------------------




		//data_size_bits: 12..15, number bits of size and prev_size data_header fields
#define data_size_bits       12U
#define data_size_mask       ((1U << data_size_bits) - 1U)
#define data_alignment       sizeof(void*)
#define max_page_size	     (data_alignment << data_size_bits)
#define default_page_size    max_page_size

#define page_header_bytes    (sizeof(void*)*2 + 4)
#define data_header_bytes    sizeof(uint32_t)
#define ptr0_offset          ((page_header_bytes + data_header_bytes + data_alignment - 1) & ~(data_alignment - 1))
		
		struct page_header
		{
			void* next;
			void* prev;
			uint16_t last_offset;//offset to the last unused block (in data_alignment units)
			uint8_t u[2];//unused
		};
		//struct data_header{ uint32_t d; };
		struct data_header_fields
		{
			uint8_t  busy_bit;  //1 - data block is busy, 0 - data block is unused block
			uint16_t size;      //size of data block (in data_alignment units), low data_size_bits
			uint16_t prev_size; //size of previous data block (in data_alignment units), low data_size_bits
		};
		class page_manager
		{
		public:
			page_manager() :_first(0), _last(0) {};
			inline void packed_to_data_header_fields(const void* ptr, data_header_fields* pdh)
			{
				uint32_t c = *reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(ptr) - data_header_bytes);
				pdh->size = c & data_size_mask;
				pdh->prev_size = (c >> data_size_bits) & data_size_mask;
				pdh->busy_bit = (c >> (data_size_bits * 2 + 0)) & 1;
			};
			inline void data_header_fields_to_packed(const data_header_fields* pdh, void* ptr)
			{
				*reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(ptr) - data_header_bytes) =
					(pdh->busy_bit & 1) << (data_size_bits * 2 + 0) |
					(pdh->prev_size & data_size_mask) << data_size_bits |
					(pdh->size & data_size_mask);
			};
			void* init()
			{
				if (_first) destroy();
				if (page_append(default_page_size) == 0)return 0;
				//init first data_header
				data_header_fields dhf;
				dhf.size = (default_page_size - ptr0_offset) / data_alignment;
				dhf.prev_size = 0;
				dhf.busy_bit = 0;
				data_header_fields_to_packed(&dhf, static_cast<uint8_t*>(_first) + ptr0_offset);
				//init end_page data_header
				dhf.prev_size = dhf.size;
				dhf.size = 0;
				dhf.busy_bit = 0;
				data_header_fields_to_packed(&dhf, static_cast<uint8_t*>(_first) + default_page_size);
				static_cast<page_header*>(_last)->last_offset = ptr0_offset / data_alignment;
				return _first;
			};
			void destroy()
			{
				while (_first)
				{
					void* p = _first;
					_first = static_cast<page_header*>(_first)->next;
					allocator::free(p);
				}
				_first = _last = 0;
			};
			void* page_append(size_t size)
			{
				if (size == 0)return 0;
				page_header* p = static_cast<page_header*>(allocator::allocate(size));
				if (p == 0)return 0;
				p->next = 0;
				p->prev = 0;
				p->last_offset = 0;
				if (_first == 0)
				{
					_first = p;
				}
				else
				{
					p->prev = _last;
					static_cast<page_header*>(_last)->next = p;
				}

				_last = p;
				return p;
			}
			void page_remove(void* page_ptr)
			{
				if (page_ptr == 0)return;
				page_header* p = static_cast<page_header*>(page_ptr);
				if (p->prev != 0) static_cast<page_header*>(p->prev)->next = p->next;
				if (p->next != 0) static_cast<page_header*>(p->next)->prev = p->prev;
				if (p->prev == 0)_first = p->next;
				if (p->next == 0)_last = p->prev;
				allocator::free(page_ptr);
			};
			void* allocate(size_t size)
			{
				if (size == 0)return 0;

				data_header_fields dhf;
				page_header* last_page;
				void* ptr;


				size_t size_aligned = (size + data_header_bytes + data_alignment - 1) & ~(data_alignment - 1);

				//проверим: если size > default_page_size, то выделим новую страницу с таким size
				if (size_aligned > default_page_size - ptr0_offset)
				{
					//append new page without unused blocks
					if (page_append(size_aligned + ptr0_offset) == 0)return 0;
					last_page = static_cast<page_header*>(_last);
					//for page without unused blocks: last_offset=0, size=prev_size=0, busy_bit=1
					last_page->last_offset = 0; 
					dhf.size = 0;
					dhf.prev_size = 0;
					dhf.busy_bit = 1;
					ptr = static_cast<uint8_t*>(_last) + ptr0_offset;
					data_header_fields_to_packed(&dhf, ptr);
					return ptr;
				}

				//skip pages without unused blocks
				last_page = static_cast<page_header*>(_last);
				while (last_page->last_offset == 0)
					last_page = static_cast<page_header*>(last_page->prev);

				//скорректируем last_offset, если слева от него есть unused block
				while (1)
				{
					packed_to_data_header_fields(reinterpret_cast<uint8_t*>(last_page) + last_page->last_offset*data_alignment, &dhf);
					if (dhf.busy_bit == 1 || dhf.prev_size == 0)break;
					last_page->last_offset -= dhf.prev_size;
				}
				if (dhf.busy_bit == 1)
				{
					last_page->last_offset += dhf.size;
					packed_to_data_header_fields(reinterpret_cast<uint8_t*>(last_page) + last_page->last_offset*data_alignment, &dhf);
				}

				//get unused block size
				size_t unused = default_page_size - last_page->last_offset*data_alignment;
				assert(unused == dhf.size*data_alignment);
				if (size_aligned > unused)
				{
					//create new page
					if (page_append(default_page_size) == 0)return 0;
					last_page = static_cast<page_header*>(_last);
					last_page->last_offset = ptr0_offset / data_alignment;
					//set new unused block
					dhf.size = (default_page_size - ptr0_offset)/data_alignment;
					dhf.prev_size = 0;
					dhf.busy_bit = 0;
				}

				//allocate new block in unused block
				uint16_t prev_size = dhf.size;
				dhf.size = static_cast<uint16_t>(size_aligned / data_alignment);
				dhf.busy_bit = 1;
				ptr = reinterpret_cast<uint8_t*>(last_page) + last_page->last_offset*data_alignment;
				data_header_fields_to_packed(&dhf, ptr);
				//set new unused block
				last_page->last_offset += dhf.size;
				dhf.prev_size = dhf.size;
				dhf.size = prev_size - dhf.size;
				dhf.busy_bit = 0;
				data_header_fields_to_packed(&dhf, reinterpret_cast<uint8_t*>(last_page) + last_page->last_offset*data_alignment);
				//set end_page header
				assert((last_page->last_offset + dhf.size)*data_alignment == default_page_size);
				dhf.prev_size = dhf.size;
				dhf.size = 0;
				dhf.busy_bit = 0;
				data_header_fields_to_packed(&dhf, reinterpret_cast<uint8_t*>(last_page) + default_page_size);
				return ptr;
			};
			void deallocate(void* ptr)
			{
				assert(ptr != 0);
				if (ptr == 0)return;

				
				//set busy_bit = 0
				data_header_fields dhf, *pdhf = &dhf;
				packed_to_data_header_fields(ptr, pdhf);
				assert(pdhf->busy_bit == 1);//can free only block with busy_bit == 1
				pdhf->busy_bit = 0;
				data_header_fields_to_packed(pdhf, ptr);

				//free page without unused block
				if (pdhf->size == 0)
				{
					assert(pdhf->prev_size == 0);
					page_remove(reinterpret_cast<page_header*>(static_cast<uint8_t*>(ptr) - ptr0_offset));
					return;
				}

				//!!! нам неизвестно положение page_header{ last_offset },
				//поэтому объединять соседние освобожденные блоки будем,
				//потому что в заголовке, на который указывает last_offset,
				//после объединения сохраниться prev_size и
				//при первой же возможности можно легко исправить last_offset.
				data_header_fields dhf0, *pdhf0 = &dhf0, *pdhf_temp;
				uint8_t *p = static_cast<uint8_t*>(ptr);
				//объединим пространства слева
				// здесь просто смещаем указатель влево до тех пор,
				// пока встречаются освобождённые блоки слева
				while (pdhf->prev_size > 0)
				{
					packed_to_data_header_fields(p - pdhf->prev_size*data_alignment, pdhf0);
					if (pdhf0->busy_bit == 1) break;
					p -= pdhf->prev_size*data_alignment;
					pdhf_temp = pdhf; pdhf = pdhf0; pdhf0 = pdhf_temp;
				}

				//объединим пространства справа
				while (1)
				{
					packed_to_data_header_fields(p + pdhf->size*data_alignment, pdhf0);
					//обновим prev_size для возможности быстрой коррекции last_offset
					pdhf0->prev_size = pdhf->size;
					data_header_fields_to_packed(pdhf0, p + pdhf->size*data_alignment);
					if (pdhf0->busy_bit == 1 || pdhf0->size == 0) break;
					pdhf->size += pdhf0->size;
				}
				data_header_fields_to_packed(pdhf, p);


				//проверим, если вся страница освобождена,
				//и если она не _first, то удалим ее
				if (pdhf->size == (default_page_size - ptr0_offset) / data_alignment
					&& (static_cast<uint8_t*>(p)- ptr0_offset) != _first)
				{
					page_remove(p - ptr0_offset);
				}
			};

		private:
			void* _first;
			void* _last;
		};
	}//end namespace memory




	// Unicode utilities

	//Main string format: utf8
	//Proxy char size: uint32_t
	//
	//utf8->utf16, utf8->utf32
	//utf16->utf8, utf16->utf32
	//utf32->utf8, utf32->utf16
	
	typedef uint32_t codepoint_t;
	#define UTF_UNICODE_CP_REPLACEMENT_CHARACTER 0xFFFDUL /* http://en.wikipedia.org/wiki/Replacement_character#Replacement_character */

	//counting the number of unicode codepoints in the utf8 string
	void utf8length(uint8_t* data, size_t size, size_t* result)
	{
		uint8_t c;
		size_t count = 0;

		while (size > 0)
		{
			c = *data;
			if (c == 0) break;
			if (c < 0x80)//0x00..0x7F
			{
				data += 1; size -= 1;
			}
			else if (c >= 0xC2 && c < 0xE0)//0xC2..0xDF, first surrogate
			{
				data += 2; size -= 2;
			}
			else if (c < 0xF0)//0xE0..0xEF, first surrogate
			{
				data += 3; size -= 3;
			}
			else if (c <= 0xF4)//0xF0..0xF4, first surrogate
			{
				data += 4; size -= 4;
			}
			else
			{
				//not a legal initial utf8 coded byte, skip it
				data++; size--;
			}
			count++;
		}
		*result = count;
	}
	//counting the number of unicode codepoints in the utf16 string
	void utf16length(uint16_t* data, size_t size, size_t* result)
	{
		uint16_t c;
		size_t count = 0;

		while (size > 0)
		{
			c = *data;
			if (c == 0) break;
			else if (c < 0xD800 || c >= 0xE000)//0x0000..0xD7FF, 0xE000..0xFFFF
			{
				data += 1; size -= 1;
			}
			else if (c >= 0xD800 && c < 0xDC00)//0xD800..0xDBFF, first surrogate
			{
				data += 2; size -= 2;
			}
			else
			{
				//not a legal utf16 coded word, skip it
				data++; size--;
			}
			count++;
		}
		*result = count;
	}
	//counting the number of unicode codepoints in the utf32 string
	void utf32length(uint32_t* data, size_t size, size_t* result)
	{
		if(size == 0)
			while (*data++ > 0) size++;
		*result = size;
	}

	//source: https://github.com/sheredom/utf8.h/blob/master/utf8.h#250
	//encode unicode codepoint to utf8 sequence
	void* utf8_encode(codepoint_t codepoint, uint8_t* result)
	{
		if (codepoint < 0x80UL)
		{
			*result++ = static_cast<uint8_t>(codepoint);
		}
		else if (codepoint < 0x800UL)
		{
			*result++ = static_cast<uint8_t>(0xc0 | (codepoint >> 6));
			*result++ = static_cast<uint8_t>(0x80 | (codepoint & 0x3f));
		}
		else if (codepoint < 0x10000UL)
		{
			*result++ = static_cast<uint8_t>(0xe0 | (codepoint >> 12));
			*result++ = static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3f));
			*result++ = static_cast<uint8_t>(0x80 | (codepoint & 0x3f));
		}
		else if(codepoint < 0x110000UL)
		{
			*result++ = static_cast<uint8_t>(0xf0 | (codepoint >> 18));
			*result++ = static_cast<uint8_t>(0x80 | ((codepoint >> 12) & 0x3f));
			*result++ = static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3f));
			*result++ = static_cast<uint8_t>(0x80 | (codepoint & 0x3f));
		}
		else
		{
			//on error emit replacement codepoint U+FFFD, encoded as [ef bf bd]
			*result++ = 0xEF;
			*result++ = 0xBF;
			*result++ = 0xBD;
		}
		return result;
	}

	//encode unicode codepoint to utf16 sequence
	void* utf16_encode(codepoint_t codepoint, uint16_t* result)
	{
		if (codepoint < 0xD800UL)
		{
			*result++ = static_cast<uint16_t>(codepoint);
		}
		else if (codepoint < 0xE000UL)
		{
			//codepoint in ranges U+D800..U+DBFF and U+DC00..U+DFFF is reserved for utf16
			//replace to codepoint U+FFFD
			*result++ = 0xFFFD;
		}
		else if (codepoint < 0x10000UL)
		{
			*result++ = static_cast<uint16_t>(codepoint);
		}
		else if(codepoint < 0x110000UL)
		{
			//U+10000..U+10FFFF
			codepoint -= 0x10000UL;
			*result++ = static_cast<uint16_t>(0xD800 | (codepoint >> 10));   //D800..DBFF
			*result++ = static_cast<uint16_t>(0xDC00 | (codepoint & 0x3FF)); //DC00..DFFF
		}
		else
		{
			//replace to codepoint U+FFFD
			*result++ = 0xFFFD;
		}
		return result;
	}

	//encode unicode codepoint to utf32 sequence
	void* utf32_encode(codepoint_t codepoint, uint32_t* result)
	{
		*result++ = codepoint;
		return result;
	}




	//decode utf8 sequence to unicode codepoint
	/*
	void* utf8_decode(const uint8_t* data, size_t size, codepoint_t* result)
	{
		while (size)
		{
			uint8_t lead = *data;

			// 0xxxxxxx -> U+0000..U+007F
			if (lead < 0x80)
			{
				result = Traits::low(result, lead);
				data += 1;
				size -= 1;

				// process aligned single-byte (ascii) blocks
				if ((reinterpret_cast<uintptr_t>(data) & 3) == 0)
				{
					// round-trip through void* to silence 'cast increases required alignment of target type' warnings
					while (size >= 4 && (*static_cast<const uint32_t*>(static_cast<const void*>(data)) & 0x80808080) == 0)
					{
						result = Traits::low(result, data[0]);
						result = Traits::low(result, data[1]);
						result = Traits::low(result, data[2]);
						result = Traits::low(result, data[3]);
						data += 4;
						size -= 4;
					}
				}
			}
			// 110xxxxx -> U+0080..U+07FF
			else if (static_cast<unsigned int>(lead - 0xC0) < 0x20 && size >= 2 && (data[1] & 0xc0) == 0x80)
			{
				result = Traits::low(result, ((lead & ~0xC0) << 6) | (data[1] & utf8_byte_mask));
				data += 2;
				size -= 2;
			}
			// 1110xxxx -> U+0800-U+FFFF
			else if (static_cast<unsigned int>(lead - 0xE0) < 0x10 && size >= 3 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80)
			{
				result = Traits::low(result, ((lead & ~0xE0) << 12) | ((data[1] & utf8_byte_mask) << 6) | (data[2] & utf8_byte_mask));
				data += 3;
				size -= 3;
			}
			// 11110xxx -> U+10000..U+10FFFF
			else if (static_cast<unsigned int>(lead - 0xF0) < 0x08 && size >= 4 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80 && (data[3] & 0xc0) == 0x80)
			{
				result = Traits::high(result, ((lead & ~0xF0) << 18) | ((data[1] & utf8_byte_mask) << 12) | ((data[2] & utf8_byte_mask) << 6) | (data[3] & utf8_byte_mask));
				data += 4;
				size -= 4;
			}
			// 10xxxxxx or 11111xxx -> invalid
			else
			{
				data += 1;
				size -= 1;
			}
		}

		return result;
	}
	*/
















	inline uint16_t endian_swap(uint16_t value)
	{
		return static_cast<uint16_t>(((value & 0xff) << 8) | (value >> 8));
	}

	inline uint32_t endian_swap(uint32_t value)
	{
		return ((value & 0xff) << 24) | ((value & 0xff00) << 8) | ((value & 0xff0000) >> 8) | (value >> 24);
	}

	struct utf8_counter
	{
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t ch)
		{
			// U+0000..U+007F
			if (ch < 0x80) return result + 1;
			// U+0080..U+07FF
			else if (ch < 0x800) return result + 2;
			// U+0800..U+FFFF
			else return result + 3;
		}

		static value_type high(value_type result, uint32_t)
		{
			// U+10000..U+10FFFF
			return result + 4;
		}
	};

	struct utf8_writer
	{
		typedef uint8_t* value_type;

		static value_type low(value_type result, uint32_t ch)
		{
			// U+0000..U+007F
			if (ch < 0x80)
			{
				*result = static_cast<uint8_t>(ch);
				return result + 1;
			}
			// U+0080..U+07FF
			else if (ch < 0x800)
			{
				result[0] = static_cast<uint8_t>(0xC0 | (ch >> 6));
				result[1] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
				return result + 2;
			}
			// U+0800..U+FFFF
			else
			{
				result[0] = static_cast<uint8_t>(0xE0 | (ch >> 12));
				result[1] = static_cast<uint8_t>(0x80 | ((ch >> 6) & 0x3F));
				result[2] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
				return result + 3;
			}
		}

		static value_type high(value_type result, uint32_t ch)
		{
			// U+10000..U+10FFFF
			result[0] = static_cast<uint8_t>(0xF0 | (ch >> 18));
			result[1] = static_cast<uint8_t>(0x80 | ((ch >> 12) & 0x3F));
			result[2] = static_cast<uint8_t>(0x80 | ((ch >> 6) & 0x3F));
			result[3] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
			return result + 4;
		}

		static value_type any(value_type result, uint32_t ch)
		{
			return (ch < 0x10000) ? low(result, ch) : high(result, ch);
		}
	};

	struct utf16_counter
	{
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t)
		{
			return result + 1;
		}

		static value_type high(value_type result, uint32_t)
		{
			return result + 2;
		}
	};

	struct utf16_writer
	{
		typedef uint16_t* value_type;

		static value_type low(value_type result, uint32_t ch)
		{
			*result = static_cast<uint16_t>(ch);

			return result + 1;
		}

		static value_type high(value_type result, uint32_t ch)
		{
			uint32_t msh = static_cast<uint32_t>(ch - 0x10000) >> 10;
			uint32_t lsh = static_cast<uint32_t>(ch - 0x10000) & 0x3ff;

			result[0] = static_cast<uint16_t>(0xD800 + msh);
			result[1] = static_cast<uint16_t>(0xDC00 + lsh);

			return result + 2;
		}

		static value_type any(value_type result, uint32_t ch)
		{
			return (ch < 0x10000) ? low(result, ch) : high(result, ch);
		}
	};

	struct utf32_counter
	{
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t)
		{
			return result + 1;
		}

		static value_type high(value_type result, uint32_t)
		{
			return result + 1;
		}
	};

	struct utf32_writer
	{
		typedef uint32_t* value_type;

		static value_type low(value_type result, uint32_t ch)
		{
			*result = ch;

			return result + 1;
		}

		static value_type high(value_type result, uint32_t ch)
		{
			*result = ch;

			return result + 1;
		}

		static value_type any(value_type result, uint32_t ch)
		{
			*result = ch;

			return result + 1;
		}
	};

	struct latin1_writer
	{
		typedef uint8_t* value_type;

		static value_type low(value_type result, uint32_t ch)
		{
			*result = static_cast<uint8_t>(ch > 255 ? '?' : ch);

			return result + 1;
		}

		static value_type high(value_type result, uint32_t ch)
		{
			(void)ch;

			*result = '?';

			return result + 1;
		}
	};

	struct utf8_decoder
	{
		typedef uint8_t type;

		template <typename Traits> static inline typename Traits::value_type process(const uint8_t* data, size_t size, typename Traits::value_type result, Traits)
		{
			const uint8_t utf8_byte_mask = 0x3f;

			while (size)
			{
				uint8_t lead = *data;

				// 0xxxxxxx -> U+0000..U+007F
				if (lead < 0x80)
				{
					result = Traits::low(result, lead);
					data += 1;
					size -= 1;

					// process aligned single-byte (ascii) blocks
					if ((reinterpret_cast<uintptr_t>(data) & 3) == 0)
					{
						// round-trip through void* to silence 'cast increases required alignment of target type' warnings
						while (size >= 4 && (*static_cast<const uint32_t*>(static_cast<const void*>(data)) & 0x80808080) == 0)
						{
							result = Traits::low(result, data[0]);
							result = Traits::low(result, data[1]);
							result = Traits::low(result, data[2]);
							result = Traits::low(result, data[3]);
							data += 4;
							size -= 4;
						}
					}
				}
				// 110xxxxx -> U+0080..U+07FF
				else if (static_cast<unsigned int>(lead - 0xC0) < 0x20 && size >= 2 && (data[1] & 0xc0) == 0x80)
				{
					result = Traits::low(result, ((lead & ~0xC0) << 6) | (data[1] & utf8_byte_mask));
					data += 2;
					size -= 2;
				}
				// 1110xxxx -> U+0800-U+FFFF
				else if (static_cast<unsigned int>(lead - 0xE0) < 0x10 && size >= 3 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80)
				{
					result = Traits::low(result, ((lead & ~0xE0) << 12) | ((data[1] & utf8_byte_mask) << 6) | (data[2] & utf8_byte_mask));
					data += 3;
					size -= 3;
				}
				// 11110xxx -> U+10000..U+10FFFF
				else if (static_cast<unsigned int>(lead - 0xF0) < 0x08 && size >= 4 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80 && (data[3] & 0xc0) == 0x80)
				{
					result = Traits::high(result, ((lead & ~0xF0) << 18) | ((data[1] & utf8_byte_mask) << 12) | ((data[2] & utf8_byte_mask) << 6) | (data[3] & utf8_byte_mask));
					data += 4;
					size -= 4;
				}
				// 10xxxxxx or 11111xxx -> invalid
				else
				{
					data += 1;
					size -= 1;
				}
			}

			return result;
		}
	};

	template <typename opt_swap> struct utf16_decoder
	{
		typedef uint16_t type;

		template <typename Traits> static inline typename Traits::value_type process(const uint16_t* data, size_t size, typename Traits::value_type result, Traits)
		{
			while (size)
			{
				uint16_t lead = opt_swap::value ? endian_swap(*data) : *data;

				// U+0000..U+D7FF
				if (lead < 0xD800)
				{
					result = Traits::low(result, lead);
					data += 1;
					size -= 1;
				}
				// U+E000..U+FFFF
				else if (static_cast<unsigned int>(lead - 0xE000) < 0x2000)
				{
					result = Traits::low(result, lead);
					data += 1;
					size -= 1;
				}
				// surrogate pair lead
				else if (static_cast<unsigned int>(lead - 0xD800) < 0x400 && size >= 2)
				{
					uint16_t next = opt_swap::value ? endian_swap(data[1]) : data[1];

					if (static_cast<unsigned int>(next - 0xDC00) < 0x400)
					{
						result = Traits::high(result, 0x10000 + ((lead & 0x3ff) << 10) + (next & 0x3ff));
						data += 2;
						size -= 2;
					}
					else
					{
						data += 1;
						size -= 1;
					}
				}
				else
				{
					data += 1;
					size -= 1;
				}
			}

			return result;
		}
	};

	template <typename opt_swap> struct utf32_decoder
	{
		typedef uint32_t type;

		template <typename Traits> static inline typename Traits::value_type process(const uint32_t* data, size_t size, typename Traits::value_type result, Traits)
		{
			while (size)
			{
				uint32_t lead = opt_swap::value ? endian_swap(*data) : *data;

				// U+0000..U+FFFF
				if (lead < 0x10000)
				{
					result = Traits::low(result, lead);
					data += 1;
					size -= 1;
				}
				// U+10000..U+10FFFF
				else
				{
					result = Traits::high(result, lead);
					data += 1;
					size -= 1;
				}
			}

			return result;
		}
	};

	struct latin1_decoder
	{
		typedef uint8_t type;

		template <typename Traits> static inline typename Traits::value_type process(const uint8_t* data, size_t size, typename Traits::value_type result, Traits)
		{
			while (size)
			{
				result = Traits::low(result, *data);
				data += 1;
				size -= 1;
			}

			return result;
		}
	};

}
