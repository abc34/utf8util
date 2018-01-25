#pragma once
#include <stddef.h>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <intrin.h>


namespace utf8util
{
	//sources:
	//https://en.wikipedia.org/wiki/UTF-8
	//https://github.com/voku/portable-utf8/tree/master/tests
	//https://github.com/JuliaLang/utf8proc/blob/master/utf8proc.c
	//https://github.com/ww898/utf-cpp/blob/master/test/utf_converters_test.cpp
	//https://github.com/tchwork/utf8/blob/master/tests/Utf8/Utf8StrcasecmpTest.php
	//https://github.com/svaarala/duktape/blob/master/src-input/duk_unicode_support.c
	//
	//other info
	//ByteOrderMark:     0xFEFF or bytes:0xEF,0xBB,0xBF
	//NotAChar:          0xFFFF or bytes:0xEF,0xBF,0xBF
	//ByteOrderNotAChar: 0xFFFE ot bytes:0xEF,0xBF,0xBE

	//Порядок байтов: https://ru.wikipedia.org/wiki/%D0%AE%D0%BD%D0%B8%D0%BA%D0%BE%D0%B4
	//UTF8:     0xEF,0xBB,0xBF (0xFEFF)
	//UTF16-BE: 0xFEFF (обычно)
	//UTF16-LE: 0xFFFE (когда нужна перестановка байт)
	//UTF32-BE: 0x00,0x00,0xFE,0xFF
	//UTF32-LE: 0xFF,0xFE,0x00,0x00

	//UTF8
	// codepoint ranges    coded bytes                           coded bytes ranges
	// U+0000...U+007F     0xxxxxxx                              (00..7F)
	// U+0080...U+07FF     110xxxxx 10xxxxxx                     (C0..DF) (00..7F)
	// U+0800...U+FFFF     1110xxxx 10xxxxxx 10xxxxxx            (E0..EF) (00..7F) (00..7F)
	//U+10000...U+10FFFF   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx   (F0..F7) (00..7F) (00..7F) (00..7F)

	//error codes:
#define RESULT_OK                     0
#define ERROR_SRC_LENGTH              1
#define ERROR_DST_LENGTH              2
#define ERROR_UTF8_DECODE_RANGE       3
#define ERROR_UTF8_ENCODE_RANGE       4
#define ERROR_UTF8_CONTINUATION_BYTE  5
#define ERROR_UTF16_DECODE_RANGE      6
#define ERROR_UTF16_ENCODE_RANGE      7
#define ERROR_UTF16_CONTINUATION_WORD 8
#define ERROR_UTF32_DECODE_RANGE      9
#define ERROR_UTF32_ENCODE_RANGE      10
//replacement_char -> "�" 0xFFFD utf8 -> [EF,BF,BD] or U+FFFF utf8->[EF BF BF]
#define R_CHAR        0xFFFD
#define R_CHAR_UTF8_0 0xEF
#define R_CHAR_UTF8_1 0xBF
#define R_CHAR_UTF8_2 0xBD
//options on invalid chars
#define OPT_REPLACE_INVALID   0
#define OPT_SKIP_INVALID      1
#define OPT_BREAKON_INVALID   2

	uint16_t utf8proc_sequences, utf8_stage1table, utf8_stage2table;
	struct validation_info
	{
		uint8_t status;
		uint32_t nerrors;
		uint32_t length8;
		uint32_t length16;
		uint32_t length32;
		void* src_next;
	};

