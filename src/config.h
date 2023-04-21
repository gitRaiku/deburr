#include <stdint.h>

static const char statusPath[] = "/tmp/deburrstat";

static const double padding = 0.3; // times font height
                                   //
static const char *fonts[2] = { "JetBrainsMono:size=18", 
                                "Koruri:size=18" }; // Max 2 fonts

static const wchar_t *tags[] = { L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9" };

static const uint64_t col_gray   = 0xFF111111; // AARRGGBB
static const uint64_t col_gray1  = 0xFF222222;
static const uint64_t col_gray2  = 0xFF444444;
static const uint64_t col_gray3  = 0xFFBBBBBB;
static const uint64_t col_gray4  = 0xFFEEEEEE;
static const uint64_t col_gray5  = 0xFF555753;
static const uint64_t col_black  = 0xFF000000;
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
	[SchemeNorm] = { col_gray3, col_gray, col_gray2 }, //minty schemenorm
	[SchemeSel] = { col_black, col_red, col_red, }, //minty schemesel
};

static const uint8_t debug = 0;
static const char debugf[] = "~/dlog";
