#pragma once
#include <stddef.h>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <intrin.h>

//TODO
//normalize non-UTF characters:
//    https://github.com/voku/portable-utf8/blob/master/src/voku/helper/UTF8.php#1160


namespace utf8util
{



	//LogBase2 ~ 26 sec
	//int LogBase2_32(uint32_t n) { static const int table[32] = { 0,  9,  1, 10, 13, 21,  2, 29, 11, 14, 16, 18, 22, 25,  3, 30, 8, 12, 20, 28, 15, 17, 24,  7, 19, 27, 23,  6, 26,  5,  4, 31 }; n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16; return n ? table[(n * 0x07C4ACDD) >> 27] : -1; };
	//inline int LogBase2_64(uint64_t n) { static const int table[64] = { 0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56, 45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5, 63 }; n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16; n |= n >> 32; return n ? table[(n * 0x03F6EAF2CD271461) >> 58] : -1; };
	//ilog2 ~ 13 sec
	//inline int ilog2_32(uint32_t x) { unsigned long index; return _BitScanForward(&index, x) ? index : -1; };
	//inline int ilog2_64(uint64_t x) { register unsigned long index; return _BitScanForward64(&index, x) ? index : -1; };
	//ilog2_generic ~ 24 sec
	//inline int ilog2_generic_64(register uint64_t x)
	//{
	//	register uint64_t r = 0;
	//	if (x >= 1LL << 32) { x >>= 32; r |= 32; }
	//	if (x >= 1LL << 16) { x >>= 16; r |= 16; }
	//	if (x >= 1LL << 8) { x >>= 8; r |= 8; }
	//	if (x >= 1LL << 4) { x >>= 4; r |= 4; }
	//	if (x >= 1LL << 2) { x >>= 2; r |= 2; }
	//	if (x >= 1LL << 1) r |= 1;
	//	return x ? (int)r : -1;
	//};
	//fast_log2 ~ 9 sec
	inline int fast_log2(register double x) { return (reinterpret_cast<uint64_t&>(x) >> 52) - 1023; };
	//inline int fast_log2(register double x) { return ((reinterpret_cast<uint64_t&>(x) >> 52) & 2047) - 1023; };
	//inline int float_log2(float x) { return ((*reinterpret_cast<uint32_t*>(&x) >> 23) & 255) - 127; };

	namespace memory
	{

		//set default memory allocation functions
		struct allocator
		{
			static void* malloc(size_t size) { return ::malloc(size); };
			static void free(void* ptr) { return ::free(ptr); };
		};

		//All data blocks inside pages have data_alignment = sizeof(void*) = 4 bytes (x86-32) or 8 bytes (x86-64).
		//Data structure:
		//        | page header   | data header 0 | data block 0 | data header 1 | data block 1 | ... |data header n | unused block   |last data header |
		//x86-32: | 32*4=128 bytes| 2*4=8 bytes   |              |   8 or 16     |              | ... |  8 or 16     |                |    8 or 16      |
		//x86-64: | 32*8=256 bytes| 2*8=16 bytes  |              |    bytes      |              | ... |   bytes      |                |      bytes      |
		//        |-------------------------------|------------------------------|-----------------------------------|----------------|-----------------|
		//   ptr: |page_ptr                       |ptr0                          |ptr1                               |                |                 |page_ptr +
		//        |                               |                              |                                   |                |                 | page_size
		//        --------------------------------------------------------------------------------------------------------------------------------------|
		//
		//????
		//For data sizes <= 16 bytes (x86-32) or 32 bytes (x86-64), allocated one block 384 + 4096 bytes.
		//Number blocks = 1024 blocks.
		//First  level - blocks busy bits. Occupy 1bit*1024 = 128 bytes.
		//Second level - blocks sizes by 2 bits =>
		//       00 -> 4 bytes, 01 -> 8 bytes, 10 -> 12 bytes, 11 -> 16 bytes. Occupy 2bit*1024 = 256 bytes.
		//Total header size = 128 + 256 = 384 bytes.
		//Total data size   = 1024 * 4 bytes = 4096 bytes.



//#ifdef _WIN64
//# define DATA_PTR (uint64_t*)
//#else
//# define DATA_PTR (uint32_t*)
//#endif

		//enum data_constant
		//{
		//	data_alignment = sizeof(void*),
		//	data_alignment_mask = ~(data_alignment - 1),
		//	data_header_offset = offsetof(struct data_header, next_unused),
		//	page_header_bytes = sizeof(struct page_header),
		//	unused_header_bytes = sizeof(struct data_header) - offsetof(struct data_header, next_unused),
		//	min_block_size = sizeof(struct data_header)
		//};

#define data_alignment       sizeof(void*)
#define data_alignment_mask  ~(data_alignment - 1)
#define data_header_offset   offsetof(struct data_header, next_unused)
#define unused_header_bytes  (sizeof(struct data_header) - data_header_offset)
#define page_header_bytes    sizeof(struct page_header)
#define min_block_size       sizeof(struct data_header)

#define cast_data_header(p,offset)  reinterpret_cast<data_header*>(reinterpret_cast<uint8_t*>(p) + (offset))

