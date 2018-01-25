// Utf8_lib.cpp : Defines the entry point for the console application.
//

#include <stdint.h>
#include "utf8util.h"

struct unicode_tuple final
{
	std::string utf8;
	std::wstring utf16;
	std::u32string utf32;
};

unicode_tuple const unicode_test_data[] =
{
	{
		"",
		L"",
{}
	}, // empty string

	{
		"\x24",
		L"\x0024",
{
	0x00000024
}
	}, // $

	{
		"\xC2\xA2",
		L"\x00A2",
{
	0x000000A2
}
	}, // ¢

	{
		"\xE2\x82\xAC",
		L"\x20AC",
{
	0x000020AC
}
	}, // €

	{
		"\xF0\x90\x8D\x88",
		L"\xD800\xDF48",
{
	0x00010348
}
	}, // 𐍈

	{
		"\xF0\xA4\xAD\xA2",
		L"\xD852\xDF62",
{
	0x00024B62
}
	}, // 𤭢

	{
		"\xF0\x90\x90\xB7",
		L"\xD801\xDC37",
{
	0x00010437
}
	}, // 𐐷

	{
		"\xEF\xAB\x82",
		L"\xFAC2",
{
	0x0000FAC2
}
	}, // 輸

	{
		"\xD0\xAE\xD0\xBD\xD0\xB8\xD0\xBA\xD0\xBE\xD0\xB4",
		L"\x042E\x043D\x0438\x043A\x043E\x0434",
{
	0x0000042E, 0x0000043D, 0x00000438, 0x0000043A, 0x0000043E, 0x00000434
}
	}, // Юникод

	{
		"\xC5\xAA\x6E\xC4\xAD\x63\xC5\x8D\x64\x65\xCC\xBD",
		L"\x016A\x006E\x012D\x0063\x014D\x0064\x0065\x033D",
{
	0x0000016A, 0x0000006E, 0x0000012D, 0x00000063, 0x0000014D, 0x00000064, 0x00000065, 0x0000033D
}
	}, // Ūnĭcōde̽

	{
		"\xE0\xA4\xAF\xE0\xA5\x82\xE0\xA4\xA8\xE0\xA4\xBF\xE0\xA4\x95\xE0\xA5\x8B\xE0\xA4\xA1",
		L"\x092F\x0942\x0928\x093F\x0915\x094B\x0921",
{
	0x0000092F, 0x00000942, 0x00000928, 0x0000093F, 0x00000915, 0x0000094B, 0x00000921
}
	}, // यूनिकोड

	{
		"\x41\xE2\x89\xA2\xCE\x91\x2E",
		L"\x0041\x2262\x0391\x002E",
{
	0x00000041, 0x00002262, 0x00000391, 0x0000002E
}
	}, // A≢Α.

	{
		"\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4",
		L"\xD55C\xAD6D\xC5B4",
{
	0x0000D55C, 0x0000AD6D, 0x0000C5B4
}
	}, // 한국어

	{
		"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E",
		L"\x65E5\x672C\x8A9E",
{
	0x000065E5, 0x0000672C, 0x00008A9E
}
	}, // 日本語

	{
		"\xE1\x9B\x81\xE1\x9A\xB3\xE1\x9B\xAB\xE1\x9B\x97\xE1\x9A\xA8\xE1\x9A\xB7\xE1\x9B\xAB\xE1\x9A\xB7\xE1\x9B\x9A\xE1\x9A\xA8\xE1\x9B\x8B\xE1\x9B"
		"\xAB\xE1\x9B\x96\xE1\x9A\xA9\xE1\x9B\x8F\xE1\x9A\xAA\xE1\x9A\xBE\xE1\x9B\xAB\xE1\x9A\xA9\xE1\x9A\xBE\xE1\x9B\x9E\xE1\x9B\xAB\xE1\x9A\xBB\xE1"
	"\x9B\x81\xE1\x9B\x8F\xE1\x9B\xAB\xE1\x9A\xBE\xE1\x9B\x96\xE1\x9B\xAB\xE1\x9A\xBB\xE1\x9B\x96\xE1\x9A\xAA\xE1\x9A\xB1\xE1\x9B\x97\xE1\x9B\x81"
	"\xE1\x9A\xAA\xE1\x9A\xA7\xE1\x9B\xAB\xE1\x9B\x97\xE1\x9B\x96\xE1\x9B\xAC",
	L"\x16C1\x16B3\x16EB\x16D7\x16A8\x16B7\x16EB\x16B7\x16DA\x16A8\x16CB\x16EB\x16D6\x16A9\x16CF\x16AA\x16BE\x16EB\x16A9\x16BE\x16DE\x16EB\x16BB"
	L"\x16C1\x16CF\x16EB\x16BE\x16D6\x16EB\x16BB\x16D6\x16AA\x16B1\x16D7\x16C1\x16AA\x16A7\x16EB\x16D7\x16D6\x16EC",
{
	0x000016C1, 0x000016B3, 0x000016EB, 0x000016D7, 0x000016A8, 0x000016B7, 0x000016EB, 0x000016B7, 0x000016DA, 0x000016A8, 0x000016CB, 0x000016EB,
	0x000016D6, 0x000016A9, 0x000016CF, 0x000016AA, 0x000016BE, 0x000016EB, 0x000016A9, 0x000016BE, 0x000016DE, 0x000016EB, 0x000016BB, 0x000016C1,
	0x000016CF, 0x000016EB, 0x000016BE, 0x000016D6, 0x000016EB, 0x000016BB, 0x000016D6, 0x000016AA, 0x000016B1, 0x000016D7, 0x000016C1, 0x000016AA,
	0x000016A7, 0x000016EB, 0x000016D7, 0x000016D6, 0x000016EC
}
	}, // ᛁᚳ᛫ᛗᚨᚷ᛫ᚷᛚᚨᛋ᛫ᛖᚩᛏᚪᚾ᛫ᚩᚾᛞ᛫ᚻᛁᛏ᛫ᚾᛖ᛫ᚻᛖᚪᚱᛗᛁᚪᚧ᛫ᛗᛖ᛬

	{
		"\xE1\x9A\x9B\xE1\x9A\x9B\xE1\x9A\x89\xE1\x9A\x91\xE1\x9A\x85\xE1\x9A\x94\xE1\x9A\x89\xE1\x9A\x89\xE1\x9A\x94\xE1\x9A\x8B\xE1\x9A\x80\xE1\x9A"
		"\x94\xE1\x9A\x88\xE1\x9A\x94\xE1\x9A\x80\xE1\x9A\x8D\xE1\x9A\x82\xE1\x9A\x90\xE1\x9A\x85\xE1\x9A\x91\xE1\x9A\x80\xE1\x9A\x85\xE1\x9A\x94\xE1"
	"\x9A\x8B\xE1\x9A\x8C\xE1\x9A\x93\xE1\x9A\x85\xE1\x9A\x90\xE1\x9A\x9C",
	L"\x169B\x169B\x1689\x1691\x1685\x1694\x1689\x1689\x1694\x168B\x1680\x1694\x1688\x1694\x1680\x168D\x1682\x1690\x1685\x1691\x1680\x1685\x1694"
	L"\x168B\x168C\x1693\x1685\x1690\x169C",
{
	0x0000169B, 0x0000169B, 0x00001689, 0x00001691, 0x00001685, 0x00001694, 0x00001689, 0x00001689, 0x00001694, 0x0000168B, 0x00001680, 0x00001694,
	0x00001688, 0x00001694, 0x00001680, 0x0000168D, 0x00001682, 0x00001690, 0x00001685, 0x00001691, 0x00001680, 0x00001685, 0x00001694, 0x0000168B,
	0x0000168C, 0x00001693, 0x00001685, 0x00001690, 0x0000169C
}
	}, // ᚛᚛ᚉᚑᚅᚔᚉᚉᚔᚋ ᚔᚈᚔ ᚍᚂᚐᚅᚑ ᚅᚔᚋᚌᚓᚅᚐ᚜

	{
		"\xE2\xA0\x8A\xE2\xA0\x80\xE2\xA0\x89\xE2\xA0\x81\xE2\xA0\x9D\xE2\xA0\x80\xE2\xA0\x91\xE2\xA0\x81\xE2\xA0\x9E\xE2\xA0\x80\xE2\xA0\x9B\xE2\xA0"
		"\x87\xE2\xA0\x81\xE2\xA0\x8E\xE2\xA0\x8E\xE2\xA0\x80\xE2\xA0\x81\xE2\xA0\x9D\xE2\xA0\x99\xE2\xA0\x80\xE2\xA0\x8A\xE2\xA0\x9E\xE2\xA0\x80\xE2"
	"\xA0\x99\xE2\xA0\x95\xE2\xA0\x91\xE2\xA0\x8E\xE2\xA0\x9D\xE2\xA0\x9E\xE2\xA0\x80\xE2\xA0\x93\xE2\xA0\xA5\xE2\xA0\x97\xE2\xA0\x9E\xE2\xA0\x80"
	"\xE2\xA0\x8D\xE2\xA0\x91",
	L"\x280A\x2800\x2809\x2801\x281D\x2800\x2811\x2801\x281E\x2800\x281B\x2807\x2801\x280E\x280E\x2800\x2801\x281D\x2819\x2800\x280A\x281E\x2800"
	L"\x2819\x2815\x2811\x280E\x281D\x281E\x2800\x2813\x2825\x2817\x281E\x2800\x280D\x2811",
{
	0x0000280A, 0x00002800, 0x00002809, 0x00002801, 0x0000281D, 0x00002800, 0x00002811, 0x00002801, 0x0000281E, 0x00002800, 0x0000281B, 0x00002807,
	0x00002801, 0x0000280E, 0x0000280E, 0x00002800, 0x00002801, 0x0000281D, 0x00002819, 0x00002800, 0x0000280A, 0x0000281E, 0x00002800, 0x00002819,
	0x00002815, 0x00002811, 0x0000280E, 0x0000281D, 0x0000281E, 0x00002800, 0x00002813, 0x00002825, 0x00002817, 0x0000281E, 0x00002800, 0x0000280D,
	0x00002811
}
	}, // ⠊⠀⠉⠁⠝⠀⠑⠁⠞⠀⠛⠇⠁⠎⠎⠀⠁⠝⠙⠀⠊⠞⠀⠙⠕⠑⠎⠝⠞⠀⠓⠥⠗⠞⠀⠍⠑

	{
		"\xD8\xA3\xD9\x86\xD8\xA7\x20\xD9\x82\xD8\xA7\xD8\xAF\xD8\xB1\x20\xD8\xB9\xD9\x84\xD9\x89\x20\xD8\xA3\xD9\x83\xD9\x84\x20\xD8\xA7\xD9\x84\xD8"
		"\xB2\xD8\xAC\xD8\xA7\xD8\xAC\x20\xD9\x88\x20\xD9\x87\xD8\xB0\xD8\xA7\x20\xD9\x84\xD8\xA7\x20\xD9\x8A\xD8\xA4\xD9\x84\xD9\x85\xD9\x86\xD9\x8A"
	"\x2E",
	L"\x0623\x0646\x0627\x0020\x0642\x0627\x062F\x0631\x0020\x0639\x0644\x0649\x0020\x0623\x0643\x0644\x0020\x0627\x0644\x0632\x062C\x0627\x062C"
	L"\x0020\x0648\x0020\x0647\x0630\x0627\x0020\x0644\x0627\x0020\x064A\x0624\x0644\x0645\x0646\x064A\x002E",
{
	0x00000623, 0x00000646, 0x00000627, 0x00000020, 0x00000642, 0x00000627, 0x0000062F, 0x00000631, 0x00000020, 0x00000639, 0x00000644, 0x00000649,
	0x00000020, 0x00000623, 0x00000643, 0x00000644, 0x00000020, 0x00000627, 0x00000644, 0x00000632, 0x0000062C, 0x00000627, 0x0000062C, 0x00000020,
	0x00000648, 0x00000020, 0x00000647, 0x00000630, 0x00000627, 0x00000020, 0x00000644, 0x00000627, 0x00000020, 0x0000064A, 0x00000624, 0x00000644,
	0x00000645, 0x00000646, 0x0000064A, 0x0000002E
}
	}, // أنا قادر على أكل الزجاج و هذا لا يؤلمني.

	{
		"\xE1\x80\x80\xE1\x80\xB9\xE1\x80\x9A\xE1\x80\xB9\xE1\x80\x9D\xE1\x80\x94\xE1\x80\xB9\xE2\x80\x8C\xE1\x80\x90\xE1\x80\xB1\xE1\x80\xAC\xE1\x80"
		"\xB9\xE2\x80\x8C\xE1\x81\x8A\xE1\x80\x80\xE1\x80\xB9\xE1\x80\x9A\xE1\x80\xB9\xE1\x80\x9D\xE1\x80\x94\xE1\x80\xB9\xE2\x80\x8C\xE1\x80\x99\x20"
	"\xE1\x80\x99\xE1\x80\xB9\xE1\x80\x9A\xE1\x80\x80\xE1\x80\xB9\xE2\x80\x8C\xE1\x80\x85\xE1\x80\xAC\xE1\x80\xB8\xE1\x80\x94\xE1\x80\xAF\xE1\x80"
	"\xAD\xE1\x80\x84\xE1\x80\xB9\xE2\x80\x8C\xE1\x80\x9E\xE1\x80\x8A\xE1\x80\xB9\xE2\x80\x8C\xE1\x81\x8B\x20\xE1\x81\x8E\xE1\x80\x80\xE1\x80\xB9"
	"\xE1\x80\x9B\xE1\x80\xB1\xE1\x80\xAC\xE1\x80\x84\xE1\x80\xB9\xE2\x80\x8C\xE1\x80\xB7\x20\xE1\x80\x91\xE1\x80\xAD\xE1\x80\x81\xE1\x80\xAF\xE1"
	"\x80\xAD\xE1\x80\x80\xE1\x80\xB9\xE2\x80\x8C\xE1\x80\x99\xE1\x80\xB9\xE1\x80\x9F\xE1\x80\xAF\x20\xE1\x80\x99\xE1\x80\x9B\xE1\x80\xB9\xE1\x80"
	"\x9F\xE1\x80\xAD\xE1\x80\x95\xE1\x80\xAC\xE1\x81\x8B",
	L"\x1000\x1039\x101A\x1039\x101D\x1014\x1039\x200C\x1010\x1031\x102C\x1039\x200C\x104A\x1000\x1039\x101A\x1039\x101D\x1014\x1039\x200C\x1019"
	L"\x0020\x1019\x1039\x101A\x1000\x1039\x200C\x1005\x102C\x1038\x1014\x102F\x102D\x1004\x1039\x200C\x101E\x100A\x1039\x200C\x104B\x0020\x104E"
	L"\x1000\x1039\x101B\x1031\x102C\x1004\x1039\x200C\x1037\x0020\x1011\x102D\x1001\x102F\x102D\x1000\x1039\x200C\x1019\x1039\x101F\x102F\x0020"
	L"\x1019\x101B\x1039\x101F\x102D\x1015\x102C\x104B",
{
	0x00001000, 0x00001039, 0x0000101A, 0x00001039, 0x0000101D, 0x00001014, 0x00001039, 0x0000200C, 0x00001010, 0x00001031, 0x0000102C, 0x00001039,
	0x0000200C, 0x0000104A, 0x00001000, 0x00001039, 0x0000101A, 0x00001039, 0x0000101D, 0x00001014, 0x00001039, 0x0000200C, 0x00001019, 0x00000020,
	0x00001019, 0x00001039, 0x0000101A, 0x00001000, 0x00001039, 0x0000200C, 0x00001005, 0x0000102C, 0x00001038, 0x00001014, 0x0000102F, 0x0000102D,
	0x00001004, 0x00001039, 0x0000200C, 0x0000101E, 0x0000100A, 0x00001039, 0x0000200C, 0x0000104B, 0x00000020, 0x0000104E, 0x00001000, 0x00001039,
	0x0000101B, 0x00001031, 0x0000102C, 0x00001004, 0x00001039, 0x0000200C, 0x00001037, 0x00000020, 0x00001011, 0x0000102D, 0x00001001, 0x0000102F,
	0x0000102D, 0x00001000, 0x00001039, 0x0000200C, 0x00001019, 0x00001039, 0x0000101F, 0x0000102F, 0x00000020, 0x00001019, 0x0000101B, 0x00001039,
	0x0000101F, 0x0000102D, 0x00001015, 0x0000102C, 0x0000104B
}
	}, // က္ယ္ဝန္‌တော္‌၊က္ယ္ဝန္‌မ မ္ယက္‌စားနုိင္‌သည္‌။ ၎က္ရောင္‌့ ထိခုိက္‌မ္ဟု မရ္ဟိပာ။

	{
		"\xF0\x9F\xA0\x80\xF0\x9F\xA0\x81\xF0\x9F\xA0\x82\xF0\x9F\xA0\x83\xF0\x9F\xA0\x84\xF0\x9F\xA0\x85\xF0\x9F\xA0\x86\xF0\x9F\xA0\x87\xF0\x9F\xA0"
		"\x88\xF0\x9F\xA0\x89\xF0\x9F\xA0\x8A\xF0\x9F\xA0\x8B",
	L"\xD83E\xDC00\xD83E\xDC01\xD83E\xDC02\xD83E\xDC03\xD83E\xDC04\xD83E\xDC05\xD83E\xDC06\xD83E\xDC07\xD83E\xDC08\xD83E\xDC09\xD83E\xDC0A\xD83E"
	L"\xDC0B",
{
	0x0001F800, 0x0001F801, 0x0001F802, 0x0001F803, 0x0001F804, 0x0001F805, 0x0001F806, 0x0001F807, 0x0001F808, 0x0001F809, 0x0001F80A, 0x0001F80B
}
	}, // 🠀🠁🠂🠃🠄🠅🠆🠇🠈🠉🠊🠋

	{
		"\xF0\x9F\x80\x80\xF0\x9F\x80\x81\xF0\x9F\x80\x82\xF0\x9F\x80\x83\xF0\x9F\x80\x84\xF0\x9F\x80\x85\xF0\x9F\x80\x86\xF0\x9F\x80\x87\xF0\x9F\x80"
		"\x88\xF0\x9F\x80\x89\xF0\x9F\x80\x8A\xF0\x9F\x80\x8B\xF0\x9F\x80\x8C\xF0\x9F\x80\x8D\xF0\x9F\x80\x8E\xF0\x9F\x80\x8F\xF0\x9F\x80\x90\xF0\x9F"
	"\x80\x91\xF0\x9F\x80\x92\xF0\x9F\x80\x93\xF0\x9F\x80\x94\xF0\x9F\x80\x95\xF0\x9F\x80\x96\xF0\x9F\x80\x97\xF0\x9F\x80\x98\xF0\x9F\x80\x99\xF0"
	"\x9F\x80\x9A\xF0\x9F\x80\x9B\xF0\x9F\x80\x9C\xF0\x9F\x80\x9D\xF0\x9F\x80\x9E\xF0\x9F\x80\x9F\xF0\x9F\x80\xA0\xF0\x9F\x80\xA1\xF0\x9F\x80\xA2"
	"\xF0\x9F\x80\xA3\xF0\x9F\x80\xA4\xF0\x9F\x80\xA5\xF0\x9F\x80\xA6\xF0\x9F\x80\xA7\xF0\x9F\x80\xA8\xF0\x9F\x80\xA9\xF0\x9F\x80\xAA\xF0\x9F\x80"
	"\xAB",
	L"\xD83C\xDC00\xD83C\xDC01\xD83C\xDC02\xD83C\xDC03\xD83C\xDC04\xD83C\xDC05\xD83C\xDC06\xD83C\xDC07\xD83C\xDC08\xD83C\xDC09\xD83C\xDC0A\xD83C"
	L"\xDC0B\xD83C\xDC0C\xD83C\xDC0D\xD83C\xDC0E\xD83C\xDC0F\xD83C\xDC10\xD83C\xDC11\xD83C\xDC12\xD83C\xDC13\xD83C\xDC14\xD83C\xDC15\xD83C\xDC16"
	L"\xD83C\xDC17\xD83C\xDC18\xD83C\xDC19\xD83C\xDC1A\xD83C\xDC1B\xD83C\xDC1C\xD83C\xDC1D\xD83C\xDC1E\xD83C\xDC1F\xD83C\xDC20\xD83C\xDC21\xD83C"
	L"\xDC22\xD83C\xDC23\xD83C\xDC24\xD83C\xDC25\xD83C\xDC26\xD83C\xDC27\xD83C\xDC28\xD83C\xDC29\xD83C\xDC2A\xD83C\xDC2B",
{
	0x0001F000, 0x0001F001, 0x0001F002, 0x0001F003, 0x0001F004, 0x0001F005, 0x0001F006, 0x0001F007, 0x0001F008, 0x0001F009, 0x0001F00A, 0x0001F00B,
	0x0001F00C, 0x0001F00D, 0x0001F00E, 0x0001F00F, 0x0001F010, 0x0001F011, 0x0001F012, 0x0001F013, 0x0001F014, 0x0001F015, 0x0001F016, 0x0001F017,
	0x0001F018, 0x0001F019, 0x0001F01A, 0x0001F01B, 0x0001F01C, 0x0001F01D, 0x0001F01E, 0x0001F01F, 0x0001F020, 0x0001F021, 0x0001F022, 0x0001F023,
	0x0001F024, 0x0001F025, 0x0001F026, 0x0001F027, 0x0001F028, 0x0001F029, 0x0001F02A, 0x0001F02B
}
	}, // 🀀🀁🀂🀃🀄🀅🀆🀇🀈🀉🀊🀋🀌🀍🀎🀏🀐🀑🀒🀓🀔🀕🀖🀗🀘🀙🀚🀛🀜🀝🀞🀟🀠🀡🀢🀣🀤🀥🀦🀧🀨🀩🀪🀫

	{
		"\xED\x9F\xBF",
		L"\xD7FF",
{
	0xD800 - 1
}
	},

	{
		"\xEE\x80\x80",
		L"\xE000",
{
	0xDFFF + 1
}
	},

	{
		"\xF0\x90\x80\x80",
		L"\xD800\xDC00",
{
	0x00010000
}
	},

	{
		"\xF4\x8F\xBF\xBF",
		L"\xDBFF\xDFFF",
{
	0x10FFFF
}
	},
};


