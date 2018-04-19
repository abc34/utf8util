#pragma once
#include <stddef.h>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <intrin.h>


namespace LZ
{
	
#if 0	
	template<typename T>
	class vec
	{//not used new operator
	public:
		vec() : _items(0) { init(0, 16); }
		vec(int size) : _items(0) { init(0, size); }
		vec(vec&& v) : _items(0) { init(v._items, v._size); v._items = 0; }
		~vec() { clean(); }
		T* operator*() { return _items; }
		bool resize(int new_size)
		{
			assert(new_size >= 0);
			T* p = _items = manager.malloc(sizeof(T)*new_size);
			::memcpy_s(p, new_size * sizeof(T), _items, _size);
			return init(p, new_size);
		}
	private:
		bool init(T* p, int size) { clean(); _size = size; _items = p; if (p == 0)_items = manager.malloc(sizeof(T)*_size); return _items != 0; }
		void clean() { if (_items != 0) { manager.free(_items); _items = 0; _size = 0; } }
		T*    _items;
		int _size;
	};
#endif
	
	
	
	namespace utf8util
	{
		//sources:
		//https://en.wikipedia.org/wiki/UTF-8
		//https://github.com/voku/portable-utf8/tree/master/tests
		//https://github.com/htacg/tidy-html5/blob/next/src/utf8.c
		//https://github.com/JuliaLang/utf8proc/blob/master/utf8proc.c
		//https://github.com/ww898/utf-cpp/blob/master/test/utf_converters_test.cpp
		//https://github.com/tchwork/utf8/blob/master/tests/Utf8/Utf8StrcasecmpTest.php
		//https://github.com/svaarala/duktape/blob/master/src-input/duk_unicode_support.c
		//
		//other info
		//https://dxr.mozilla.org/mozilla-central/source/js/src/vm/Unicode.h
		//https://dxr.mozilla.org/mozilla-central/source/js/src/vm/Unicode.cpp
		//https://dxr.mozilla.org/mozilla-central/source/js/src/vm/CharacterEncoding.cpp
		//https://dxr.mozilla.org/mozilla-central/source/intl/icu/source/common/unicode/uchar.h
		//


		//NotAChar:       0xFFFF or bytes:0xEF,0xBF,0xBF
		//ByteOrderMark:  0xFEFF or bytes:0xEF,0xBB,0xBF
		//ByteOrderMark2: 0xFFFE or bytes:0xEF,0xBF,0xBE

		//Legal UTF - 8 byte sequences
		//http://www.unicode.org/versions/corrigendum1.html
		//	Code point          1st byte    2nd byte    3rd byte    4th byte
		//	----------          --------    --------    --------    --------
		//	U+0000 .. U+007F     00..7F
		//	U+0080 .. U+07FF     C2..DF      80..BF
		//	U+0800 .. U+0FFF     E0          A0..BF      80..BF
		//	U+1000 .. U+FFFF     E1..EF      80..BF      80..BF
		//
		//excluding
		//[ U+D800 .. U+DFFF     ED          A0..BF      80..BF ]
		//
		//	U+10000.. U+3FFFF    F0          90..BF      80..BF      80..BF
		//	U+40000.. U+FFFFF    F1..F3      80..BF      80..BF      80..BF
		//	U+100000..U+10FFFF   F4          80..8F      80..BF      80..BF

		//UTF16
		//  U+D800 .. U+DFFF forbidden
		//http://www.unicode.org/faq/utf_bom.html


		//error codes:
#define RESULT_OK                     0
#define ERROR_DST_LENGTH              2
#define ERROR_UTF8_DECODE_RANGE       3
#define ERROR_UTF8_ENCODE_RANGE       4
#define ERROR_UTF8_CONTINUATION_BYTE  5
#define ERROR_UTF16_DECODE_RANGE      6
#define ERROR_UTF16_ENCODE_RANGE      7
#define ERROR_UTF16_CONTINUATION_WORD 8
#define ERROR_UTF32_DECODE_RANGE      9
#define ERROR_UTF32_ENCODE_RANGE      10
//replacement_char -> "�" 0xFFFD utf8 -> [EF,BF,BD]
#define R_CHAR        0xFFFD
#define R_CHAR_UTF8_0 0xEF
#define R_CHAR_UTF8_1 0xBF
#define R_CHAR_UTF8_2 0xBD
//options on invalid chars
#define OPT_REPLACE_INVALID   0
#define OPT_SKIP_INVALID      1
#define OPT_BREAKON_INVALID   2


		struct validation_info
		{
			uint8_t status;
			uint32_t nerrors;
			uint32_t length8;
			uint32_t length16;
			uint32_t length32;
			void* src_next;
		};


		//checking UTF8 string for validity
		//also counting the number uint16 and uint32 data elements nedeed to convert utf8 to utf16 and utf32
		//src_length - src length, uint8_t data elements
		//option     - OPT_*, for OPT_BREAKON_INVALID return validity status on first error
		//vi         - ptr to validation_info struct, or nullptr
		//return RESULT_OK on success or ERROR_UTF8_* on error occurs
		uint8_t utf8_validation(uint8_t* src, uint32_t src_length, uint8_t option, validation_info* vi)
		{
			uint8_t ret = RESULT_OK;
			uint32_t ncount32 = 0, ncount16 = 0, nerrors = 0, len = src_length;
			while (src_length > 0)
			{
				uint8_t c = *src++;
				if (c < 0x80) { src_length -= 1; goto on_ok; }
				else if (c < 0xC2 || c > 0xF4) { src_length -= 1; goto on_error_range; }
				else if (c < 0xE0)
				{
					if (src_length < 2) { src_length -= 1; goto on_error_range; }
					c = *src++;if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; }
					src_length -= 2;
					goto on_ok;
				}
				else if (c < 0xF0)
				{
					if (src_length < 3) { src_length -= 1; goto on_error_range; }
					if (c == 0xE0) { c = *src++; if (c < 0xA0 || c > 0xBF) { src_length -= 1; goto on_error_continuation; } }
					else if (c == 0xED) { c = *src++; if (c < 0x80 || c > 0x9F) { src_length -= 1; goto on_error_continuation; } }
					else { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
					c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
					src_length -= 3;
					goto on_ok;
				}
				else
				{
					if (src_length < 4) { src_length -= 1; goto on_error_range; }
					if (c == 0xF0) { c = *src++; if (c < 0x90 || c > 0xBF) { src_length -= 1; goto on_error_continuation; } }
					else if (c < 0xF4) { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
					else { c = *src++; if (c < 0x80 || c > 0x8F) { src_length -= 1; goto on_error_continuation; } }
					c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
					c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 3; goto on_error_continuation; }
					src_length -= 4; ncount16++;
					goto on_ok;
				}
			on_error_continuation:
				nerrors++;
				src--;
				if (option == OPT_BREAKON_INVALID) { ret = ERROR_UTF8_CONTINUATION_BYTE; break; }
				goto on_ok;
			on_error_range:
				nerrors++;
				if (option == OPT_BREAKON_INVALID) { ret = ERROR_UTF8_DECODE_RANGE; src--; break; }
			on_ok:
				ncount32++;
			}
			if (vi != nullptr)
			{
				ncount16 += ncount32;
				if (option == OPT_SKIP_INVALID) { ncount16 -= nerrors; ncount32 -= nerrors; }
				vi->length8 = len - src_length;
				vi->length16 = ncount16;
				vi->length32 = ncount32;
				vi->nerrors = nerrors;
				vi->src_next = src;
				vi->status = ret;
			}
			return ret;
		}

		//checking UTF16 string for validity
		//also counting the number uint8 and uint32 data elements nedeed to convert utf16 to utf8 and utf32
		//src_length - src length, uint16_t data elements
		//option     - OPT_*, for OPT_BREAKON_INVALID return validity status on first error
		//vi         - ptr to validation_info struct, or nullptr
		//return RESULT_OK on success or ERROR_UTF16_* on error occurs
		uint8_t utf16_validation(uint16_t* src, uint32_t src_length, uint8_t option, validation_info* vi)
		{
			uint8_t ret = RESULT_OK;
			uint16_t c, *src_end = src + src_length;
			uint32_t ncount32 = 0, ncount8 = 0, nerrors = 0;
			while (src < src_end)
			{
				c = *src++;
				if (c < 0x0080) { ncount8 += 1; }
				else if (c < 0x0800) { ncount8 += 2; }
				else if (c < 0xD800 || c > 0xDFFF) { ncount8 += 3; }
				else if (c < 0xDC00)
				{
					if (src == src_end || *src < 0xDC00 || *src > 0xDFFF)
					{
						nerrors++;
						if (option == OPT_BREAKON_INVALID) { ret = ERROR_UTF16_CONTINUATION_WORD; break; }
					}
					else { ncount8 += 4; src++; }
				}
				else
				{
					nerrors++;
					if (option == OPT_BREAKON_INVALID) { ret = ERROR_UTF16_ENCODE_RANGE; src--; break; }
				}
				ncount32++;
			}
			if (vi != nullptr)
			{
				if (option == OPT_SKIP_INVALID) { ncount32 -= nerrors; }
				else { ncount8 += nerrors * 3; }
				vi->length8 = ncount8;
				vi->length16 = src_length - uint32_t(src_end - src);
				vi->length32 = ncount32;
				vi->nerrors = nerrors;
				vi->src_next = src;
				vi->status = ret;
			}
			return ret;
		}




