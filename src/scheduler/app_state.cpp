#include "app_state.h"

std::vector<SItemSchedule*> g_vecSchedule;

std::wstring content_filename = L"D:\\Eness_Projects\\2038-Wellesley-Library\\Pixile_Sketch\\Packed\\2038-Wellesley_auto.pxl";
std::wstring orig_content_filename = L"D:\\Eness_Projects\\2038-Wellesley-Library\\Pixile_Sketch\\Packed\\2038-Wellesley_auto.pxl";
std::wstring pixile_location = L"C:\\Eness_Projects\\pixile\\Bin\\Studio\\Release\\";
std::wstring alt_pixile_location = L"C:\\Eness_Projects\\pixile\\Bin\\Studio\\Release\\";
std::wstring client_filename = L"";

int start_hour = 6;
int start_minute = 55;
int end_hour = 20;
int end_minute = 55;

bool g_bUseAlternatePlayer = false;
bool g_bCanUseAlternatePlayer = false;
bool g_bUseMouse = false;

sScreeninfo screeninfo{ 0,0,1024,768 };

bool bDays[7] = {};

bool compareByStartDate(const SItemSchedule* a, const SItemSchedule* b) {
    tm a_tm = a->startDate;
    tm b_tm = b->startDate;
    return std::mktime(&a_tm) < std::mktime(&b_tm);
}
