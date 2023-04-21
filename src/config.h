#include <stdint.h>

static const char statusPath[] = "/tmp/deburrstat"; // Location of the status pipe (through which you can set the status)

static const double padding = 0.3; // Adds padding above and under the text
                                   // This value is them multiplied font height

static const char *fonts[2] = { "JetBrainsMono:size=18", // Written using fontconfig selectors
                                "Koruri:size=18" };      // Max 2 fonts, Leave NULL if you 
                                                         // don't want a fallback font

static const wchar_t *tags[] = { L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9" };

static const uint64_t col_gray   = 0xFF111111; // AARRGGBB
static const uint64_t col_gray1  = 0xFF222222; // Truth be told idk
static const uint64_t col_gray2  = 0xFF444444; // Why i kept these here
static const uint64_t col_gray3  = 0xFFBBBBBB; // But they were in my 
static const uint64_t col_gray4  = 0xFFEEEEEE; // Dwm config and i
static const uint64_t col_gray5  = 0xFF555753; // Like they way they 
static const uint64_t col_black  = 0xFF000000; // Looked :3
static const uint64_t col_cyan   = 0xFF005577;
static const uint64_t col_red    = 0xFFFE3F09;
static const uint64_t col_blue   = 0xFF0000FF;
static const uint64_t col_blue1  = 0xFF3366FF;
static const uint64_t col_blue2  = 0xFF222233;
static const uint64_t col_white  = 0xFFFFFFFF;

enum Schemes { SchemeNorm, SchemeSel };
enum Colors { FOREG, BACKG, BORDER };

static const uint64_t colors[2][3]      = {
	//     fg         bg         border  
	[SchemeNorm] = { col_gray3, col_gray, col_gray2 }, //minty Normal scheme
	[SchemeSel] = { col_black, col_red, col_red, },    //minty Selected scheme
};

static const uint8_t debug = 0;        /// Debug vars
static const char debugf[] = "~/dlog";