	//counting the number uint16 nedeed to convert utf8 to utf16
	//src_length - src length, uint8_t elements
	//option     - OPT_*
	uint8_t utf8_validation(uint8_t* src, uint32_t src_length, uint8_t option, validation_info* vi)
	{
		uint8_t ret = RESULT_OK;
		uint32_t ncount16 = 0, ncount32 = 0, nerrors = 0, src_len = src_length;
		while (src_length > 0)
		{
			uint8_t c = *src++;
			if (c < 0x80) { src_length -= 1; goto on_ok; }
			else if (c < 0xC2 || c > 0xF4) { goto on_error_range; }
			else if (c < 0xE0)
			{
				if (src_length < 2) { goto on_error_length; }
				c = *src++;if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; }
				src_length -= 2;
				goto on_ok;
			}
			else if (c < 0xF0)
			{
				if (src_length < 3) { goto on_error_length; }
				if (c == 0xE0) { c = *src++; if (c < 0xA0 || c >= 0xC0) { src_length -= 1; goto on_error_continuation; } }
				else { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
				c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
				src_length -= 3;
				goto on_ok;
			}
			else if (c <= 0xF4)
			{
				if (src_length < 4) { goto on_error_length; }
				if (c == 0xF0) { c = *src++; if (c < 0x90 || c >= 0xC0) { src_length -= 1; goto on_error_continuation; } }
				else if (c < 0xF4) { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
				else { c = *src++; if (c < 0x80 || c >= 0x90) { src_length -= 1; goto on_error_continuation; } }
				c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
				c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 3; goto on_error_continuation; }
				src_length -= 4; ncount16++;
				goto on_ok;
			}
		on_error_range:
			src_length -= 1;
			nerrors++;
			if (option == OPT_BREAKON_INVALID) { ret = ERROR_UTF8_DECODE_RANGE; ncount32++; break; }
			goto on_ok;
		on_error_length:
			src_length -= 1;
			nerrors++;
			if (option == OPT_BREAKON_INVALID) { ret = ERROR_SRC_LENGTH; ncount32++; break; }
			goto on_ok;
		on_error_continuation:
			nerrors++;
			src--;
			if (option == OPT_BREAKON_INVALID) { ret = ERROR_UTF8_CONTINUATION_BYTE; ncount32++; break; }
		on_ok:
			ncount32++;
		}
		if (vi != nullptr)
		{
			ncount16 += ncount32;
			if (option == OPT_SKIP_INVALID) { ncount16 -= nerrors; ncount32 -= nerrors; }
			vi->length8 = src_len;
			vi->length16 = ncount16;
			vi->length32 = ncount32;
			vi->nerrors = nerrors;
			vi->src_next = src;
			vi->status = ret;
		}
		return ret;
	}
	//counting the number uint16 nedeed to convert utf8 to utf16
	//src_length - src length, uint8_t elements
	//option     - OPTION_SKIP_INVALID or OPTION_REPLACE_INVALID
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
				if (c == 0xE0) { c = *src++; if (c < 0xA0 || c >= 0xC0) { src_length -= 1; goto on_error_continuation; } }
				else { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
				c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
				src_length -= 3;
				goto on_ok;
			}
			else if (c <= 0xF4)
			{
				if (src_length < 4) { src_length -= 1; goto on_error_range; }
				if (c == 0xF0) { c = *src++; if (c < 0x90 || c >= 0xC0) { src_length -= 1; goto on_error_continuation; } }
				else if (c < 0xF4) { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
				else { c = *src++; if (c < 0x80 || c >= 0x90) { src_length -= 1; goto on_error_continuation; } }
				c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
				c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 3; goto on_error_continuation; }
				src_length -= 4; ncount++;
				goto on_ok;
			}
		on_error_continuation:
			src--;
		on_error_range:
			nerrors++;
		on_ok:
			ncount++;
		}
		if (option == OPT_SKIP_INVALID) ncount -= nerrors;
		return ncount;
	}
	//counting the number uint8 nedeed to convert utf16 to utf8
	//src_length - src length, uint16_t elements
	//option     - OPTION_SKIP_INVALID or OPTION_REPLACE_INVALID
	uint32_t utf16to8_length(uint16_t* src, uint32_t src_length, uint8_t option)
	{
		uint16_t c, *src_end = src + src_length;
		uint32_t ncount = 0;
		while (src < src_end)
		{
			c = *src++;
			if (c < 0x0080) ncount++;
			else if (c < 0x0800) ncount += 2;
			else if (c < 0xD800 || c>0xDFFF) ncount += 3;
			else if (c < 0xDC00)
			{
				if (src == src_end || src[0] < 0xDC00 || src[0] > 0xDFFF) { if (option == OPT_REPLACE_INVALID) ncount += 3; }
				else { ncount += 4;src++; }
			}
			else if (option == OPT_REPLACE_INVALID) ncount += 3;
		}
		return ncount;
	}
	//counting the number uint32 nedeed to convert utf8 to utf32
	//src_length - src length, uint8_t elements
	//option     - OPTION_SKIP_INVALID or OPTION_REPLACE_INVALID
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
				if (c == 0xE0) { c = *src++; if (c < 0xA0 || c >= 0xC0) { src_length -= 1; goto on_error_continuation; } }
				else { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
				c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
				src_length -= 3;
				goto on_ok;
			}
			else if (c <= 0xF4)
			{
				if (src_length < 4) { src_length -= 1; goto on_error_range; }
				if (c == 0xF0) { c = *src++; if (c < 0x90 || c >= 0xC0) { src_length -= 1; goto on_error_continuation; } }
				else if (c < 0xF4) { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } }
				else { c = *src++; if (c < 0x80 || c >= 0x90) { src_length -= 1; goto on_error_continuation; } }
				c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; }
				c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 3; goto on_error_continuation; }
				src_length -= 4;
				goto on_ok;
			}
		on_error_continuation:
			src--;
		on_error_range:
			nerrors++;
		on_ok:
			ncount++;
		}
		if (option == OPT_SKIP_INVALID) ncount -= nerrors;
		return ncount;
	}
	//counting the number uint8 nedeed to convert utf32 to utf8
	//src_length - src length, uint32_t elements
	uint32_t utf32to8_length(uint32_t* src, uint32_t src_length, uint8_t option)
	{
		uint32_t ncount = 0;
		uint32_t c, *src_end = src + src_length;
		while (src < src_end)
		{
			c = *src++;
			if (c < 0x80) ncount++;
			else if (c < 0x0800) ncount += 2;
			else if (c < 0x10000) ncount += 3;
			else if (c < 0x110000) ncount += 4;
			else if (option == OPT_REPLACE_INVALID) ncount += 3;
		}
		return ncount;
	}
	//cp = |00..7F|80..7FF|800..FFFF|10000..10FFFF|