		//counting the number uint16 data elements nedeed to convert utf8 to utf16
		//src_length - src length, uint8_t elements
		//unsafe, for valid utf8, but fast
		//uint32_t utf8to16_length_unsafe(uint8_t* src, uint32_t src_length)
		//{
		//	uint32_t i = 0, ncount = 0;
		//	while (i < src_length)
		//	{
		//		uint8_t c = src[i];
		//		if (c < 0x80) { i += 1; }
		//		else if (c < 0xE0) { i += 2; }
		//		else if (c < 0xF0) { i += 3; }
		//		else { i += 4; ncount++; }
		//		ncount++;
		//	}
		//	return ncount;
		//}
		uint32_t utf8to16_length_unsafe(uint8_t* src, uint32_t src_length)
		{
			uint32_t i = 0, ncount = 0;
			while (i < src_length)
			{
				int8_t c = src[i];
				if (c >= 0) { i++; }          // 0 ... 127
				else if (c < -32) { i += 2; } //-64...-33
				else if (c < -16) { i += 3; } //-32...-17
				else { i += 4; ncount++; }    //-16...-1
				ncount++;
			}
			return ncount;
		}
		//counting the number uint16 data elements nedeed to convert utf8 to utf16
		//src_length - src length, uint8_t elements
		//option     - OPT_*
		uint32_t utf8to16_length(uint8_t* src, uint32_t src_length, uint8_t option)
		{
			uint32_t ncount = 0, nerrors = 0;
			while (src_length > 0)
			{
				uint8_t c = *src++;
				if (c < 0x80) { src_length -= 1; goto on_ok; }
				else if (c < 0xC2 || c > 0xF4) { src_length -= 1; goto on_error_range; }
				else if (c < 0xE0)
				{
					if (src_length < 2) { src_length -= 1; goto on_error_range; }
					c = *src++;if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; }
					src_length -= 2;
					goto on_ok;
				}
				else if (c < 0xF0)
				{
					if (src_length < 3) { src_length -= 1; goto on_error_range; }
					if (c == 0xE0) { c = *src++; if (c < 0xA0 || c > 0xBF) { src_length -= 1; goto on_error_continuation; } }
					else if (c == 0xED) { c = *src++; if (c < 0x80 || c > 0x9F) { src_length -= 1; goto on_error_continuation; } }
					else { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
					c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
					src_length -= 3;
					goto on_ok;
				}
				else
				{
					if (src_length < 4) { src_length -= 1; goto on_error_range; }
					if (c == 0xF0) { c = *src++; if (c < 0x90 || c > 0xBF) { src_length -= 1; goto on_error_continuation; } }
					else if (c < 0xF4) { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
					else { c = *src++; if (c < 0x80 || c > 0x8F) { src_length -= 1; goto on_error_continuation; } }
					c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
					c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 3; goto on_error_continuation; }
					src_length -= 4; ncount++;
					goto on_ok;
				}
			on_error_continuation:
				src--;
			on_error_range:
				nerrors++;
				if (option == OPT_BREAKON_INVALID) { return ncount; }
			on_ok:
				ncount++;
			}
			if (option == OPT_SKIP_INVALID) { return ncount - nerrors; }
			return ncount;
		}

		//counting the number uint8 data elements nedeed to convert utf16 to utf8
		//src_length - src length, uint16_t elements
		//option     - OPT_*
		uint32_t utf16to8_length(uint16_t* src, uint32_t src_length, uint8_t option)
		{
			uint16_t *src_end = src + src_length;
			uint32_t ncount = 0;
			while (src < src_end)
			{
				uint16_t c = *src++;
				if (c < 0x0080) { ncount += 1; }
				else if (c < 0x0800) { ncount += 2; }
				else if (c < 0xD800 || c > 0xDFFF) { ncount += 3; }
				else if (c < 0xDC00)
				{
					if (src == src_end || *src < 0xDC00 || *src > 0xDFFF) { if (option == OPT_BREAKON_INVALID) { return ncount; } if (option == OPT_REPLACE_INVALID) { ncount += 3; } }
					else { ncount += 4; src++; }
				}
				else { if (option == OPT_BREAKON_INVALID) { return ncount; } if (option == OPT_REPLACE_INVALID) { ncount += 3; } }
			}
			return ncount;
		}
		//counting the number uint8 data elements nedeed to convert utf16 to utf8
		//src_length - src length, uint16_t elements
		//unsafe, for valid utf16, but fast
#define utf16to8_length_unsafe(src, src_length) utf16to8_length((src), (src_length), OPT_REPLACE_INVALID)

