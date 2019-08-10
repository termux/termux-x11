#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#ifndef __EVDEV_KEYCODES_H__
#define EVDEV___EVDEV_KEYCODES_H__

// Added for pc105 compatibility
#define EVDEV_LSGT 94

#define EVDEV_TLDE 49
#define EVDEV_AE01 10
#define EVDEV_AE02 11
#define EVDEV_AE03 12
#define EVDEV_AE04 13
#define EVDEV_AE05 14
#define EVDEV_AE06 15
#define EVDEV_AE07 16
#define EVDEV_AE08 17
#define EVDEV_AE09 18
#define EVDEV_AE10 19
#define EVDEV_AE11 20
#define EVDEV_AE12 21
#define EVDEV_BKSP 22

#define EVDEV_TAB 23
#define EVDEV_AD01 24
#define EVDEV_AD02 25
#define EVDEV_AD03 26
#define EVDEV_AD04 27
#define EVDEV_AD05 28
#define EVDEV_AD06 29
#define EVDEV_AD07 30
#define EVDEV_AD08 31
#define EVDEV_AD09 32
#define EVDEV_AD10 33
#define EVDEV_AD11 34
#define EVDEV_AD12 35
#define EVDEV_BKSL 51
#define EVDEV_AC12 EVDEV_BKSL
#define EVDEV_RTRN 36

#define EVDEV_CAPS 66
#define EVDEV_AC01 38
#define EVDEV_AC02 39
#define EVDEV_AC03 40
#define EVDEV_AC04 41
#define EVDEV_AC05 42
#define EVDEV_AC06 43
#define EVDEV_AC07 44
#define EVDEV_AC08 45
#define EVDEV_AC09 46
#define EVDEV_AC10 47
#define EVDEV_AC11 48

#define EVDEV_LFSH 50
#define EVDEV_AB01 52
#define EVDEV_AB02 53
#define EVDEV_AB03 54
#define EVDEV_AB04 55
#define EVDEV_AB05 56
#define EVDEV_AB06 57
#define EVDEV_AB07 58
#define EVDEV_AB08 59
#define EVDEV_AB09 60
#define EVDEV_AB10 61
#define EVDEV_RTSH 62

#define EVDEV_LALT 64
#define EVDEV_LCTL 37
#define EVDEV_SPCE 65
#define EVDEV_RCTL 105
#define EVDEV_RALT 108
// Microsoft keyboard extra keys
#define EVDEV_LWIN 133
#define EVDEV_RWIN 134
#define EVDEV_COMP 135
#define EVDEV_MENU EVDEV_COMP

#define EVDEV_ESC 9
#define EVDEV_FK01 67
#define EVDEV_FK02 68
#define EVDEV_FK03 69
#define EVDEV_FK04 70
#define EVDEV_FK05 71
#define EVDEV_FK06 72
#define EVDEV_FK07 73
#define EVDEV_FK08 74
#define EVDEV_FK09 75
#define EVDEV_FK10 76
#define EVDEV_FK11 95
#define EVDEV_FK12 96

#define EVDEV_PRSC 107
//#define EVDEV_SYRQ 107
#define EVDEV_SCLK 78
#define EVDEV_PAUS 127
//#define EVDEV_BRK 419

#define EVDEV_INS 118
#define EVDEV_HOME 110
#define EVDEV_PGUP 112
#define EVDEV_DELE 119
#define EVDEV_END 115
#define EVDEV_PGDN 117

#define EVDEV_UP 111
#define EVDEV_LEFT 113
#define EVDEV_DOWN 116
#define EVDEV_RGHT 114

#define EVDEV_NMLK 77
#define EVDEV_KPDV 106
#define EVDEV_KPMU 63
#define EVDEV_KPSU 82

#define EVDEV_KP7 79
#define EVDEV_KP8 80
#define EVDEV_KP9 81
#define EVDEV_KPAD 86

#define EVDEV_KP4 83
#define EVDEV_KP5 84
#define EVDEV_KP6 85

#define EVDEV_KP1 87
#define EVDEV_KP2 88
#define EVDEV_KP3 89
#define EVDEV_KPEN 104

#define EVDEV_KP0 90
#define EVDEV_KPDL 91
#define EVDEV_KPEQ 125

#define EVDEV_FK13 191
#define EVDEV_FK14 192
#define EVDEV_FK15 193
#define EVDEV_FK16 194
#define EVDEV_FK17 195
#define EVDEV_FK18 196
#define EVDEV_FK19 197
#define EVDEV_FK20 198
#define EVDEV_FK21 199
#define EVDEV_FK22 200
#define EVDEV_FK23 201
#define EVDEV_FK24 202

// Keys that are generated on Japanese keyboards

//#define EVDEV_HZTG  93	// Hankaku/Zenkakau toggle - not actually used
#define EVDEV_HZTG TLDE
#define EVDEV_HKTG 101	// Hiragana/Katakana toggle
#define EVDEV_AB11 97		// backslash/underscore
#define EVDEV_HENK 100	// Henkan
#define EVDEV_MUHE 102	// Muhenkan
#define EVDEV_AE13 132	// Yen
#define EVDEV_KATA  98	// Katakana
#define EVDEV_HIRA  99	// Hiragana
#define EVDEV_JPCM 103	// KPJPComma
// #define EVDEV_RO 97	// Romaji