		struct page_header
		{
			struct data_header* unused_ptr[32]; //offsets to first unused block ordered by size
		};
		struct data_header
		{
			size_t size; //block size, 0 bit is busy_bit: 1 - data block is busy, 0 - data block is unused block
			struct data_header* prev; //ptr to previuos data block
			struct data_header* next_unused; //ptr to next unused block
			struct data_header* prev_unused; //ptr to previous unused block
		};

		class page_manager
		{
		public:
			page_manager() : _pool_ptr(0), _pool_size(0) {};
			bool create_with_pool(void* pool, size_t size)
			{
				_pool_ptr = 0;
				_pool_size = 0;
				if (pool == 0
					|| size < (page_header_bytes + data_header_offset + min_block_size)
					|| (reinterpret_cast<size_t>(pool) & (data_alignment - 1)) != 0
					|| (data_alignment & (data_alignment - 1)) != 0)return 0;
				_pool_ptr = static_cast<page_header*>(pool);
				_pool_size = size & data_alignment_mask;

				::memset(_pool_ptr, 0, sizeof(page_header));

				//init first data_header
				data_header* p = cast_data_header(_pool_ptr, page_header_bytes);
				p->size = _pool_size - page_header_bytes - data_header_offset;
				p->prev = 0;
				link_unused_block(p);
				//init last data_header at end pool
				data_header* p_last = cast_data_header(p, p->size);
				p_last->size = 0;
				p_last->prev = p;
				return true;
			};
			size_t get_used_size()
			{
				data_header *p = cast_data_header(_pool_ptr, page_header_bytes);
				size_t size, total = page_header_bytes + data_header_offset;
				while (p && p->size)
				{
					size = p->size & data_alignment_mask;
					if (p->size & 1)total += size;
					p = cast_data_header(p, size);
				}
				return total;
			};
		private:
			inline data_header** get_first_unused_block_by_size(register size_t size)
			{
				//index = ilog2(size)
				register union { double d; int64_t i; }v = { (double)size };
				v.i = (v.i >> 52) - 1026; if (v.i < 0) v.i = 0;
				return &_pool_ptr->unused_ptr[v.i];
			};
			void unlink_unused_block(data_header* p)
			{
				data_header* *pu = get_first_unused_block_by_size(p->size);
				if (p->prev_unused)
					p->prev_unused->next_unused = p->next_unused;
				if (p->next_unused)
					p->next_unused->prev_unused = p->prev_unused;
				if (p->prev_unused == 0)
					*pu = p->next_unused;
			};
			void link_unused_block(data_header* p)
			{
				p->next_unused = 0;
				p->prev_unused = 0;
				p->size &= data_alignment_mask;
				data_header* *pu = get_first_unused_block_by_size(p->size);
				if (*pu) 
				{
					(*pu)->prev_unused = p;
					p->next_unused = *pu;
				}
				*pu = p;
			};
			data_header* find_unused_block(size_t size)
			{
				data_header
					*p = 0,
					**pu = get_first_unused_block_by_size(size),
					**end = &_pool_ptr->unused_ptr[32];
				while (1)
				{
					if (p == 0)
					{
						while (pu < end && *pu == 0)pu++;
						if (pu >= end) return 0;
					}
					p = *pu;
					if (p->size >= size) break;
					p = p->next_unused;
				}
				return p;
			};
		public:
			void* malloc(size_t size)
			{
				if (size == 0)return 0;

				//align size
				size_t size_aligned = (size + data_header_offset + data_alignment - 1) & data_alignment_mask;

				//find unused block
				data_header* p = find_unused_block(size_aligned);
				if (p == 0)return 0;

				//unlink unused block
				unlink_unused_block(p);
				
				size_t unused = p->size;

				//!!!here you can set the margin for the data block
				if (unused < unused_header_bytes)
				{
					//если в unused нельзя вместить unused_header,
					//то весь unused block выделим для data block

					//set new data block
					p->size = unused | 1;
				}
				else
				{
					//если в unused можно вместить unused_header,
					//то выделим в нем data block, а оставшееся
					//простанство установим как unused block

					//set new data block
					p->size = size_aligned | 1;

					//set new unused block
					data_header* p_new = cast_data_header(p, size_aligned);
					p_new->size = unused - size_aligned;
					p_new->prev = p;
					//link unused block
					link_unused_block(p_new);

					//update prev_size of next data block
					data_header* p_next = cast_data_header(p, unused);
					p_next->prev = p_new;
				}
				return reinterpret_cast<uint8_t*>(p) + data_header_offset;
			};
			void free(void* ptr)
			{
				assert(ptr != 0);
				if (ptr == 0)return;
				
				data_header *p = cast_data_header(ptr, -ptrdiff_t(data_header_offset));

				//!!! при такой стратегии слева и справа от data block
				//!!! может быть только один unused block

				size_t unused = p->size & data_alignment_mask;
				assert(unused > 0);

				data_header* p_next = cast_data_header(p, unused);
				if ((p_next->size & 1) == 0)
				{
					unlink_unused_block(p_next);
					unused += p_next->size;
				}
				if (p->prev && (p->prev->size & 1) == 0)
				{
					unlink_unused_block(p->prev);
					unused += p->prev->size;
					p = p->prev;
				}
				p->size = unused;
				//update prev_size of next data_header
				p_next = cast_data_header(p, unused);
				p_next->prev = p;
				//link new unused block
				link_unused_block(p);
			};

		private:
			page_header* _pool_ptr;
			size_t       _pool_size;
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