#define UTF8_DECODE(cp, src, src_length, option) \
		uint8_t c = *src++; \
		if (c < 0x80) { cp = static_cast<uint32_t>(c); src_length--; goto on_ok;} \
		else if (c < 0xC2 || c > 0xF4) { goto on_error_range;} \
		else if (c < 0xE0) \
		{ \
			if (src_length < 2) { goto on_error_length; } \
			cp = static_cast<uint32_t>(c & 0x1F) << 6; \
			c = *src++;if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } \
			cp |= static_cast<uint32_t>(c & 0x3F); \
			src_length -= 2; \
			goto on_ok; \
		} \
		else if (c < 0xF0) \
		{ \
			if (src_length < 3) { goto on_error_length; } \
			cp = static_cast<uint32_t>(c & 0x0F) << 12; \
			if( c == 0xE0) { c = *src++; if (c < 0xA0 || c >= 0xC0) { src_length -= 1; goto on_error_continuation; } } \
			else { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } } \
			cp |= static_cast<uint32_t>(c & 0x3F) << 6; \
			c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; } \
			cp |= static_cast<uint32_t>(c & 0x3F); \
			src_length -= 3; \
			goto on_ok; \
		} \
		else if (c <= 0xF4) \
		{ \
			if (src_length < 4) { goto on_error_length; } \
			cp = static_cast<uint32_t>(c & 0x07) << 18; \
			if(c == 0xF0) { c = *src++; if (c < 0x90 || c >= 0xC0) { src_length -= 1; goto on_error_continuation; } } \
			else if(c < 0xF4) { c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 1; goto on_error_continuation; } } \
			else { c = *src++; if (c < 0x80 || c >= 0x90) { src_length -= 1; goto on_error_continuation; } } \
			cp |= static_cast<uint32_t>(c & 0x3F) << 12; \
			c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 2; goto on_error_continuation; } \
			cp |= static_cast<uint32_t>(c & 0x3F) << 6; \
			c = *src++; if ((c & 0xC0) != 0x80) { src_length -= 3; goto on_error_continuation; } \
			cp |= static_cast<uint32_t>(c & 0x3F); \
			src_length -= 4; \
			goto on_ok; \
		} \
on_error_range: \
		if (option == OPT_BREAKON_INVALID) { return ERROR_UTF8_DECODE_RANGE; } src_length--; if(option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; goto on_ok; \
on_error_length: \
		if (option == OPT_BREAKON_INVALID) { return ERROR_SRC_LENGTH; } src_length--; if(option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; goto on_ok; \
on_error_continuation: \
		if (option == OPT_BREAKON_INVALID) { return ERROR_UTF8_CONTINUATION_BYTE; } src--; if(option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; \
on_ok:

#define UTF8_ENCODE(cp, dst, dst_length, option) \
	if (cp < 0x80) \
	{ \
		if (dst_length == 0) { return ERROR_DST_LENGTH; } \
		*dst++ = static_cast<uint8_t>(cp); dst_length--; \
	} \
	else if (cp < 0x800) \
	{ \
		if (dst_length < 2) { return ERROR_DST_LENGTH; } \
		*dst++ = static_cast<uint8_t>(0xC0 | (cp >> 6)); \
		*dst++ = static_cast<uint8_t>(0x80 | (cp & 0x3F)); \
		dst_length -= 2; \
	} \
	else if (cp < 0x10000) \
	{ \
		if (dst_length < 3) { return ERROR_DST_LENGTH; } \
		*dst++ = static_cast<uint8_t>(0xE0 | (cp >> 12)); \
		*dst++ = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F)); \
		*dst++ = static_cast<uint8_t>(0x80 | (cp & 0x3F)); \
		dst_length -= 3; \
	} \
	else if (cp < 0x110000) \
	{ \
		if (dst_length < 4) { return ERROR_DST_LENGTH; } \
		*dst++ = static_cast<uint8_t>(0xF0 | (cp >> 18)); \
		*dst++ = static_cast<uint8_t>(0x80 | ((cp >> 12) & 0x3F)); \
		*dst++ = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F)); \
		*dst++ = static_cast<uint8_t>(0x80 | (cp & 0x3F)); \
		dst_length -= 4; \
	} \
	else \
	{ \
		/*on error emit replacement codepoint U+FFFD utf8->[EF BF BD] or U+FFFF utf8->[EF BF BF]*/ \
		if (option == OPT_BREAKON_INVALID) { return ERROR_UTF8_ENCODE_RANGE; } \
		if (option == OPT_SKIP_INVALID) { continue; } \
		if (dst_length < 3) { return ERROR_DST_LENGTH; } \
		*dst++ = R_CHAR_UTF8_0; \
		*dst++ = R_CHAR_UTF8_1; \
		*dst++ = R_CHAR_UTF8_2; \
		dst_length -= 3; \
	}