		//counting the number uint32 data elements nedeed to convert utf8 to utf32
		//src_length - src length, uint8_t elements
		//unsafe, for valid utf8, but fast
		uint32_t utf8to32_length_unsafe(uint8_t* src, uint32_t src_length)
		{
			uint32_t i = 0, ncount = 0;
			while (i < src_length)
			{
				uint8_t c = src[i];
				if (c < 0x80) { i += 1; }
				else if (c < 0xE0) { i += 2; }
				else if (c == 0xE0) { i += 3; }
				else { i += 4; }
				ncount++;
			}
			return ncount;
		}
		//uint32_t utf8to32_length_unsafe_(uint8_t* src, uint32_t src_length)
		//{
		//	uint32_t i = 0, ncount = 0;
		//	while (i < src_length)
		//	{
		//		int8_t c = src[i];
		//		if (c >= 0) { i += 1; }       // 0 ... 127
		//		else if (c < -32) { i += 2; } //-64...-33
		//		else if (c < -16) { i += 3; } //-32...-17
		//		else { i += 4; }              //-16...-1
		//		ncount++;
		//	}
		//	return ncount;
		//}
		//counting the number uint32 data elements nedeed to convert utf8 to utf32
		//src_length - src length, uint8_t elements
		//option     - OPT_*
		uint32_t utf8to32_length(uint8_t* src, uint32_t src_length, uint8_t option)
		{
			uint32_t ncount = 0, nerrors = 0;
			while (src_length > 0)
			{
				uint8_t c = *src++;
				if (c < 0x80) { src_length -= 1; goto on_ok; }
				else if (c < 0xC2 || c > 0xF4) { src_length -= 1; goto on_error_range; }
				else if (c < 0xE0)
				{
					if (src_length < 2) { src_length -= 1; goto on_error_range; }
					c = *src++;if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; }
					src_length -= 2;
					goto on_ok;
				}
				else if (c < 0xF0)
				{
					if (src_length < 3) { src_length -= 1; goto on_error_range; }
					if (c == 0xE0) { c = *src++; if (c < 0xA0 || c > 0xBF) { src_length -= 1; goto on_error_continuation; } }
					else if (c == 0xED) { c = *src++; if (c < 0x80 || c > 0x9F) { src_length -= 1; goto on_error_continuation; } }
					else { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
					c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
					src_length -= 3;
					goto on_ok;
				}
				else
				{
					if (src_length < 4) { src_length -= 1; goto on_error_range; }
					if (c == 0xF0) { c = *src++; if (c < 0x90 || c > 0xBF) { src_length -= 1; goto on_error_continuation; } }
					else if (c == 0xF4) { c = *src++; if (c < 0x80 || c > 0x8F) { src_length -= 1; goto on_error_continuation; } }
					else { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
					c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
					c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 3; goto on_error_continuation; }
					src_length -= 4;
					goto on_ok;
				}
			on_error_continuation:
				src--;
			on_error_range:
				nerrors++;
				if (option == OPT_BREAKON_INVALID) { return ncount; }
			on_ok:
				ncount++;
			}
			if (option == OPT_SKIP_INVALID) { return ncount - nerrors; }
			return ncount;
		}
		//counting the number uint8 nedeed to convert utf32 to utf8
		//src_length - src length, uint32_t elements
		//option     - OPT_*
		uint32_t utf32to8_length(uint32_t* src, uint32_t src_length, uint8_t option)
		{
			uint32_t ncount = 0;
			uint32_t c, *src_end = src + src_length;
			while (src < src_end)
			{
				c = *src++;
				if (c < 0x80) { ncount++; }
				else if (c < 0x0800) { ncount += 2; }
				else if (c < 0xD800) { ncount += 3; }
				else if (c > 0xDFFF && c < 0x110000)
				{
					if (c < 0x10000) { ncount += 3; }
					else { ncount += 4; }
				}
				else { if (option == OPT_BREAKON_INVALID) { return ncount; } if (option == OPT_REPLACE_INVALID) { ncount += 3; } }
			}
			return ncount;
		}
		//counting the number uint8 nedeed to convert utf32 to utf8
		//src_length - src length, uint32_t elements
		//unsafe, for valid utf32, but fast
#define utf32to8_length_unsafe(src, src_length) utf32to8_length((src), (src_length), OPT_REPLACE_INVALID)

#define UTF8_DECODE(cp, src, src_length, option) \
	{ \
		uint8_t __c = *src++; \
		if (__c < 0x80) { cp = uint32_t(__c); src_length--; goto on_ok;} \
		else if (__c < 0xC2 || __c > 0xF4) { goto on_error_range;} \
		else if (__c < 0xE0) \
		{ \
			if (src_length < 2) { goto on_error_range; } \
			cp = uint32_t(__c & 0x1F) << 6; \
			__c = *src++;if ((__c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } \
			cp |= uint32_t(__c & 0x3F); \
			src_length -= 2; \
			goto on_ok; \
		} \
		else if (__c < 0xF0) \
		{ \
			if (src_length < 3) { goto on_error_range; } \
			cp = uint32_t(__c & 0x0F) << 12; \
			if( __c == 0xE0) { __c = *src++; if (__c < 0xA0 || __c > 0xBF) { src_length -= 1; goto on_error_continuation; } } \
			else if (__c == 0xED) { __c = *src++; if (__c < 0x80 || __c > 0x9F) { src_length -= 1; goto on_error_continuation; } } \
			else { __c = *src++; if ((__c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } } \
			cp |= uint32_t(__c & 0x3F) << 6; \
			__c = *src++; if ((__c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; } \
			cp |= uint32_t(__c & 0x3F); \
			src_length -= 3; \
			goto on_ok; \
		} \
		else if (__c <= 0xF4) \
		{ \
			if (src_length < 4) { goto on_error_range; } \
			cp = uint32_t(__c & 0x07) << 18; \
			if(__c == 0xF0) { __c = *src++; if (__c < 0x90 || __c > 0xBF) { src_length -= 1; goto on_error_continuation; } } \
			else if(__c < 0xF4) { __c = *src++; if ((__c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } } \
			else { __c = *src++; if (__c < 0x80 || __c > 0x8F) { src_length -= 1; goto on_error_continuation; } } \
			cp |= uint32_t(__c & 0x3F) << 12; \
			__c = *src++; if ((__c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; } \
			cp |= uint32_t(__c & 0x3F) << 6; \
			__c = *src++; if ((__c & 0xC0) != 0x80) { src_length -= 3; goto on_error_continuation; } \
			cp |= uint32_t(__c & 0x3F); \
			src_length -= 4; \
			goto on_ok; \
		} \
	} \
on_error_range: \
		if (option == OPT_BREAKON_INVALID) { return ERROR_UTF8_DECODE_RANGE; } src_length--; if(option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; goto on_ok; \
on_error_continuation: \
		if (option == OPT_BREAKON_INVALID) { return ERROR_UTF8_CONTINUATION_BYTE; } src--; if(option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; \
on_ok:

#define UTF8_NEXT(i, src, src_length, cp, option) \
	{ \
		uint8_t __c = src[i++]; \
		if (__c < 0x80) { cp = __c; goto on_ok; } \
		else if (__c < 0xC2 || __c > 0xF4) { goto on_error_range; } \
		else if (__c < 0xE0) \
		{ \
			if (i >= src_length) { goto on_error_range; } \
			cp = uint32_t(__c & 0x1F) << 6; \
			__c = src[i];if ((__c & 0xC0) != 0x80) { goto on_error_continuation; } \
			cp |= (__c & 0x3F); i++; \
			goto on_ok; \
		} \
		else if (__c < 0xF0) \
		{ \
			if (i + 1 >= src_length) { goto on_error_range; } \
			cp = uint32_t(__c & 0x0F) << 12; \
			if (__c == 0xE0) { __c = src[i]; if (__c < 0xA0 || __c > 0xBF) { goto on_error_continuation; } } \
			else if (__c == 0xED) { __c = src[i]; if (__c < 0x80 || __c > 0x9F) { goto on_error_continuation; } } \
			else { __c = src[i]; if ((__c & 0xC0) != 0x80) { goto on_error_continuation; } } \
			cp |= uint32_t(__c & 0x3F) << 6; i++; \
			__c = src[i]; if ((__c & 0xC0) != 0x80) { goto on_error_continuation; } \
			cp |= (__c & 0x3F); i++; \
			goto on_ok; \
		} \
		else /*if (__c <= 0xF4)*/ \
		{ \
			if (i + 2 >= src_length) { goto on_error_range; } \
			cp = uint32_t(__c & 0x07) << 18; \
			if (__c == 0xF0) { __c = src[i]; if (__c < 0x90 || __c > 0xBF) { goto on_error_continuation; } } \
			else if (__c < 0xF4) { __c = src[i]; if ((__c & 0xC0) != 0x80) { goto on_error_continuation; } } \
			else { __c = src[i]; if (__c < 0x80 || __c > 0x8F) { goto on_error_continuation; } } \
			cp |= uint32_t(__c & 0x3F) << 12; i++; \
			__c = src[i]; if ((__c & 0xC0) != 0x80) { goto on_error_continuation; } \
			cp |= uint32_t(__c & 0x3F) << 6; i++; \
			__c = src[i]; if ((__c & 0xC0) != 0x80) { goto on_error_continuation; } \
			cp |= (__c & 0x3F); i++; \
			goto on_ok; \
		} \
	} \
on_error_range: \
	if (option == OPT_BREAKON_INVALID) { return ERROR_UTF8_DECODE_RANGE; } \
on_error_continuation: \
	if (option == OPT_BREAKON_INVALID) { return ERROR_UTF8_CONTINUATION_BYTE; } \
	if (option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; \
on_ok:

#define UTF8_ENCODE(cp, dst, dst_length, option) \
	if (cp < 0x80) \
	{ \
		if (dst_length < 1) { return ERROR_DST_LENGTH; } \
		*dst++ = static_cast<uint8_t>(cp); dst_length--; \
	} \
	else if (cp < 0x800) \
	{ \
		if (dst_length < 2) { return ERROR_DST_LENGTH; } \
		*dst++ = static_cast<uint8_t>(0xC0 | (cp >> 6)); \
		*dst++ = static_cast<uint8_t>(0x80 | (cp & 0x3F)); \
		dst_length -= 2; \
	} \
	else if (cp < 0xD800) \
	{ \
		if (dst_length < 3) { return ERROR_DST_LENGTH; } \
		*dst++ = static_cast<uint8_t>(0xE0 | (cp >> 12)); \
		*dst++ = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F)); \
		*dst++ = static_cast<uint8_t>(0x80 | (cp & 0x3F)); \
		dst_length -= 3; \
	} \
	else if (cp > 0xDFFF && cp < 0x110000) \
	{ \
		if (cp < 0x10000) \
		{ \
			if (dst_length < 3) { return ERROR_DST_LENGTH; } \
			*dst++ = static_cast<uint8_t>(0xE0 | (cp >> 12)); \
			*dst++ = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F)); \
			*dst++ = static_cast<uint8_t>(0x80 | (cp & 0x3F)); \
			dst_length -= 3; \
		} \
		else /*if (cp < 0x110000)*/ \
		{ \
			if (dst_length < 4) { return ERROR_DST_LENGTH; } \
			*dst++ = static_cast<uint8_t>(0xF0 | (cp >> 18)); \
			*dst++ = static_cast<uint8_t>(0x80 | ((cp >> 12) & 0x3F)); \
			*dst++ = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F)); \
			*dst++ = static_cast<uint8_t>(0x80 | (cp & 0x3F)); \
			dst_length -= 4; \
		} \
	} \
	else \
	{ \
		/*on error out of range*/ \
		if (option == OPT_BREAKON_INVALID) { return ERROR_UTF8_ENCODE_RANGE; } \
		if (option == OPT_REPLACE_INVALID) \
		{ \
			if (dst_length < 3) { return ERROR_DST_LENGTH; } \
			*dst++ = R_CHAR_UTF8_0; \
			*dst++ = R_CHAR_UTF8_1; \
			*dst++ = R_CHAR_UTF8_2; \
			dst_length -= 3; \
		} \
	}

#define UTF16_NEXT(i, src, src_length, c, option) \
	c = src[i++]; \
	/*if (need_swap) c = static_cast<uint16_t>(((c & 0xFF) << 8) | (c >> 8));*/ \
	if ((c & 0xF800) == 0xD800) \
	{ \
		if (c < 0xDC00 && i < src_length) \
		{ \
			uint32_t __c1 = src[i];if (__c1 < 0xDC00 || __c1 > 0xDFFF) { if (option == OPT_BREAKON_INVALID)return ERROR_UTF16_CONTINUATION_WORD;if (option == OPT_SKIP_INVALID)continue; c = R_CHAR; }\
			else { c = (c << 10UL) + __c1 + 0x0FCA02400; i++; } \
		} \
		else { if (option == OPT_BREAKON_INVALID)return ERROR_UTF16_DECODE_RANGE;if (option == OPT_SKIP_INVALID)continue; c = R_CHAR; } \
	}

#define UTF16_DECODE(cp, src, src_length, option) \
	{ \
		uint32_t __c = *src++; \
		/*if (need_swap) c = static_cast<uint16_t>(((c & 0xFF) << 8) | (c >> 8));*/ \
		if (__c < 0xD800 || __c > 0xDFFF) { cp = uint32_t(__c); src_length--; goto on_ok_16; } \
		else if (__c < 0xDC00) \
		{ \
			if (src_length < 2) { goto on_error_range_16; }\
			/*codepoint in ranges U+D800..U+DBFF and U+DC00..U+DFFF is reserved for utf16 surrogate pairs*/ \
			cp = uint32_t(__c & 0x3FF) << 10; \
			__c = *src++; if (__c < 0xDC00 || __c > 0xDFFF) { src_length -= 1; goto on_error_continuation_16; } \
			cp |= uint32_t(__c & 0x3FF); \
			cp += 0x10000; \
			src_length -= 2; \
			goto on_ok_16; \
		} \
	} \
on_error_range_16: \
	if (option == OPT_BREAKON_INVALID) { return ERROR_UTF16_DECODE_RANGE; } src_length--; if(option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; goto on_ok_16; \
on_error_continuation_16: \
	if (option == OPT_BREAKON_INVALID) { return ERROR_UTF16_CONTINUATION_WORD; } src--; if(option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; \
on_ok_16:

#define UTF16_ENCODE(cp, dst, dst_length, option) \
	if (dst_length == 0) { return ERROR_DST_LENGTH; } \
	if (cp < 0xD800) { *dst++ = uint16_t(cp); dst_length--; } \
	else if (cp < 0xE000) \
	{ \
		/*codepoint in ranges U+D800..U+DBFF and U+DC00..U+DFFF is reserved for utf16 surrogate pairs */ \
		if (option == OPT_BREAKON_INVALID) { return ERROR_UTF16_ENCODE_RANGE; } \
		if (option == OPT_REPLACE_INVALID) { *dst++ = R_CHAR; dst_length--; } \
	} \
	else if (cp < 0x10000) \
	{ \
		*dst++ = uint16_t(cp); dst_length--; \
	} \
	else if (cp < 0x110000) \
	{ \
		if(dst_length < 2) { return ERROR_DST_LENGTH; } \
		/*U+010000..U+10FFFF*/ \
		cp -= 0x10000; \
		*dst++ = uint16_t(0xD800 | (cp >> 10));   /*D800..DBFF*/ \
		*dst++ = uint16_t(0xDC00 | (cp & 0x3FF)); /*DC00..DFFF*/ \
		dst_length -= 2; \
	} \
	else \
	{ \
		/*on error emit replacement codepoint*/ \
		if (option == OPT_BREAKON_INVALID) { return ERROR_UTF16_ENCODE_RANGE; } \
		if (option == OPT_REPLACE_INVALID) { *dst++ = R_CHAR; dst_length--; } \
	}

#define UTF32_NEXT(i, src, src_length, cp, option) \
	cp = src[i++]; \
	/*if (need_swap) cp = ((cp & 0xFF) << 24) | ((cp & 0xFF00) << 8) | ((cp & 0xFF0000) >> 8) | (cp >> 24);*/ \
	if ((cp & 0xF800) == 0xD800 || cp > 0x10FFFF) { if (option == OPT_BREAKON_INVALID) { return ERROR_UTF32_DECODE_RANGE; } if (option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; }

#define UTF32_DECODE(cp, src, src_length, option) \
	cp = *src++; src_length--; \
	/*if (need_swap) cp = ((cp & 0xFF) << 24) | ((cp & 0xFF00) << 8) | ((cp & 0xFF0000) >> 8) | (cp >> 24);*/ \
	if ((cp & 0xF800) == 0xD800 || cp > 0x10FFFF) { if (option == OPT_BREAKON_INVALID) { return ERROR_UTF32_DECODE_RANGE; } if (option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; }

#define UTF32_ENCODE(cp, dst, dst_length, option) \
	if (dst_length == 0) { return ERROR_DST_LENGTH; } \
	if ((cp & 0xF800) == 0xD800 || cp > 0x10FFFF) { if (option == OPT_BREAKON_INVALID) { return ERROR_UTF32_ENCODE_RANGE; } if (option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; } \
	*dst++ = cp; dst_length--;

		//convert utf8 string to utf32 string
		//src_length - src string length, uint8_t elements
		//dst_length - dst string length, uint32_t elements
		uint8_t utf8to32(uint8_t* src, uint32_t src_length, uint32_t* dst, uint32_t dst_length, uint8_t option)
		{
			uint32_t i = 0, cp;
			//test BOM: 0xEF,0xBB,0xBF and skip it
			//if (src_length > 2 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) { src += 3; src_length -= 3; }
			while (i < src_length)
			{
				UTF8_NEXT(i, src, src_length, cp, option);
				UTF32_ENCODE(cp, dst, dst_length, OPT_SKIP_INVALID);
			}
			return RESULT_OK;
		}
		//convert utf32 string to utf8 string
		//src_length - src string length, uint32_t elements
		//dst_length - dst string length, uint8_t elements
		uint8_t utf32to8(uint32_t* src, uint32_t src_length, uint8_t* dst, uint32_t dst_length, uint8_t option)
		{
			uint32_t i = 0, cp;
			//test BOM: 0x0000FEFF and skip it
			//if (src_length > 0 && src[0] == 0x0000FEFF) { src++; src_length--; }
			while (i < src_length)
			{
				UTF32_NEXT(i, src, src_length, cp, option);
				UTF8_ENCODE(cp, dst, dst_length, OPT_SKIP_INVALID);
			}
			return RESULT_OK;
		}
		//convert utf8 string to utf16 string
		//src_length - src string length, uint8_t elements
		//dst_length - dst string length, uint16_t elements
		uint8_t utf8to16(uint8_t* src, uint32_t src_length, uint16_t* dst, uint32_t dst_length, uint8_t option)
		{
			uint32_t i = 0, cp;
			//test BOM: 0xEF,0xBB,0xBF and skip it
			//if (src_length > 2 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) { src += 3; src_length -= 3; }
			while (i < src_length)
			{
				UTF8_NEXT(i, src, src_length, cp, option);
				UTF16_ENCODE(cp, dst, dst_length, OPT_SKIP_INVALID);
			}
			return RESULT_OK;
		}
		//convert utf16 string to utf8 string
		//src_length - src string length, uint16_t elements
		//dst_length - dst string length, uint8_t elements
		uint8_t utf16to8(uint16_t* src, uint32_t src_length, uint8_t* dst, uint32_t dst_length, uint8_t option)
		{
			uint32_t i = 0, cp;
			//test BOM: 0xFEFF and skip it
			//if (src_length > 0 && src[0] == 0xFEFF) { src++; src_length--; }
			while (i < src_length)
			{
				UTF16_NEXT(i, src, src_length, cp, option);
				UTF8_ENCODE(cp, dst, dst_length, OPT_SKIP_INVALID);
			}
			return RESULT_OK;
		}

		/*
		//counting the number of unicode codepoints in the utf8 string
		//src_length - src length, uint8_t elements
		//ошибка: более трёх подряд continuation байт считаются одним символом
		uint32_t utf8_codepoints(uint8_t* src, uint32_t src_length)
		{
		uint32_t cp, ncount = 0, *src_i32, *src_end;
		if (src_length == 0) return 0;
		//align to 4 byte
		cp = reinterpret_cast<uint32_t>(src) & 3;
		if (cp != 0)
		{
		if (cp < 2 && src_length > 0) { if ((((*src++) ^ 0x80) & 0xC0) != 0) ncount++; src_length--; }
		if (cp < 3 && src_length > 0) { if ((((*src++) ^ 0x80) & 0xC0) != 0) ncount++; src_length--; }
		if (cp < 4 && src_length > 0) { if ((((*src++) ^ 0x80) & 0xC0) != 0) ncount++; src_length--; }
		}
		if (src_length == 0)return ncount;
		//load as int32_t and count non continuation bytes
		src_i32 = reinterpret_cast<uint32_t*>(src);
		src_end = reinterpret_cast<uint32_t*>(src + (src_length & 0xFFFFFFFC));
		while (src_i32 < src_end)
		{
		cp = *src_i32++;
		cp ^= 0x80808080;
		if ((cp & 0xC0000000) != 0)ncount++;
		if ((cp & 0x00C00000) != 0)ncount++;
		if ((cp & 0x0000C000) != 0)ncount++;
		if ((cp & 0x000000C0) != 0)ncount++;
		}
		//count last bytes
		cp = src_length & 3;
		if (cp != 0)
		{
		src = reinterpret_cast<uint8_t*>(src_end);
		if (cp > 0 && (((*src++) ^ 0x80) & 0xC0) != 0)ncount++;
		if (cp > 1 && (((*src++) ^ 0x80) & 0xC0) != 0)ncount++;
		if (cp > 2 && (((*src++) ^ 0x80) & 0xC0) != 0)ncount++;
		}
		return ncount;
		}
		//counting the number of unicode codepoints in the utf16 string
		//src_length - src length, uint16_t elements
		uint32_t utf16_codepoints(uint16_t* src, uint32_t src_length)
		{
		uint16_t *src_end = src + src_length;
		uint32_t ncount = src_length;
		while (src < src_end)
		{
		if (((*src++) & 0xFC00) == 0xDC00) { ncount--; }
		}
		return ncount;
		}
		//counting the number of unicode codepoints in the utf32 string
		//src_length - src length, uint32_t elements
		uint32_t utf32_codepoints(uint32_t* src, uint32_t src_length)
		{
		//if(size == 0)//for null terminated data
		//	while (*data++ > 0) size++;
		return src_length;
		}
		*/







	}//end namespace utf8util



	namespace memory
	{
		//LogBase2 ~ 26 sec (DeBruijn)
		//int LogBase2_32(uint32_t n) { static const int table[32] = {0,9,1,10,13,21,2,29,11,14,16,18,22,25,3,30,8,12,20,28,15,17,24,7,19,27,23,6,26,5,4,31}; n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16; return n ? table[(n * 0x07C4ACDDU) >> 27] : -1; };
		//inline int LogBase2_64(uint64_t n) { static const int table[64] = {0,58,1,59,47,53,2,60,39,48,27,54,33,42,3,61,51,37,40,49,18,28,20,55,30,34,11,43,14,22,4,62,57,46,52,38,26,32,41,50,36,17,19,29,10,13,21,56,45,25,31,35,16,9,12,44,24,15,8,23,7,6,5,63}; n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16; n |= n >> 32; return n ? table[(n * 0x03F6EAF2CD271461ULL) >> 58] : -1; };
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
		inline int fast_log2(double x) { return (reinterpret_cast<uint64_t&>(x) >> 52) - 1023; };
		inline int fast_log2i(double x){ union { double d; int32_t i[2]; }v = { x }; return (v.i[1] >> 20) - 1023;};
		//inline int fast_log2(register double x) { return ((reinterpret_cast<uint64_t&>(x) >> 52) & 2047) - 1023; };
		//inline int float_log2(float x) { return ((*reinterpret_cast<uint32_t*>(&x) >> 23) & 255) - 127; };

		//--------------------------------------------------------------
		//                TLSF-like memory pool
		//--------------------------------------------------------------
		// Sources:
		// https://github.com/mattconte/tlsf
		// http://www.gii.upv.es/tlsf/main/docs
		// https://code.google.com/archive/p/compcache/wikis/xvMalloc.wiki
		//
		//Для 32 и 64 битных систем:
		//1. Для выделенных блоков заголовок занимает            4 байт (только поле size).
		//   Для освобождённых блоков заголовок занимает        16 байт.
		//2. Адрес выделяемого блока выравнивается            до 4 байт.
		//3. Размеры выделяемых блоков выравниваются          до 4 байт.
		//4. Минимальный размер блока равен                     16 байт (min capacity = 12 байт).
		//5. Размер fti_table для хранения списков unused blocks
		//   (при fti_bits = 5)        равен 828 эл.= 828*4 = 3312 байт.
		//6. Максимальный размер блока не может превышать величины
		//   (при fti_bits = 5)                 = pool_size - 3424 байт.
		//7. Свободные блоки с размерами < fti_first_max*data_alignment,
		//   (при fti_bits = 5) fti_first_max*data_alignment = 256 байт,
		//   попадают в fti_table в соответствии с диапазонами:
		//       |16-19|20-23|...|254-255|       <= с шагом 4 байта,
		//   далее шаг удваивается с пропорциональным удвоением диапазона:
		//       |256-263|264-271|...|504-511|   <= с шагом 8 байт,
		//       |512-527|528-543|...|1008-1023| <= с шагом 16 байт,
		//       |1024-1055|...                  <= с шагом 32 байт,
		//       и т.д.
		//
		//VERSION 2.1
		// - on 32-bit systems, reduce all data_header fields to 4 bytes
		//   on 64-bit systems, reduce all data_header fields to 4 bytes
		// - ptr replaced to int32_t offsets (4 bytes)
		// - added malloc_aligned()
		// - added realloc()
		//
		//VERSION 2.0
		// - on 32-bit systems, reduce all data_header fields to 4 bytes
		//   on 64-bit systems, reduce all data_header fields to 8 bytes
		//
		//
		//
		//Data structure:
		//1) structure of data block 1 after malloc space 1
		//|--------------------------------------------------------------------|---------------------------------
		//|  size   |                    used space 1                          |  size   |   used space 2 ...
		//|---------|----------------------------------------------------------|---------|-----------------------
		//| header1 |ptr1                                                      | header2 |ptr2
		//|--------------------------------------------------------------------|---------------------------------
		//|                      data block 1                                  |     data block 2
		//|--------------------------------------------------------------------|---------------------------------
		//
		//2) structure of data block 1 after free space 1
		//|-----------------------------------------------------------|------------------------------------------
		//|  size   | next_unused  | prev_unused |   unused space 1   |  prev  |  size   |   used space 2 ...
		//|--------------------------------------|--------------------|------------------|-----------------------
		//|             header1                  |                    |     header2      |ptr2
		//|-----------------------------------------------------------|------------------------------------------
		//|                      data block 1                         |              data block 2
		//|-----------------------------------------------------------|------------------------------------------

		//Возможные улучшения (TODO):
		//Для малых размеров 32 бит для поля size избыточно.
		//  Тогда для хранения размера можно использовать столько байт, сколько требуется.
		//  В этом случае для определения 1,2,3 или 4 байт можно использовать 2 и 3 биты size.
		//  Из-за выравниваения по 4 байта адреса блока и его размера, возможно, что
		//  эта мера не будет эффективной. Минимально всё равно будет 4 байта.
		//Можно не вычислять fti, если вместе с size хранить его fti.
		//Можно все указатели сделать uint32_t*, вместо uint8_t*.


		//set default memory allocation functions
		struct allocator
		{
			static inline void* malloc(size_t size) { return ::malloc(size); };
			static inline void* malloc_aligned(size_t size, size_t align) { return ::_aligned_malloc(size, align); };
			static inline void* realloc(void* p, size_t size) { return ::realloc(p, size); }
			static inline void  free(void* ptr) { return ::free(ptr); };
			static inline size_t msize(void* ptr) { return ::_msize(ptr); };
		};

#define data_alignment       4
#define data_alignment_bits  2
#define data_alignment_mask  (~3UL)
#define data_header_size     4
#define data_header_bytes    16
#define data_header_offset   8
#define default_pool_size    0x40000UL
#define busy_bit             1
#define prev_busy_bit        2
#define fti_bits             3
#define fti_first_max        (1UL<<(fti_bits + 1))
#define fti_table_n          (((32 - data_alignment_bits - fti_bits + 1) << fti_bits) - (data_header_bytes / data_alignment))
#define sti_table_n          ((fti_table_n + 31)/32)
#define page_header_bytes    ((fti_table_n + sti_table_n + 1)*4)
#define cast_data_header(p, offset)  reinterpret_cast<data_header*>(reinterpret_cast<uint8_t*>(p) + (offset))
//cast and offset by 4 byte words
#define cast_data_header_32bit(p, offs)  reinterpret_cast<data_header*>(reinterpret_cast<uint32_t*>(p) + (offs))
#define offs_data_header_32bit(p, p_to)  int32_t(reinterpret_cast<uint32_t*>(p_to) - reinterpret_cast<uint32_t*>(p))
//use external allocator functions on failure
#undef USE_EXTERNAL_ALLOCATOR_ON_FAILURE
#undef USE_FTI_FIELD_IN_HEADER

		struct data_header
		{
			 int32_t prev;         //offset to previuos data header  (if prev_block_busy_bit == 0)
			uint32_t size;         //memory block size, 0 bit is busy_bit, 1 bit is prev_block_busy_bit
			 int32_t next_unused;  //offset to next unused block     (if busy_bit == 0)
			 int32_t prev_unused;  //offset to previous unused block (if busy_bit == 0)
#if defined(USE_FTI_FIELD_IN_HEADER)
			uint32_t fti;          //fti value for size field        (if size > data_header_bytes)
#endif
		};
		struct page_header
		{
			uint32_t fti_table[fti_table_n]; //first table  - offsets to first unused block
			uint32_t sti_table[sti_table_n]; //second table - bitmap of used cells in fti_table
			uint32_t memory_in_use;          //memory in use, bytes
		};

		class page_manager
		{
		public:
			page_manager()
			{
				_pool_ptr = 0;_pool_size = 0;
			}
			page_manager(void* pool, uint32_t bytes)
			{
				_pool_ptr = 0;_pool_size = 0; create(pool, bytes);
			}
			~page_manager()
			{
				destroy();
			}
			bool create(void* pool, uint32_t bytes)
			{
				if (_pool_ptr != 0) destroy();
				if (bytes == 0) bytes = default_pool_size;
				if (pool == 0) pool = allocator::malloc(bytes);
				return create_with_pool(pool, bytes);
			}
			bool destroy()
			{
				if (_pool_ptr != 0) allocator::free(_pool_ptr);
				_pool_ptr = 0; _pool_size = 0;
				return true;
			}
			bool reinit()
			{
				if (_pool_ptr != 0)
					return create_with_pool(_pool_ptr, _pool_size);
				return false;
			}
		private:
			bool create_with_pool(void* pool, uint32_t size)
			{//create memory pool
				_pool_ptr = 0;
				_pool_size = 0;
				if (pool == 0
					|| size < (page_header_bytes + data_header_size + data_header_bytes)
					|| ((reinterpret_cast<size_t>(pool) | data_alignment) & (data_alignment - 1)) != 0) return false;
				_pool_ptr = static_cast<page_header*>(pool);
				_pool_size = size & data_alignment_mask;
				//init page header
				::memset(_pool_ptr, 0, page_header_bytes);
				_pool_ptr->memory_in_use = page_header_bytes + data_header_size;
				//init first data_header
				data_header* p = cast_data_header(_pool_ptr, page_header_bytes + data_header_size - data_header_offset);
				p->size = _pool_size - page_header_bytes - data_header_size;
				p->size |= prev_busy_bit;
				//init last data_header at end pool
				data_header* p_last = cast_data_header(p, p->size & data_alignment_mask);
				p_last->size = 0 | busy_bit;
				link_unused_block(p);
				return true;
			};
			inline uint32_t* get_first_unused_block_by_size(uint32_t size)
			{
				assert(size >= data_header_bytes && "size must be greater than or equal to data_header_bytes");
				uint32_t fti = size >> data_alignment_bits;
				if (fti >= fti_first_max)
				{
					register union { double d; uint32_t i[2]; }v = { (double)fti };
					v.i[1] = (v.i[1] >> 20) - 1023 - fti_bits;
					fti = (v.i[1] << fti_bits) + (fti >> v.i[1]);
				}
				fti -= data_header_bytes / data_alignment;
				return &_pool_ptr->fti_table[fti];
			};
			void unlink_unused_block(data_header* p)
			{
				if (p->prev_unused)
				{
					if (p->next_unused) cast_data_header_32bit(p, p->prev_unused)->next_unused += p->next_unused;
					else cast_data_header_32bit(p, p->prev_unused)->next_unused = 0;
				}
				if (p->next_unused)
				{
					if (p->prev_unused) cast_data_header_32bit(p, p->next_unused)->prev_unused += p->prev_unused;
					else cast_data_header_32bit(p, p->next_unused)->prev_unused = 0;
				}
				if (p->prev_unused == 0)
				{
				#if defined(USE_FTI_FIELD_IN_HEADER)
					//restore fti value
					assert((data_header_bytes >> data_alignment_bits) <= fti_first_max);
					uint32_t fti = (p->size >> data_alignment_bits);
					if (fti > fti_first_max) fti = p->fti; else fti -= data_header_bytes / data_alignment;
					uint32_t *pu = &_pool_ptr->fti_table[fti];
					assert(pu == get_first_unused_block_by_size(p->size));
				#else
					uint32_t *pu = get_first_unused_block_by_size(p->size);
				#endif
					if (p->next_unused) *pu = offs_data_header_32bit(_pool_ptr, cast_data_header_32bit(p, p->next_unused));
					else
					{
						*pu = 0;
						_pool_ptr->sti_table[(pu - _pool_ptr->fti_table) >> 5] &= ~(0x80000000UL >> ((pu - _pool_ptr->fti_table) & 31));
					}
				}
			};
			void link_unused_block(data_header* p)
			{
				p->next_unused = 0;
				p->prev_unused = 0;
				p->size &= ~busy_bit;//clear busy_bit
				//update prev field and prev_busy_bit of next data_header
				data_header *p_next = cast_data_header(p, p->size & data_alignment_mask);
				p_next->size &= ~prev_busy_bit;
				p_next->prev = offs_data_header_32bit(p_next, p);
				//link to list
				uint32_t *pu = get_first_unused_block_by_size(p->size);
			#if defined(USE_FTI_FIELD_IN_HEADER)
				//store fti value
				if ((p->size >> data_alignment_bits) > fti_first_max){ p->fti = offs_data_header_32bit(_pool_ptr->fti_table, pu); }
			#endif
				if (*pu)
				{
					p->next_unused = offs_data_header_32bit(p, cast_data_header_32bit(_pool_ptr, *pu));
					cast_data_header_32bit(_pool_ptr, *pu)->prev_unused = -p->next_unused;
				}
				*pu = offs_data_header_32bit(_pool_ptr, p);
				_pool_ptr->sti_table[(pu - _pool_ptr->fti_table) >> 5] |= 0x80000000UL >> ((pu - _pool_ptr->fti_table) & 31);
			};
			data_header* find_unused_block(uint32_t size)
			{
				data_header *p = 0;
				uint32_t
					*pu = get_first_unused_block_by_size(size),
					 m = 0xFFFFFFFFUL >> ((pu - _pool_ptr->fti_table) & 31),
					*pbit = &_pool_ptr->sti_table[(pu - _pool_ptr->fti_table) >> 5],
					*pbit_end = &_pool_ptr->sti_table[sti_table_n];
				while (1)
				{
					if (p == 0)
					{
						m &= (*pbit);
						if (m == 0)
						{
							pbit++; while (pbit < pbit_end && *pbit == 0) pbit++;
							if (pbit >= pbit_end) return 0;
							m = *pbit;
						}
						register union { double d; int32_t i[2]; }v = { (double)m };
						m = 31 + 1023 - (v.i[1] >> 20);
						pu = &_pool_ptr->fti_table[m + ((pbit - _pool_ptr->sti_table) << 5)];
						m++; m = (m < 32) ? 0xFFFFFFFFUL >> m : 0;//shr workaround for 5 bit mask
						p = cast_data_header_32bit(_pool_ptr, *pu);

						assert(pu < &_pool_ptr->fti_table[fti_table_n] && "pu must not point outside of the table");
					}
					if (p->size >= size) break;
					p = p->next_unused == 0 ? 0 : cast_data_header_32bit(p, p->next_unused);
				}
				return p;
			};
			//split unused block
#define split_block(p, required_size, sub_size) \
			{ \
				uint32_t size = p->size & data_alignment_mask; \
				if (size < (required_size) + data_header_bytes) \
				{ \
					/*update busy_bit*/ \
					p->size |= busy_bit; \
					/*update prev_busy_bit of next data block*/ \
					cast_data_header(p, size)->size |= prev_busy_bit; \
				} \
				else \
				{ \
					/*set new data block*/ \
					p->size = (required_size) | (p->size & prev_busy_bit) | busy_bit; \
					/*set new unused block*/ \
					data_header* p_right = cast_data_header(p, (required_size)); \
					p_right->size = size - (required_size); \
					p_right->size |= prev_busy_bit; \
					/*link unused block*/ \
					link_unused_block(p_right); \
				} \
				/*update memory_in_use*/ \
				_pool_ptr->memory_in_use += (p->size & data_alignment_mask) - (sub_size); \
			}
		public:
			size_t get_used_size()
			{
				assert(_pool_ptr > 0 && "pointer must not be 0");
				return _pool_ptr->memory_in_use;
			};
			size_t get_free_size()
			{
				assert(_pool_ptr > 0 && "pointer must not be 0");
				return _pool_size - _pool_ptr->memory_in_use;
			};
			size_t msize(void* ptr)
			{//return capacity (greater than or equal to the size) of used data block, 0 otherwise
				assert(_pool_ptr > 0 && "pointer must not be 0");
			#if defined(USE_EXTERNAL_ALLOCATOR_ON_FAILURE)
				if (ptr < _pool_ptr || ptr >= cast_data_header(_pool_ptr, _pool_size)) { return allocator::msize(ptr); }
			#else
				assert(ptr > _pool_ptr && ptr < cast_data_header(_pool_ptr, _pool_size) && "ptr must be in the pool");
			#endif
				data_header *p = cast_data_header(ptr, -int(data_header_offset));
				return (p->size & busy_bit) == busy_bit ? (p->size & data_alignment_mask) - data_header_size : 0;
			};
			void* malloc(uint32_t size)
			{
				assert(_pool_ptr > 0 && size > 0 && "pointer and size must not be 0");
				if (size == 0) return 0;
				//alignment of size
				uint32_t size_aligned = (size + data_header_size + data_alignment - 1) & data_alignment_mask;
				if (size_aligned < data_header_bytes) size_aligned = data_header_bytes;
				//find unused block
				data_header* p = find_unused_block(size_aligned);
			#if defined(USE_EXTERNAL_ALLOCATOR_ON_FAILURE)
				if (p == 0) { return allocator::malloc(size); }
			#else
				if (p == 0) { return 0; }
			#endif
				//unlink unused block
				unlink_unused_block(p);
				//split unused block
				split_block(p, size_aligned, 0);
				return cast_data_header(p, data_header_offset);
			};
			void free(void* ptr)
			{
				assert(_pool_ptr > 0 && "pointer must not be 0");
			#if defined(USE_EXTERNAL_ALLOCATOR_ON_FAILURE)
				if (ptr < _pool_ptr || ptr >= cast_data_header(_pool_ptr, _pool_size)) { allocator::free(ptr); return; }
			#else
				assert(ptr > _pool_ptr && ptr < cast_data_header(_pool_ptr, _pool_size) && "ptr must be in the pool");
			#endif
				data_header *p = cast_data_header(ptr, -int(data_header_offset));
				
				assert((p->size & busy_bit) == busy_bit && "memory block must be busy");

				//далее предполагается, что слева и справа от data block
				//может быть только один unused block

				uint32_t unused = p->size & data_alignment_mask;
				//update memory_in_use
				_pool_ptr->memory_in_use -= unused;

				data_header *p_next = cast_data_header(p, unused), *p_prev;
				if ((p_next->size & busy_bit) == 0)
				{
					unlink_unused_block(p_next);
					unused += p_next->size & data_alignment_mask;
				}
				if ((p->size & prev_busy_bit) == 0)
				{
					p_prev = cast_data_header_32bit(p, p->prev);
					unlink_unused_block(p_prev);
					unused += p_prev->size & data_alignment_mask;
					p = p_prev;
				}
				p->size = unused | (p->size & prev_busy_bit);
				//link new unused block
				link_unused_block(p);
			};
			void* malloc_aligned(uint32_t size, uint32_t align)
			{
				if (align == 0 || (data_alignment % align) == 0)return malloc(size);
				assert((align & ~data_alignment_mask) == 0 && "align should be a multiple of data_alignment");

				//allocate free memory block with an additional minimum block size bytes
				void *ptr = malloc(size + data_header_bytes + align - 1);
			#if defined(USE_EXTERNAL_ALLOCATOR_ON_FAILURE)
				if (ptr == 0) { return allocator::malloc_aligned(size, align); }
			#else
				if (ptr == 0) { return 0; }
			#endif				
				//align ptr
				data_header *p = cast_data_header(ptr, -int(data_header_offset)), *p_new;			
				size_t addr = (size_t)ptr + data_header_bytes + align - 1; addr -= addr % align; ptr = (void*)addr;
				p_new = cast_data_header(ptr, -int(data_header_offset));
				p_new->size = offs_data_header_32bit(p_new, cast_data_header(p, p->size & data_alignment_mask)) << 2;
				p_new->size |= prev_busy_bit | busy_bit;
				
				//release leading free block back to the pool
				p->size -= p_new->size & data_alignment_mask;
				free(cast_data_header(p, data_header_offset));
				return ptr;
			}
			void* realloc(void* ptr, uint32_t size)
			{
			#if defined(USE_EXTERNAL_ALLOCATOR_ON_FAILURE)
				if (ptr < _pool_ptr || ptr >= cast_data_header(_pool_ptr, _pool_size)) { return allocator::realloc(ptr, size); }
			#else
				assert(ptr > _pool_ptr && ptr < cast_data_header(_pool_ptr, _pool_size) && "ptr must be in the pool");
			#endif
				//treated as malloc
				if (ptr == 0) { return malloc(size); }
				//treated as free
				if (size == 0) { free(ptr); return 0; }
				//realloc
				//alignment of size
				uint32_t required_size, cur_size, unused_left, unused_right, join_size;
				required_size = (size + data_header_size + data_alignment - 1) & data_alignment_mask;
				if (required_size < data_header_bytes) required_size = data_header_bytes;
				data_header *p = cast_data_header(ptr, -int(data_header_offset)), *p_right, *p_left;
				cur_size = p->size & data_alignment_mask;

				//there is no need to move the data block
				//test for current block capacity enough
				if (required_size <= cur_size)
				{
					//split unused block
					split_block(p, required_size, cur_size);
					return ptr;
				}
				//test the right data block for growth and if necessary, increase the size
				p_right = cast_data_header(p, cur_size);
				unused_right = (p_right->size & busy_bit) == busy_bit ? 0 : p_right->size & data_alignment_mask;
				join_size = cur_size + unused_right;
				if (join_size >= required_size)
				{
					unlink_unused_block(p_right);
					p->size += unused_right;
					//split unused block
					split_block(p, required_size, cur_size);
					return ptr;
				}
				//there is a need to move the data block
				//test the right data block for growth and if necessary, increase the size
				unused_left = (p->size & prev_busy_bit) == prev_busy_bit ? 0 : (-p->prev) << 2;
				join_size += unused_left;
				if (join_size >= required_size)
				{
					p_left = cast_data_header_32bit(p, p->prev);
					unlink_unused_block(p_left);
					if (unused_right) unlink_unused_block(p_right);
					p_left->size = join_size | prev_busy_bit | busy_bit;
					ptr = cast_data_header(p_left, data_header_offset);
					//copy data to new place
					::memcpy_s(ptr, required_size, cast_data_header(p, data_header_offset), cur_size - data_header_size);
					//split unused block
					split_block(p_left, required_size, cur_size);
					return ptr;
				}
				//force malloc new data block
				ptr = malloc(required_size);
			#if defined(USE_EXTERNAL_ALLOCATOR_ON_FAILURE)
				if (ptr == 0) { ptr = allocator::malloc(required_size); }
			#endif
				if (ptr == 0) { return 0; }
				//copy data to new place
				::memcpy_s(ptr, required_size, cast_data_header(p, data_header_offset), cur_size - data_header_size);
				//release free block back to the pool
				free(cast_data_header(p, data_header_offset));
				return ptr;
			}

		private:
			page_header* _pool_ptr;
			uint32_t     _pool_size;
		};


#undef data_alignment
#undef data_alignment_bits
#undef data_alignment_mask
#undef data_header_size
#undef data_header_bytes
#undef data_header_offset
#undef default_pool_size
#undef busy_bit
#undef prev_busy_bit
#undef fti_bits
#undef fti_first_max
#undef fti_table_n
#undef sti_table_n
#undef page_header_bytes
#undef cast_data_header
#undef cast_data_header_32bit
#undef offs_data_header_32bit
#undef split_block
#undef USE_EXTERNAL_ALLOCATOR_ON_FAILURE
#undef USE_FTI_FIELD_IN_HEADER

#if 0
		//firefox javascript scratchpad
		//for calculation fti_table_n
		var FTI = 5, align = 4, min_size = 16;
		function cl(s, k) { return Math.trunc(s*Math.pow(2, -k))*Math.pow(2, k); }
		var f = function(size)
		{
			if (size < min_size) size = min_size;
			var m = 0, i = 0, fti = Math.trunc(size / align), fti_first_max = 1 << (FTI + 1);
			if (fti >= fti_first_max)
			{
				i = Math.floor(Math.log2(fti)) - FTI;
				//fti = (i<<FTI) + (fti>>i);
				fti = Math.pow(2, FTI)*i + Math.trunc(Math.pow(2, -i)*fti);
			}
			return[cl(size / align, i)*align, (cl(size / align, i) + Math.pow(2, i))*align - 1, fti - min_size / align];
		}
		console.log(f(Math.pow(2, 32)));//2^32 =  4 GB, sli_table_n = 828
		console.log(f(Math.pow(2, 33)));//2^33 =  8 GB, sli_table_n = 860
		console.log(f(Math.pow(2, 34)));//2^34 = 16 GB, sli_table_n = 892
#endif















		void MurmurHash3_x86_32(const void* key, const uint32_t len, const uint32_t seed, uint32_t* out)
		{
			uint32_t i;
			uint32_t k1;
			uint32_t h1 = seed;
			uint32_t c1 = 0xCC9E2D51UL;
			uint32_t c2 = 0x1B873593UL;
			const uint32_t nblocks = len >> 2, *blocks = (const uint32_t*)key;

			for (i = nblocks; i; --i)
			{
				k1 = *blocks++; k1 *= c1; k1 = (k1 << 15) | (k1 >> (32 - 15)); k1 *= c2;
				h1 ^= k1; h1 = (h1 << 13) | (h1 >> (32 - 13)); h1 = h1 * 5 + 0xE6546B64UL;
			}
			k1 = 0;
			switch (len & 3)
			{
				case 3: k1 ^= blocks[2] << 16;
				case 2: k1 ^= blocks[1] << 8;
				case 1: k1 ^= blocks[0];
						k1 *= c1; k1 = (k1 << 15) | (k1 >> (32 - 15)); k1 *= c2; h1 ^= k1;
			};
			h1 ^= len;
			h1 ^= h1 >> 16; h1 *= 0x85EBCA6BUL; h1 ^= h1 >> 13; h1 *= 0xC2B2AE35UL; h1 ^= h1 >> 16;
			*out = h1;
		}

		void MurmurHash3_x86_128(const void* key, const uint32_t len, const uint32_t seed, uint32_t* out)
		{
			uint32_t i;
			uint32_t k1, k2, k3, k4;
			uint32_t h1 = seed, h2 = seed, h3 = seed, h4 = seed;
			uint32_t c1 = 0x239B961BUL;
			uint32_t c2 = 0xAB0E9789UL;
			uint32_t c3 = 0x38B34AE5UL;
			uint32_t c4 = 0xA1E38B93UL;
			const uint32_t nblocks = len >> 4, *blocks = (const uint32_t*)key;

			for (i = nblocks; i; --i)
			{
				k1 = *blocks++; k2 = *blocks++; k3 = *blocks++; k4 = *blocks++;
				k1 *= c1; k1 = (k1 << 15) | (k1 >> (32 - 15)); k1 *= c2; h1 ^= k1;
				h1 = (h1 << 19) | (h1 >> (32 - 19)); h1 += h2; h1 = h1 * 5 + 0x561CCD1BUL;
				k2 *= c2; k2 = (k2 << 16) | (k2 >> (32 - 16)); k2 *= c3; h2 ^= k2;
				h2 = (h2 << 17) | (h2 >> (32 - 17)); h2 += h3; h2 = h2 * 5 + 0x0BCAA747UL;
				k3 *= c3; k3 = (k3 << 17) | (k3 >> (32 - 17)); k3 *= c4; h3 ^= k3;
				h3 = (h3 << 15) | (h3 >> (32 - 15)); h3 += h4; h3 = h3 * 5 + 0x96CD1C35UL;
				k4 *= c4; k4 = (k4 << 18) | (k4 >> (32 - 18)); k4 *= c1; h4 ^= k4;
				h4 = (h4 << 13) | (h4 >> (32 - 13)); h4 += h1; h4 = h4 * 5 + 0x32AC3B17UL;
			}
			k1 = 0; k2 = 0; k3 = 0; k4 = 0;
			switch (len & 15)
			{
				case 15: k4 ^= blocks[14] << 16;
				case 14: k4 ^= blocks[13] << 8;
				case 13: k4 ^= blocks[12] << 0;
						 k4 *= c4; k4 = (k4 << 18) | (k4 >> (32 - 18)); k4 *= c1; h4 ^= k4;
				case 12: k3 ^= blocks[11] << 24;
				case 11: k3 ^= blocks[10] << 16;
				case 10: k3 ^= blocks[9] << 8;
				case  9: k3 ^= blocks[8] << 0;
						 k3 *= c3; k3 = (k3 << 17) | (k3 >> (32 - 17)); k3 *= c4; h3 ^= k3;
				case  8: k2 ^= blocks[7] << 24;
				case  7: k2 ^= blocks[6] << 16;
				case  6: k2 ^= blocks[5] << 8;
				case  5: k2 ^= blocks[4] << 0;
						 k2 *= c2; k2 = (k2 << 16) | (k2 >> (32 - 16)); k2 *= c3; h2 ^= k2;
				case  4: k1 ^= blocks[3] << 24;
				case  3: k1 ^= blocks[2] << 16;
				case  2: k1 ^= blocks[1] << 8;
				case  1: k1 ^= blocks[0] << 0;
						 k1 *= c1; k1 = (k1 << 15) | (k1 >> (32 - 15)); k1 *= c2; h1 ^= k1;
			};
			h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;
			h1 += h2; h1 += h3; h1 += h4;
			h2 += h1; h3 += h1; h4 += h1;
			h1 ^= h1 >> 16; h1 *= 0x85EBCA6BUL; h1 ^= h1 >> 13; h1 *= 0xC2B2AE35UL; h1 ^= h1 >> 16;
			h2 ^= h2 >> 16; h2 *= 0x85EBCA6BUL; h2 ^= h2 >> 13; h2 *= 0xC2B2AE35UL; h2 ^= h2 >> 16;
			h3 ^= h3 >> 16; h3 *= 0x85EBCA6BUL; h3 ^= h3 >> 13; h3 *= 0xC2B2AE35UL; h3 ^= h3 >> 16;
			h4 ^= h4 >> 16; h4 *= 0x85EBCA6BUL; h4 ^= h4 >> 13; h4 *= 0xC2B2AE35UL; h4 ^= h4 >> 16;
			h1 += h2; h1 += h3; h1 += h4;
			h2 += h1; h3 += h1; h4 += h1;
			out[0] = h1; out[1] = h2; out[2] = h3; out[3] = h4;
		}

		void MurmurHash3_x64_128(const void* key, const uint32_t len, const uint32_t seed, uint64_t* out)
		{
			uint32_t i;
			uint64_t k1, k2;
			uint64_t h1 = seed, h2 = seed;
			uint64_t c1 = 0x87C37B91114253D5ULL;
			uint64_t c2 = 0x4CF5AD432745937FULL;
			const uint32_t nblocks = len >> 4;
			const uint64_t *blocks = (const uint64_t*)key;

			for (i = nblocks; i; --i)
			{
				k1 = *blocks++; k2 = *blocks++;
				k1 *= c1; k1 = (k1 << 31) | (k1 >> (64 - 31)); k1 *= c2; h1 ^= k1;
				h1 = (h1 << 27) | (h1 >> (64 - 27)); h1 += h2; h1 = h1 * 5 + 0x52DCE729ULL;
				k2 *= c2; k2 = (k2 << 33) | (k2 >> (64 - 33)); k2 *= c1; h2 ^= k2;
				h2 = (h2 << 31) | (h2 >> (64 - 31)); h2 += h1; h2 = h2 * 5 + 0x38495AB5ULL;
			}
			k1 = 0; k2 = 0;
			switch (len & 15)
			{
				case 15: k2 ^= blocks[14] << 48;
				case 14: k2 ^= blocks[13] << 40;
				case 13: k2 ^= blocks[12] << 32;
				case 12: k2 ^= blocks[11] << 24;
				case 11: k2 ^= blocks[10] << 16;
				case 10: k2 ^= blocks[9] << 8;
				case  9: k2 ^= blocks[8] << 0;
						 k2 *= c2; k2 = (k2 << 33) | (k2 >> (64 - 33)); k2 *= c1; h2 ^= k2;
				case  8: k1 ^= blocks[7] << 56;
				case  7: k1 ^= blocks[6] << 48;
				case  6: k1 ^= blocks[5] << 40;
				case  5: k1 ^= blocks[4] << 32;
				case  4: k1 ^= blocks[3] << 24;
				case  3: k1 ^= blocks[2] << 16;
				case  2: k1 ^= blocks[1] << 8;
				case  1: k1 ^= blocks[0] << 0;
						 k1 *= c1; k1 = (k1 << 31) | (k1 >> (64 - 31)); k1 *= c2; h1 ^= k1;
			};
			h1 ^= len; h2 ^= len;
			h1 += h2; h2 += h1;
			h1 ^= h1 >> 33; h1 *= 0xFF51AFD7ED558CCDULL; h1 ^= h1 >> 33; h1 *= 0xC4CEB9FE1A85EC53ULL; h1 ^= h1 >> 33;
			h2 ^= h2 >> 33; h2 *= 0xFF51AFD7ED558CCDULL; h2 ^= h2 >> 33; h2 *= 0xC4CEB9FE1A85EC53ULL; h2 ^= h2 >> 33;
			h1 += h2; h2 += h1;
			out[0] = h1; out[1] = h2;
		}

	}//end namespace memory
}//end namespace LZ - lazy