int main()
{
	uint32_t n8, n16, n32;
	uint8_t  d8[1024];
	uint32_t d32[1024];
	uint16_t d16[1024];
	utf8util::validation_info info;
	unicode_tuple tuple;
	bool success;
#define OPT OPT_SKIP_INVALID

	printf("TEST utf8_length for 5 bytes string\n");
	{
		uint8_t *src, len;
		uint32_t cp, src_length, count;
		uint64_t cstr;
		for (len = 0;len <= 5;len++)
		{
			for (cstr = 0;cstr < (1ULL << (len * 8));cstr += 135)
			{
				src_length = len; count = 0; src = (uint8_t*)&cstr;
				while (src_length > 0)
				{
					UTF8_DECODE(cp, src, src_length, OPT);
					count++;if (cp >= 0x10000)count++;
				}
				n16 = utf8util::utf8to16_length((uint8_t*)&cstr, len, OPT);
				result = utf8util::utf8_validation((uint8_t*)&cstr, len, OPT, &info);
				if (result != RESULT_OK || count != n16 || info.length16 != n16) { printf("cstr = %lli len = %i error\n", cstr, len); len = 10; break; }
			}
			printf("len = %i\n", len);
		}
		printf("%s\n", len != 10 ? "success" : "error");
	}
	printf("TEST utf16_length for 4 bytes string\n");
	{
		uint16_t *src, len;
		uint32_t cp, src_length, count;
		uint64_t cstr;
		for (len = 0;len <= 4;len++)
		{
			for (cstr = 0;cstr < (1ULL << (len * 8));cstr += 134)
			{
				src_length = len; count = 0; src = (uint16_t*)&cstr;
				while (src_length > 0)
				{
					UTF16_DECODE(cp, src, src_length, OPT);
					count++;
				}
				n8 = utf8util::utf16to8_length((uint16_t*)&cstr, len, OPT);
				result = utf8util::utf16to8((uint16_t*)&cstr, len, d8, n8, OPT);
				n32 = utf8util::utf8to32_length(d8, n8, OPT);
				if (count != n32 || result != RESULT_OK) { printf("cstr = %lli len = %i error\n", cstr, len); len = 10; break; }
			}
		}
		printf("%s\n", len != 10 ? "success" : "error");
	}
#if 0
	printf("TEST utf8_codepoints for unaligned string\n");
	{
		// यूनिकोड : 7 codepoints
		// L"\x092F\x0942\x0928\x093F\x0915\x094B\x0921",
		char str[] = "\x1\x2\xE0\xA4\xAF\xE0\xA5\x82\xE0\xA4\xA8\xE0\xA4\xBF\xE0\xA4\x95\xE0\xA5\x8B\xE0\xA4\xA1";
		char* src = str + 2;
		n8 = sizeof(str) - 3;
		success = utf8util::utf8_codepoints((uint8_t*)src, n8) == 7;
		printf("%s\n", success ? "success" : "error");
		success = utf8util::utf8to32_length((uint8_t*)src, n8) == 7;
		printf("%s\n", success ? "success" : "error");
		char str0[] = "";
		success = utf8util::utf8_codepoints((uint8_t*)str0, 0) == 0;
		printf("%s\n", success ? "success" : "error");
		success = utf8util::utf8to32_length((uint8_t*)str0, 0) == 0;
		printf("%s\n", success ? "success" : "error");
		char str1[] = "a";
		success = utf8util::utf8_codepoints((uint8_t*)str1, 1) == 1;
		printf("%s\n", success ? "success" : "error");
		success = utf8util::utf8to32_length((uint8_t*)str1, 1) == 1;
		printf("%s\n", success ? "success" : "error");
		char str2[] = "\xC0\xA4";
		success = utf8util::utf8_codepoints((uint8_t*)str2, 2) == 1;
		printf("%s\n", success ? "success" : "error");
		success = utf8util::utf8to32_length((uint8_t*)str2, 2) == 1;
		printf("%s\n", success ? "success" : "error");
		char str3[] = "\xE1\xAB\x80";
		success = utf8util::utf8_codepoints((uint8_t*)str3, 3) == 1;
		printf("%s\n", success ? "success" : "error");
		success = utf8util::utf8to32_length((uint8_t*)str3, 3) == 1;
		printf("%s\n", success ? "success" : "error");
	}
#endif
	printf("TEST utf8 for some erroneous string\n");
	{
		char str[] = "\xE0\xA0\xF0"; n8 = sizeof(str) - 1;
		n16 = utf8util::utf8to16_length((uint8_t*)str, n8, OPT);
		printf("%s\n", n16 == (OPT == OPT_SKIP_INVALID ? 0 : 2) ? "success" : "error");
		n32 = utf8util::utf8to32_length((uint8_t*)str, n8, OPT);
		printf("%s\n", n32 == (OPT == OPT_SKIP_INVALID ? 0 : 2) ? "success" : "error");
		result = utf8util::utf8to16((uint8_t*)str, n8, d16, n16, OPT);
		printf("%s\n", result == RESULT_OK && (OPT == OPT_SKIP_INVALID ? true : d16[0] == 0xFFFD) ? "success" : "error");
		result = utf8util::utf8to32((uint8_t*)str, n8, d32, n32, OPT);
		printf("%s\n", result == RESULT_OK && (OPT == OPT_SKIP_INVALID ? true : d32[0] == 0xFFFD) ? "success" : "error");
	}
	//TEST convert utf8to16
	printf("TEST convert utf8to16\n");
	{
		for (int i = 0;i < sizeof(unicode_test_data) / sizeof(unicode_test_data[0]);i++)
		{
			std::wstring res;
			tuple = (unicode_tuple&)unicode_test_data[i];
			n8 = tuple.utf8.size();
			n16 = utf8util::utf8to16_length((uint8_t*)tuple.utf8.c_str(), n8, OPT); memset(d16, 0xF, sizeof(d16));
			result = utf8util::utf8to16((uint8_t*)tuple.utf8.c_str(), n8, d16, n16, OPT);
			res.assign((const wchar_t*)d16, n16);
			success = result == RESULT_OK && res == tuple.utf16;
			printf("%i) %s %i\n", i, success ? "success" : "error", result);
		}
	}
	printf("TEST convert utf8to32\n");
	{
		for (int i = 0;i < sizeof(unicode_test_data) / sizeof(unicode_test_data[0]);i++)
		{
			std::u32string res;
			tuple = (unicode_tuple&)unicode_test_data[i];
			n8 = tuple.utf8.size();
			n32 = utf8util::utf8to32_length((uint8_t*)tuple.utf8.c_str(), n8, OPT); memset(d32, 0xF, sizeof(d32));
			result = utf8util::utf8to32((uint8_t*)tuple.utf8.c_str(), n8, d32, n32, OPT);
			res.assign((const char32_t*)d32, n32);
			success = result == RESULT_OK && res == tuple.utf32;
			printf("%i) %s %i\n", i, success ? "success" : "error", result);
		}
	}
	printf("TEST convert utf16to8\n");
	{
		for (int i = 0;i < sizeof(unicode_test_data) / sizeof(unicode_test_data[0]);i++)
		{
			std::string res;
			tuple = (unicode_tuple&)unicode_test_data[i];
			n16 = tuple.utf16.size();
			n8 = utf8util::utf16to8_length((uint16_t*)tuple.utf16.c_str(), n16, OPT); memset(d8, 0xF, sizeof(d8));
			result = utf8util::utf16to8((uint16_t*)tuple.utf16.c_str(), n16, d8, n8, OPT);
			res.assign((const char*)d8, n8);
			success = result == RESULT_OK && res == tuple.utf8;
			printf("%i) %s %i\n", i, success ? "success" : "error", result);
		}
	}
	printf("TEST convert utf32to8\n");
	{
		for (int i = 0;i < sizeof(unicode_test_data) / sizeof(unicode_test_data[0]);i++)
		{
			std::string res;
			tuple = (unicode_tuple&)unicode_test_data[i];
			n32 = tuple.utf32.size();
			n8 = utf8util::utf32to8_length((uint32_t*)tuple.utf32.c_str(), n32, OPT); memset(d8, 0xF, sizeof(d8));
			result = utf8util::utf32to8((uint32_t*)tuple.utf32.c_str(), n32, d8, n8, OPT);
			res.assign((const char*)d8, n8);
			success = result == RESULT_OK && res == tuple.utf8;
			printf("%i) %s %i\n", i, success ? "success" : "error", result);
		}
	}
	return 0;
}

