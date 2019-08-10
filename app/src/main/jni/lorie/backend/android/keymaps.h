#include <stdint.h>
#define SYM_LENGTH 7
#define KEYCODE_MIN 8
#define KEYCODE_MAX 255
#define NOSYM {{0}, {0}}
struct lorie_keymap {
	char *name;
	struct keysym {
		char normal[SYM_LENGTH];
		char shift[SYM_LENGTH];
	} keysyms[KEYCODE_MAX - KEYCODE_MIN];
};

struct lorie_keymap lorie_keymap_ru = {
	.name = (char*) "ru",
	.keysyms = {
		NOSYM, // eventCode: 0
		NOSYM, // eventCode: 1
		{{49, 0, 0, 0, 0, 0, 0}, {33, 0, 0, 0, 0, 0, 0}}, // eventCode: 2; normal: "1"; shift: "!";
		{{50, 0, 0, 0, 0, 0, 0}, {34, 0, 0, 0, 0, 0, 0}}, // eventCode: 3; normal: "2"; shift: """;
		{{51, 0, 0, 0, 0, 0, 0}, {-30, -124, -106, 0, 0, 0, 0}}, // eventCode: 4; normal: "3"; shift: "№";
		{{52, 0, 0, 0, 0, 0, 0}, {59, 0, 0, 0, 0, 0, 0}}, // eventCode: 5; normal: "4"; shift: ";";
		{{53, 0, 0, 0, 0, 0, 0}, {37, 0, 0, 0, 0, 0, 0}}, // eventCode: 6; normal: "5"; shift: "%";
		{{54, 0, 0, 0, 0, 0, 0}, {58, 0, 0, 0, 0, 0, 0}}, // eventCode: 7; normal: "6"; shift: ":";
		{{55, 0, 0, 0, 0, 0, 0}, {63, 0, 0, 0, 0, 0, 0}}, // eventCode: 8; normal: "7"; shift: "?";
		{{56, 0, 0, 0, 0, 0, 0}, {42, 0, 0, 0, 0, 0, 0}}, // eventCode: 9; normal: "8"; shift: "*";
		{{57, 0, 0, 0, 0, 0, 0}, {40, 0, 0, 0, 0, 0, 0}}, // eventCode: 10; normal: "9"; shift: "(";
		{{48, 0, 0, 0, 0, 0, 0}, {41, 0, 0, 0, 0, 0, 0}}, // eventCode: 11; normal: "0"; shift: ")";
		{{45, 0, 0, 0, 0, 0, 0}, {95, 0, 0, 0, 0, 0, 0}}, // eventCode: 12; normal: "-"; shift: "_";
		{{61, 0, 0, 0, 0, 0, 0}, {43, 0, 0, 0, 0, 0, 0}}, // eventCode: 13; normal: "="; shift: "+";
		{{8, 0, 0, 0, 0, 0, 0}, {8, 0, 0, 0, 0, 0, 0}}, // eventCode: 14;
		NOSYM, // eventCode: 15; normal: "	";
		{{-48, -71, 0, 0, 0, 0, 0}, {-48, -103, 0, 0, 0, 0, 0}}, // eventCode: 16; normal: "й"; shift: "Й";
		{{-47, -122, 0, 0, 0, 0, 0}, {-48, -90, 0, 0, 0, 0, 0}}, // eventCode: 17; normal: "ц"; shift: "Ц";
		{{-47, -125, 0, 0, 0, 0, 0}, {-48, -93, 0, 0, 0, 0, 0}}, // eventCode: 18; normal: "у"; shift: "У";
		{{-48, -70, 0, 0, 0, 0, 0}, {-48, -102, 0, 0, 0, 0, 0}}, // eventCode: 19; normal: "к"; shift: "К";
		{{-48, -75, 0, 0, 0, 0, 0}, {-48, -107, 0, 0, 0, 0, 0}}, // eventCode: 20; normal: "е"; shift: "Е";
		{{-48, -67, 0, 0, 0, 0, 0}, {-48, -99, 0, 0, 0, 0, 0}}, // eventCode: 21; normal: "н"; shift: "Н";
		{{-48, -77, 0, 0, 0, 0, 0}, {-48, -109, 0, 0, 0, 0, 0}}, // eventCode: 22; normal: "г"; shift: "Г";
		{{-47, -120, 0, 0, 0, 0, 0}, {-48, -88, 0, 0, 0, 0, 0}}, // eventCode: 23; normal: "ш"; shift: "Ш";
		{{-47, -119, 0, 0, 0, 0, 0}, {-48, -87, 0, 0, 0, 0, 0}}, // eventCode: 24; normal: "щ"; shift: "Щ";
		{{-48, -73, 0, 0, 0, 0, 0}, {-48, -105, 0, 0, 0, 0, 0}}, // eventCode: 25; normal: "з"; shift: "З";
		{{-47, -123, 0, 0, 0, 0, 0}, {-48, -91, 0, 0, 0, 0, 0}}, // eventCode: 26; normal: "х"; shift: "Х";
		{{-47, -118, 0, 0, 0, 0, 0}, {-48, -86, 0, 0, 0, 0, 0}}, // eventCode: 27; normal: "ъ"; shift: "Ъ";
		{{13, 0, 0, 0, 0, 0, 0}, {13, 0, 0, 0, 0, 0, 0}}, // eventCode: 28;
		NOSYM, // eventCode: 29
		{{-47, -124, 0, 0, 0, 0, 0}, {-48, -92, 0, 0, 0, 0, 0}}, // eventCode: 30; normal: "ф"; shift: "Ф";
		{{-47, -117, 0, 0, 0, 0, 0}, {-48, -85, 0, 0, 0, 0, 0}}, // eventCode: 31; normal: "ы"; shift: "Ы";
		{{-48, -78, 0, 0, 0, 0, 0}, {-48, -110, 0, 0, 0, 0, 0}}, // eventCode: 32; normal: "в"; shift: "В";
		{{-48, -80, 0, 0, 0, 0, 0}, {-48, -112, 0, 0, 0, 0, 0}}, // eventCode: 33; normal: "а"; shift: "А";
		{{-48, -65, 0, 0, 0, 0, 0}, {-48, -97, 0, 0, 0, 0, 0}}, // eventCode: 34; normal: "п"; shift: "П";
		{{-47, -128, 0, 0, 0, 0, 0}, {-48, -96, 0, 0, 0, 0, 0}}, // eventCode: 35; normal: "р"; shift: "Р";
		{{-48, -66, 0, 0, 0, 0, 0}, {-48, -98, 0, 0, 0, 0, 0}}, // eventCode: 36; normal: "о"; shift: "О";
		{{-48, -69, 0, 0, 0, 0, 0}, {-48, -101, 0, 0, 0, 0, 0}}, // eventCode: 37; normal: "л"; shift: "Л";
		{{-48, -76, 0, 0, 0, 0, 0}, {-48, -108, 0, 0, 0, 0, 0}}, // eventCode: 38; normal: "д"; shift: "Д";
		{{-48, -74, 0, 0, 0, 0, 0}, {-48, -106, 0, 0, 0, 0, 0}}, // eventCode: 39; normal: "ж"; shift: "Ж";
		{{-47, -115, 0, 0, 0, 0, 0}, {-48, -83, 0, 0, 0, 0, 0}}, // eventCode: 40; normal: "э"; shift: "Э";
		{{-47, -111, 0, 0, 0, 0, 0}, {-48, -127, 0, 0, 0, 0, 0}}, // eventCode: 41; normal: "ё"; shift: "Ё";
		NOSYM, // eventCode: 42;
		{{92, 0, 0, 0, 0, 0, 0}, {47, 0, 0, 0, 0, 0, 0}}, // eventCode: 43; normal: "\"; shift: "/";
		{{-47, -113, 0, 0, 0, 0, 0}, {-48, -81, 0, 0, 0, 0, 0}}, // eventCode: 44; normal: "я"; shift: "Я";
		{{-47, -121, 0, 0, 0, 0, 0}, {-48, -89, 0, 0, 0, 0, 0}}, // eventCode: 45; normal: "ч"; shift: "Ч";
		{{-47, -127, 0, 0, 0, 0, 0}, {-48, -95, 0, 0, 0, 0, 0}}, // eventCode: 46; normal: "с"; shift: "С";
		{{-48, -68, 0, 0, 0, 0, 0}, {-48, -100, 0, 0, 0, 0, 0}}, // eventCode: 47; normal: "м"; shift: "М";
		{{-48, -72, 0, 0, 0, 0, 0}, {-48, -104, 0, 0, 0, 0, 0}}, // eventCode: 48; normal: "и"; shift: "И";
		{{-47, -126, 0, 0, 0, 0, 0}, {-48, -94, 0, 0, 0, 0, 0}}, // eventCode: 49; normal: "т"; shift: "Т";
		{{-47, -116, 0, 0, 0, 0, 0}, {-48, -84, 0, 0, 0, 0, 0}}, // eventCode: 50; normal: "ь"; shift: "Ь";
		{{-48, -79, 0, 0, 0, 0, 0}, {-48, -111, 0, 0, 0, 0, 0}}, // eventCode: 51; normal: "б"; shift: "Б";
		{{-47, -114, 0, 0, 0, 0, 0}, {-48, -82, 0, 0, 0, 0, 0}}, // eventCode: 52; normal: "ю"; shift: "Ю";
		{{46, 0, 0, 0, 0, 0, 0}, {44, 0, 0, 0, 0, 0, 0}}, // eventCode: 53; normal: "."; shift: ",";
	}
};