#define UTF16_DECODE(cp, src, src_length, option) \
	uint16_t c = *src++; \
	/*if (need_swap) c = static_cast<uint16_t>(((c & 0xFF) << 8) | (c >> 8));*/ \
	if (c < 0xD800 || c >= 0xDC00) { cp = static_cast<uint32_t>(c); src_length--; goto on_ok_16; } \
	else if (c < 0xDC00) \
	{ \
		if (src_length < 2) { goto on_error_length_16; }\
		/*codepoint in ranges U+D800..U+DBFF and U+DC00..U+DFFF is reserved for utf16 surrogate pairs*/ \
		cp = static_cast<uint32_t>(c & 0x3FF) << 10; \
		c = *src++; if (c < 0xDC00 || c > 0xDFFF) { src_length -= 1; goto on_error_continuation_16; } \
		cp |= static_cast<uint32_t>(c & 0x3FF); \
		cp += 0x10000; \
		src_length -= 2; \
		goto on_ok_16; \
	} \
/*on_error_range:*/ \
	if (option == OPT_BREAKON_INVALID) { return ERROR_UTF16_DECODE_RANGE; } src_length--; if(option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; goto on_ok_16; \
on_error_length_16: \
	if (option == OPT_BREAKON_INVALID) { return ERROR_SRC_LENGTH; } src_length = 0; if(option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; goto on_ok_16; \
on_error_continuation_16: \
	if (option == OPT_BREAKON_INVALID) { return ERROR_UTF16_CONTINUATION_WORD; } src--; if(option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; \
on_ok_16:

#define UTF16_ENCODE(cp, dst, dst_length, option) \
	if (dst_length == 0) { return ERROR_DST_LENGTH; } \
	if (cp < 0xD800) { *dst++ = static_cast<uint16_t>(cp); dst_length--; } \
	else if (cp < 0xDC00) \
	{ \
		/*codepoint in ranges U+D800..U+DBFF and U+DC00..U+DFFF is reserved for utf16 surrogate pairs */ \
		if (option == OPT_BREAKON_INVALID) { return ERROR_UTF16_ENCODE_RANGE; } \
		if (option == OPT_SKIP_INVALID) { continue; } \
		*dst++ = R_CHAR; dst_length--; \
	} \
	else if (cp < 0x10000) \
	{ \
		*dst++ = static_cast<uint16_t>(cp); dst_length--; \
	} \
	else if (cp < 0x110000) \
	{ \
		if(dst_length < 2) { return ERROR_DST_LENGTH; } \
		/*U+10000..U+10FFFF*/ \
		cp -= 0x10000; \
		*dst++ = static_cast<uint16_t>(0xD800 | (cp >> 10));   /*D800..DBFF*/ \
		*dst++ = static_cast<uint16_t>(0xDC00 | (cp & 0x3FF)); /*DC00..DFFF*/ \
		dst_length -= 2; \
	} \
	else \
	{ \
		/*on error emit replacement codepoint*/ \
		if (option == OPT_BREAKON_INVALID) { return ERROR_UTF16_ENCODE_RANGE; } \
		if (option == OPT_SKIP_INVALID) { continue; } \
		*dst++ = R_CHAR; dst_length--; \
	}

#define UTF32_DECODE(cp, src, src_length, option) \
	cp = *src++; src_length--; \
	/*if (need_swap) cp = ((cp & 0xFF) << 24) | ((cp & 0xFF00) << 8) | ((cp & 0xFF0000) >> 8) | (cp >> 24);*/ \
	if (cp >= 0x110000) { if (option == OPT_BREAKON_INVALID) { return ERROR_UTF32_DECODE_RANGE; } if (option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; }

#define UTF32_ENCODE(cp, dst, dst_length, option) \
	if (dst_length == 0) { return ERROR_DST_LENGTH; } \
	if (cp >= 0x110000) { if (option == OPT_BREAKON_INVALID) { return ERROR_UTF32_ENCODE_RANGE; } if (option == OPT_SKIP_INVALID) { continue; } cp = R_CHAR; } \
	*dst++ = cp; dst_length--;

	//convert utf8 string to utf32 string
	//src_length - src string length, uint8_t chars
	//dst_length - dst string length, uint32_t chars
	uint8_t utf8to32(uint8_t* src, uint32_t src_length, uint32_t* dst, uint32_t dst_length, uint8_t option)
	{
		uint32_t cp;
		//test BOM: 0xEF,0xBB,0xBF and skip it
		//if (src_length > 2 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) { src += 3; src_length -= 3; }
		while (src_length > 0)
		{
			//UTF8_DECODE(cp, src, src_length, need_replace);
			UTF8_DECODE(cp, src, src_length, option);
			UTF32_ENCODE(cp, dst, dst_length, OPT_SKIP_INVALID);
		}
		return RESULT_OK;
	}
	//convert utf32 string to utf8 string
	//src_length - src string length, uint32_t chars
	//dst_length - dst string length, uint8_t chars
	uint8_t utf32to8(uint32_t* src, uint32_t src_length, uint8_t* dst, uint32_t dst_length, uint8_t option)
	{
		uint32_t cp;
		//test BOM: 0x0000FEFF and skip it
		//if (src_length > 0 && src[0] == 0x0000FEFF) { src++; src_length--; }
		while (src_length > 0)
		{
			UTF32_DECODE(cp, src, src_length, option);
			UTF8_ENCODE(cp, dst, dst_length, OPT_SKIP_INVALID);
		}
		return RESULT_OK;
	}
	//convert utf8 string to utf16 string
	//src_length - src string length, uint8_t chars
	//dst_length - dst string length, uint16_t chars
	uint8_t utf8to16(uint8_t* src, uint32_t src_length, uint16_t* dst, uint32_t dst_length, uint8_t option)
	{
		uint32_t cp;
		//test BOM: 0xEF,0xBB,0xBF and skip it
		//if (src_length > 2 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) { src += 3; src_length -= 3; }
		while (src_length > 0)
		{
			UTF8_DECODE(cp, src, src_length, option);
			UTF16_ENCODE(cp, dst, dst_length, OPT_SKIP_INVALID);
		}
		return RESULT_OK;
	}
	//convert utf16 string to utf8 string
	//src_length - src string length, uint16_t chars
	//dst_length - dst string length, uint8_t chars
	uint8_t utf16to8(uint16_t* src, uint32_t src_length, uint8_t* dst, uint32_t dst_length, uint8_t option)
	{
		uint32_t cp;
		//test BOM: 0xFEFF and skip it
		//if (src_length > 0 && src[0] == 0xFEFF) { src++; src_length--; }
		while (src_length > 0)
		{
			UTF16_DECODE(cp, src, src_length, option);
			UTF8_ENCODE(cp, dst, dst_length, OPT_SKIP_INVALID);
		}
		return RESULT_OK;
	}

	uint16_t utf16toupper(uint32_t cp)
	{
		//utf8proc_get_property(uc)
		//uc < 0 || uc >= 0x110000 ? utf8proc_properties : unsafe_get_property(uc)

		//unsafe_get_property(uc)
		//return utf8proc_properties + (
		//	utf8proc_stage2table[
		//		utf8proc_stage1table[uc >> 8] + (uc & 0xFF)
		//	]);
		//utf8proc_int32_t cu = utf8proc_get_property(c)->uppercase_seqindex;
		//return cu != UINT16_MAX ? seqindex_decode_index(cu) : c;
		return 0;
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







}



namespace memory
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
	//#define get_first_unused_block_by_size(_size) [this](register int64_t size) \
	//		{\
//			register union { double d; int64_t i; }v = { (double)size }; \
//			v.i = (v.i >> 52) - 1026; if (v.i < 0) v.i = 0; \
//			return &this->_pool_ptr->unused_ptr[v.i]; \
//		}(_size)

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
		page_header * _pool_ptr;
		size_t       _pool_size;
	};
}//end namespace memory