// Keys that are generated on Korean keyboards

#define EVDEV_HNGL 130	// Hangul Latin toggle
#define EVDEV_HJCV 131	// Hangul to Hanja conversion

	// Solaris compatibility

#define EVDEV_LMTA EVDEV_LWIN
#define EVDEV_RMTA EVDEV_RWIN
#define EVDEV_MUTE 121
#define EVDEV_VOL_MINUS 122
#define EVDEV_VOL_PLUS 123
#define EVDEV_POWR 124
#define EVDEV_STOP 136
#define EVDEV_AGAI 137
#define EVDEV_PROP 138
#define EVDEV_UNDO 139
#define EVDEV_FRNT 140
#define EVDEV_COPY 141
#define EVDEV_OPEN 142
#define EVDEV_PAST 143
#define EVDEV_FIND 144
#define EVDEV_CUT  145
#define EVDEV_HELP 146

// Extended keys that may be generated on "Internet" keyboards.
// evdev has standardize names for these.

#define EVDEV_LNFD 109	// #define EVDEV_KEY_LINEFEED            101
#define EVDEV_I120 120	// #define EVDEV_KEY_MACRO               112
#define EVDEV_I126 126	// #define EVDEV_KEY_KPPLUSMINUS         118
#define EVDEV_I128 128    // #define EVDEV_KEY_SCALE               120
#define EVDEV_I129 129	// #define EVDEV_KEY_KPCOMMA             121
#define EVDEV_I147 147	// #define EVDEV_KEY_MENU                139
#define EVDEV_I148 148	// #define EVDEV_KEY_CALC                140
#define EVDEV_I149 149	// #define EVDEV_KEY_SETUP               141
#define EVDEV_I150 150	// #define EVDEV_KEY_SLEEP               142
#define EVDEV_I151 151	// #define EVDEV_KEY_WAKEUP              143
#define EVDEV_I152 152	// #define EVDEV_KEY_FILE                144
#define EVDEV_I153 153	// #define EVDEV_KEY_SENDFILE            145
#define EVDEV_I154 154	// #define EVDEV_KEY_DELETEFILE          146
#define EVDEV_I155 155	// #define EVDEV_KEY_XFER                147
#define EVDEV_I156 156	// #define EVDEV_KEY_PROG1               148
#define EVDEV_I157 157	// #define EVDEV_KEY_PROG2               149
#define EVDEV_I158 158	// #define EVDEV_KEY_WWW                 150
#define EVDEV_I159 159	// #define EVDEV_KEY_MSDOS               151
#define EVDEV_I160 160	// #define EVDEV_KEY_COFFEE              152
#define EVDEV_I161 161	// #define EVDEV_KEY_DIRECTION           153
#define EVDEV_I162 162	// #define EVDEV_KEY_CYCLEWINDOWS        154
#define EVDEV_I163 163	// #define EVDEV_KEY_MAIL                155
#define EVDEV_I164 164	// #define EVDEV_KEY_BOOKMARKS           156
#define EVDEV_I165 165	// #define EVDEV_KEY_COMPUTER            157
#define EVDEV_I166 166	// #define EVDEV_KEY_BACK                158
#define EVDEV_I167 167	// #define EVDEV_KEY_FORWARD             159
#define EVDEV_I168 168	// #define EVDEV_KEY_CLOSECD             160
#define EVDEV_I169 169	// #define EVDEV_KEY_EJECTCD             161
#define EVDEV_I170 170	// #define EVDEV_KEY_EJECTCLOSECD        162
#define EVDEV_I171 171	// #define EVDEV_KEY_NEXTSONG            163
#define EVDEV_I172 172	// #define EVDEV_KEY_PLAYPAUSE           164
#define EVDEV_I173 173	// #define EVDEV_KEY_PREVIOUSSONG        165
#define EVDEV_I174 174	// #define EVDEV_KEY_STOPCD              166
#define EVDEV_I175 175	// #define EVDEV_KEY_RECORD              167
#define EVDEV_I176 176	// #define EVDEV_KEY_REWIND              168
#define EVDEV_I177 177	// #define EVDEV_KEY_PHONE               169
#define EVDEV_I178 178	// #define EVDEV_KEY_ISO                 170
#define EVDEV_I179 179	// #define EVDEV_KEY_CONFIG              171
#define EVDEV_I180 180	// #define EVDEV_KEY_HOMEPAGE            172
#define EVDEV_I181 181	// #define EVDEV_KEY_REFRESH             173
#define EVDEV_I182 182	// #define EVDEV_KEY_EXIT                174
#define EVDEV_I183 183	// #define EVDEV_KEY_MOVE                175
#define EVDEV_I184 184	// #define EVDEV_KEY_EDIT                176
#define EVDEV_I185 185	// #define EVDEV_KEY_SCROLLUP            177
#define EVDEV_I186 186	// #define EVDEV_KEY_SCROLLDOWN          178
#define EVDEV_I187 187	// #define EVDEV_KEY_KPLEFTPAREN         179
#define EVDEV_I188 188	// #define EVDEV_KEY_KPRIGHTPAREN        180
#define EVDEV_I189 189	// #define EVDEV_KEY_NEW                 181
#define EVDEV_I190 190	// #define EVDEV_KEY_REDO                182
#define EVDEV_I208 208	// #define EVDEV_KEY_PLAYCD              200
#define EVDEV_I209 209	// #define EVDEV_KEY_PAUSECD             201
#define EVDEV_I210 210	// #define EVDEV_KEY_PROG3               202
#define EVDEV_I211 211	// #define EVDEV_KEY_PROG4               203 conflicts with AB11
#define EVDEV_I212 212  // #define EVDEV_KEY_DASHBOARD           204
#define EVDEV_I213 213	// #define EVDEV_KEY_SUSPEND             205
#define EVDEV_I214 214	// #define EVDEV_KEY_CLOSE               206
#define EVDEV_I215 215	// #define EVDEV_KEY_PLAY                207
#define EVDEV_I216 216	// #define EVDEV_KEY_FASTFORWARD         208
#define EVDEV_I217 217	// #define EVDEV_KEY_BASSBOOST           209
#define EVDEV_I218 218	// #define EVDEV_KEY_PRINT               210
#define EVDEV_I219 219	// #define EVDEV_KEY_HP                  211
#define EVDEV_I220 220	// #define EVDEV_KEY_CAMERA              212
#define EVDEV_I221 221	// #define EVDEV_KEY_SOUND               213
#define EVDEV_I222 222	// #define EVDEV_KEY_QUESTION            214
#define EVDEV_I223 223	// #define EVDEV_KEY_EMAIL               215
#define EVDEV_I224 224	// #define EVDEV_KEY_CHAT                216
#define EVDEV_I225 225	// #define EVDEV_KEY_SEARCH              217
#define EVDEV_I226 226	// #define EVDEV_KEY_CONNECT             218
#define EVDEV_I227 227	// #define EVDEV_KEY_FINANCE             219
#define EVDEV_I228 228	// #define EVDEV_KEY_SPORT               220
#define EVDEV_I229 229	// #define EVDEV_KEY_SHOP                221
#define EVDEV_I230 230	// #define EVDEV_KEY_ALTERASE            222
#define EVDEV_I231 231	// #define EVDEV_KEY_CANCEL              223
#define EVDEV_I232 232	// #define EVDEV_KEY_BRIGHTNESSDOWN      224
#define EVDEV_I233 233	// #define EVDEV_KEY_BRIGHTNESSUP        225
#define EVDEV_I234 234	// #define EVDEV_KEY_MEDIA               226
#define EVDEV_I235 235	// #define EVDEV_KEY_SWITCHVIDEOMODE     227
#define EVDEV_I236 236	// #define EVDEV_KEY_KBDILLUMTOGGLE      228
#define EVDEV_I237 237	// #define EVDEV_KEY_KBDILLUMDOWN        229
#define EVDEV_I238 238	// #define EVDEV_KEY_KBDILLUMUP          230
#define EVDEV_I239 239	// #define EVDEV_KEY_SEND                231
#define EVDEV_I240 240	// #define EVDEV_KEY_REPLY               232
#define EVDEV_I241 241	// #define EVDEV_KEY_FORWARDMAIL         233
#define EVDEV_I242 242	// #define EVDEV_KEY_SAVE                234
#define EVDEV_I243 243	// #define EVDEV_KEY_DOCUMENTS           235
#define EVDEV_I244 244	// #define EVDEV_KEY_BATTERY             236
#define EVDEV_I245 245	// #define EVDEV_KEY_BLUETOOTH           237
#define EVDEV_I246 246	// #define EVDEV_KEY_WLAN                238
#define EVDEV_I247 247	// #define EVDEV_KEY_UWB                 239
#define EVDEV_I248 248	// #define EVDEV_KEY_UNKNOWN             240
#define EVDEV_I249 249	// #define EVDEV_KEY_VIDEO_NEXT          241
#define EVDEV_I250 250	// #define EVDEV_KEY_VIDEO_PREV          242
#define EVDEV_I251 251	// #define EVDEV_KEY_BRIGHTNESS_CYCLE    243
#define EVDEV_I252 252	// #define EVDEV_KEY_BRIGHTNESS_ZERO     244
#define EVDEV_I253 253	// #define EVDEV_KEY_DISPLAY_OFF         245
#define EVDEV_I254 254	// #define EVDEV_KEY_WWAN                246
#define EVDEV_I255 255	// #define EVDEV_KEY_RFKILL              247

// Fake keycodes for virtual keys
#define EVDEV_LVL3   92
#define EVDEV_MDSW   203
#define EVDEV_ALT    204
#define EVDEV_META   205
#define EVDEV_SUPR   206
#define EVDEV_HYPR   207

#endif // __EVDEV_KEYCODES_H__

#pragma clang diagnostic pop