struct lorie_keymap *lorie_keymaps[] = {&lorie_keymap_ru, NULL};

struct lorie_keymap_android {
	int eventCode;
	int shift;
} lorie_keymap_android[] = {
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{1, 0}, // keycode 4
	{0, 0}, {0, 0}, 
	{11, 0}, // keycode 7
	{2, 0}, // keycode 8
	{3, 0}, // keycode 9
	{4, 0}, // keycode 10
	{5, 0}, // keycode 11
	{6, 0}, // keycode 12
	{7, 0}, // keycode 13
	{8, 0}, // keycode 14
	{9, 0}, // keycode 15
	{10, 0}, // keycode 16
	{9, 1}, // keycode 17
	{4, 1}, // keycode 18
	{103, 0}, // keycode 19
	{108, 0}, // keycode 20
	{105, 0}, // keycode 21
	{106, 0}, // keycode 22
	{0, 0}, 
	{115, 0}, // keycode 24
	{114, 0}, // keycode 25
	{116, 0}, // keycode 26
	{212, 0}, // keycode 27
	{0, 0}, 
	{30, 0}, // keycode 29
	{48, 0}, // keycode 30
	{46, 0}, // keycode 31
	{32, 0}, // keycode 32
	{18, 0}, // keycode 33
	{33, 0}, // keycode 34
	{34, 0}, // keycode 35
	{35, 0}, // keycode 36
	{23, 0}, // keycode 37
	{36, 0}, // keycode 38
	{37, 0}, // keycode 39
	{38, 0}, // keycode 40
	{50, 0}, // keycode 41
	{49, 0}, // keycode 42
	{24, 0}, // keycode 43
	{25, 0}, // keycode 44
	{16, 0}, // keycode 45
	{19, 0}, // keycode 46
	{31, 0}, // keycode 47
	{20, 0}, // keycode 48
	{22, 0}, // keycode 49
	{47, 0}, // keycode 50
	{17, 0}, // keycode 51
	{45, 0}, // keycode 52
	{21, 0}, // keycode 53
	{44, 0}, // keycode 54
	{51, 0}, // keycode 55
	{52, 0}, // keycode 56
	{56, 0}, // keycode 57
	{100, 0}, // keycode 58
	{42, 0}, // keycode 59
	{54, 0}, // keycode 60
	{15, 0}, // keycode 61
	{57, 0}, // keycode 62
	{0, 0}, 
	{150, 0}, // keycode 64
	{155, 0}, // keycode 65
	{28, 0}, // keycode 66
	{14, 0}, // keycode 67
	{41, 0}, // keycode 68
	{12, 0}, // keycode 69
	{13, 0}, // keycode 70
	{26, 0}, // keycode 71
	{27, 0}, // keycode 72
	{43, 0}, // keycode 73
	{39, 0}, // keycode 74
	{40, 0}, // keycode 75
	{53, 0}, // keycode 76
	{3, 1}, // keycode 77
	{0, 0}, {0, 0}, {0, 0}, 
	{13, 1}, // keycode 81
	{139, 0}, // keycode 82
	{0, 0}, 
	{217, 0}, // keycode 84
	{164, 0}, // keycode 85
	{625, 0}, // keycode 86
	{163, 0}, // keycode 87
	{165, 0}, // keycode 88
	{168, 0}, // keycode 89
	{208, 0}, // keycode 90
	{248, 0}, // keycode 91
	{104, 0}, // keycode 92
	{109, 0}, // keycode 93
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{1, 0}, // keycode 111
	{111, 0}, // keycode 112
	{29, 0}, // keycode 113
	{97, 0}, // keycode 114
	{58, 0}, // keycode 115
	{70, 0}, // keycode 116
	{125, 0}, // keycode 117
	{126, 0}, // keycode 118
	{0, 0}, 
	{99, 0}, // keycode 120
	{411, 0}, // keycode 121
	{102, 0}, // keycode 122
	{107, 0}, // keycode 123
	{110, 0}, // keycode 124
	{159, 0}, // keycode 125
	{207, 0}, // keycode 126
	{0, 0}, 
	{160, 0}, // keycode 128
	{161, 0}, // keycode 129
	{167, 0}, // keycode 130
	{59, 0}, // keycode 131
	{60, 0}, // keycode 132
	{61, 0}, // keycode 133
	{62, 0}, // keycode 134
	{63, 0}, // keycode 135
	{64, 0}, // keycode 136
	{65, 0}, // keycode 137
	{66, 0}, // keycode 138
	{67, 0}, // keycode 139
	{68, 0}, // keycode 140
	{87, 0}, // keycode 141
	{88, 0}, // keycode 142
	{69, 0}, // keycode 143
	{82, 0}, // keycode 144
	{79, 0}, // keycode 145
	{80, 0}, // keycode 146
	{81, 0}, // keycode 147
	{75, 0}, // keycode 148
	{76, 0}, // keycode 149
	{77, 0}, // keycode 150
	{71, 0}, // keycode 151
	{72, 0}, // keycode 152
	{73, 0}, // keycode 153
	{98, 0}, // keycode 154
	{55, 0}, // keycode 155
	{74, 0}, // keycode 156
	{78, 0}, // keycode 157
	{83, 0}, // keycode 158
	{121, 0}, // keycode 159
	{96, 0}, // keycode 160
	{117, 0}, // keycode 161
	{179, 0}, // keycode 162
	{180, 0}, // keycode 163
	{113, 0}, // keycode 164
	{358, 0}, // keycode 165
	{402, 0}, // keycode 166
	{403, 0}, // keycode 167
	{418, 0}, // keycode 168
	{419, 0}, // keycode 169
	{377, 0}, // keycode 170
	{0, 0}, {0, 0}, {0, 0}, 
	{156, 0}, // keycode 174
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{398, 0}, // keycode 183
	{399, 0}, // keycode 184
	{400, 0}, // keycode 185
	{401, 0}, // keycode 186
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{429, 0}, // keycode 207
	{397, 0}, // keycode 208
	{387, 0}, // keycode 209
	{140, 0}, // keycode 210
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{224, 0}, // keycode 220
	{225, 0}, // keycode 221
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
};
