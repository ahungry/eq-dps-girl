#include <windows.h>
#include <gdiplus.h>

// For dll inclusion
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>
#include "resource.h"

#include <fstream>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string>
#include <ctime>
#include <iomanip>
#include <chrono>

#define _MSC_VER 1

#include "../cppparser/parser.cpp"

// #pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

ULONG_PTR gdiplusToken;
GdiplusStartupInput gdiplusStartupInput;
wchar_t tempDir[MAX_PATH]; // = L".\\";

int animation_delay = 100;
int currentImage = 0;
int sleeping = 0;
float scale = 1.0f;
int bubbleX = -30;
int bubbleY = -60;

// Helper function to get a formatted error message for a given error code
std::wstring
GetFormattedErrorMessage(DWORD error)
{
  LPWSTR buffer = nullptr;
  DWORD size = FormatMessageW
    (
     FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
     nullptr,
     error,
     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
     (LPWSTR)&buffer,
     0,
     nullptr
     );

  std::wstring message;
  if (size > 0)
    {
      message = buffer;
      // Trim trailing newline characters
      while (!message.empty() && (message.back() == L'\n' || message.back() == L'\r')) {
        message.pop_back();
      }
      LocalFree(buffer); // Free the allocated buffer
    }
  else
    {
      std::wostringstream oss;
      oss << L"Error code: " << error;
      message = oss.str();
    }
  return message;
}

// Function to get the current timestamp for logging
std::wstring
GetTimestamp()
{
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::tm now_tm;
  localtime_s(&now_tm, &now_c);
  std::wostringstream woss;
  woss << std::put_time(&now_tm, L"%Y-%m-%d %H:%M:%S");
  return woss.str();
}

int
spit(std::wstring fileName, std::wstring msg)
{
  FILE* file;
  errno_t err = _wfopen_s(&file, fileName.c_str(), L"a+"); // Open for appending

  if (file == nullptr) return -1;
  if (err > 0) return -1;

  fwprintf(file, msg.c_str());
  fflush(file);
  fclose(file);

  return 0;
}

int
spit(std::string fileName, std::string msg)
{
  FILE* file;
  errno_t err = fopen_s(&file, fileName.c_str(), "a+"); // Open for appending

  if (file == nullptr) return -1;
  if (err > 0) return -1;

  fprintf(file, msg.c_str());
  fflush(file);
  fclose(file);

  return 0;
}

int
log(std::wstring msg)
{
  return spit(L"dpsgirl.log", GetTimestamp() + L" - " + msg + L"\n");
}

int
log(std::string msg)
{
  std::wstring wstr = GetTimestamp();
  char buffer[100];
  std::wcstombs(buffer, wstr.c_str(), sizeof(buffer));
  std::string timestamp(buffer);
  return spit("dpsgirl.log", timestamp + " - " + msg + "\n");
}

void
logError(std::wstring msg)
{
  DWORD lastError = GetLastError();
  std::wstring errorMessage = GetFormattedErrorMessage(lastError);
  log(msg + L": [" + errorMessage + L"]");
}

int
addToConfig(std::wstring msg)
{
  return spit(L"dpsgirl.conf", msg + L"\n");
}

bool
ExtractResourceToFile(LPWSTR resourceName, const wchar_t* filePath)
{
  HRSRC hRes = FindResource(GetModuleHandle(NULL), resourceName, RT_RCDATA);
  if (hRes == NULL)
    {
      logError(L"FindResource failed");
      return false;
    }

  //Load the resource.
  HGLOBAL hResData = LoadResource(GetModuleHandle(NULL), hRes);
  if (hResData == NULL)
    {
      logError(L"LoadResource failed");
      return false;
    }

  //Get a pointer to the resource data.
  const void* pData = LockResource(hResData);
  if (pData == NULL)
    {
      logError(L"LockResource failed");
      return false;
    }

  //Get the size of the resource.
  DWORD dataSize = SizeofResource(GetModuleHandle(NULL), hRes);
  if (dataSize == 0)
    {
      logError(L"SizeofResource failed");
      return false;
    }

  //Write the resource data to a file.
  std::ofstream file(filePath, std::ios::binary);
  if (!file.is_open())
    {
      // logError(L"Failed to open for writing: " + filePath);
      logError(L"Failed to open for writing: ");
      return false;
    }
  file.write(static_cast<const char*>(pData), dataSize);
  file.close();

  //Clean up.  (Unlock, Free, no need to free hRes)
  UnlockResource(hResData);
  // if (!UnlockResource(hResData)) {
  //   logError(L"UnlockResource failed");
  // }
  return true;
}

void
setClippingRegion(HWND hWnd)
{
  RECT winrect, rect;
  POINT client_origin;

  // Get window and client rects
  GetWindowRect(hWnd, &winrect);
  GetClientRect(hWnd, &rect);

  // Get the client origin in window coordinates
  client_origin.x = 0; client_origin.y = 0;
  ClientToScreen(hWnd, &client_origin);
  client_origin.x -= winrect.left;
  client_origin.y -= winrect.top;

  // Generate this via the contour.bin file (compile as needed)
  POINT points[] = {{170, 0}, {167, 3}, {167, 4}, {166, 5}, {166, 6}, {165, 7}, {165, 9}, {164, 10}, {164, 11}, {163, 12}, {163, 14}, {162, 15}, {162, 16}, {161, 17}, {161, 19}, {160, 20}, {160, 23}, {159, 24}, {159, 28}, {157, 30}, {157, 36}, {156, 37}, {155, 37}, {153, 35}, {134, 35}, {133, 36}, {127, 36}, {126, 37}, {124, 37}, {123, 38}, {121, 38}, {120, 39}, {118, 39}, {117, 40}, {116, 40}, {115, 41}, {113, 41}, {112, 42}, {110, 42}, {108, 44}, {107, 44}, {106, 45}, {105, 45}, {103, 47}, {102, 47}, {101, 48}, {99, 48}, {98, 49}, {97, 49}, {95, 51}, {94, 51}, {93, 50}, {93, 46}, {94, 45}, {94, 43}, {95, 42}, {95, 40}, {103, 32}, {111, 32}, {113, 30}, {113, 25}, {109, 21}, {105, 21}, {104, 20}, {100, 20}, {99, 21}, {97, 21}, {96, 22}, {95, 22}, {86, 31}, {86, 32}, {83, 35}, {83, 36}, {82, 37}, {82, 39}, {81, 40}, {81, 43}, {80, 44}, {80, 51}, {81, 52}, {81, 54}, {82, 55}, {82, 56}, {83, 57}, {83, 58}, {84, 59}, {83, 60}, {81, 60}, {80, 59}, {79, 59}, {77, 57}, {76, 57}, {75, 56}, {74, 56}, {72, 54}, {71, 54}, {70, 53}, {69, 53}, {68, 52}, {67, 52}, {66, 51}, {65, 51}, {64, 50}, {62, 50}, {61, 49}, {57, 49}, {55, 51}, {55, 59}, {56, 60}, {56, 62}, {57, 63}, {57, 66}, {58, 67}, {58, 68}, {54, 72}, {54, 73}, {52, 75}, {52, 76}, {51, 77}, {51, 78}, {50, 79}, {50, 80}, {49, 81}, {49, 83}, {48, 84}, {48, 86}, {47, 87}, {47, 90}, {46, 91}, {46, 94}, {45, 95}, {45, 98}, {44, 99}, {44, 105}, {43, 106}, {43, 118}, {42, 119}, {42, 141}, {41, 142}, {41, 144}, {40, 145}, {40, 146}, {39, 147}, {39, 149}, {38, 150}, {38, 152}, {37, 153}, {37, 154}, {36, 155}, {36, 158}, {35, 159}, {35, 160}, {34, 161}, {34, 163}, {33, 164}, {33, 165}, {32, 166}, {32, 167}, {31, 168}, {31, 169}, {29, 171}, {29, 172}, {28, 173}, {28, 174}, {26, 176}, {26, 177}, {25, 178}, {25, 179}, {23, 181}, {23, 182}, {22, 183}, {22, 184}, {21, 185}, {21, 186}, {18, 189}, {18, 193}, {17, 194}, {17, 195}, {15, 197}, {15, 199}, {14, 200}, {14, 204}, {13, 205}, {13, 211}, {14, 212}, {14, 216}, {13, 217}, {13, 221}, {14, 222}, {14, 223}, {15, 224}, {15, 227}, {16, 228}, {16, 231}, {20, 235}, {20, 236}, {28, 244}, {29, 244}, {33, 248}, {33, 255}, {34, 256}, {34, 260}, {35, 261}, {35, 263}, {36, 264}, {36, 265}, {37, 266}, {37, 267}, {41, 271}, {41, 272}, {45, 276}, {46, 276}, {50, 280}, {51, 280}, {52, 281}, {53, 281}, {54, 282}, {55, 282}, {56, 283}, {57, 283}, {58, 284}, {60, 284}, {61, 285}, {61, 291}, {62, 292}, {62, 293}, {64, 295}, {63, 296}, {61, 296}, {60, 297}, {59, 297}, {55, 293}, {55, 292}, {52, 289}, {51, 289}, {50, 288}, {49, 288}, {45, 284}, {45, 283}, {44, 282}, {44, 281}, {43, 280}, {43, 279}, {42, 278}, {42, 277}, {41, 276}, {41, 275}, {38, 272}, {33, 272}, {28, 277}, {28, 290}, {29, 291}, {29, 292}, {30, 293}, {30, 294}, {29, 295}, {23, 295}, {21, 293}, {10, 293}, {9, 294}, {3, 294}, {0, 297}, {0, 318}, {2, 318}, {3, 317}, {4, 318}, {4, 329}, {9, 334}, {9, 340}, {14, 345}, {14, 348}, {18, 352}, {20, 352}, {21, 353}, {22, 353}, {23, 354}, {25, 354}, {26, 355}, {36, 355}, {37, 354}, {38, 354}, {40, 352}, {41, 352}, {43, 350}, {44, 350}, {45, 349}, {46, 349}, {49, 346}, {50, 347}, {52, 347}, {53, 348}, {57, 348}, {58, 349}, {59, 349}, {61, 351}, {69, 351}, {70, 352}, {71, 352}, {73, 354}, {74, 354}, {79, 359}, {80, 359}, {83, 362}, {84, 362}, {87, 365}, {88, 365}, {91, 368}, {92, 368}, {97, 373}, {98, 373}, {100, 375}, {101, 375}, {102, 376}, {103, 376}, {104, 377}, {105, 377}, {106, 378}, {107, 378}, {108, 379}, {110, 379}, {111, 380}, {112, 380}, {113, 381}, {114, 381}, {115, 382}, {125, 382}, {128, 379}, {128, 378}, {129, 377}, {129, 375}, {130, 374}, {130, 372}, {131, 371}, {131, 370}, {132, 369}, {132, 368}, {133, 367}, {133, 366}, {134, 365}, {134, 364}, {135, 363}, {135, 362}, {136, 361}, {136, 359}, {137, 358}, {137, 357}, {139, 355}, {140, 355}, {141, 356}, {142, 356}, {143, 357}, {149, 357}, {150, 356}, {151, 356}, {153, 358}, {154, 358}, {155, 359}, {155, 360}, {154, 361}, {154, 363}, {153, 364}, {153, 366}, {152, 367}, {152, 369}, {151, 370}, {151, 376}, {150, 377}, {150, 380}, {149, 381}, {149, 383}, {274, 383}, {274, 381}, {273, 380}, {273, 379}, {268, 374}, {267, 374}, {266, 373}, {266, 372}, {265, 371}, {265, 370}, {264, 369}, {264, 368}, {263, 367}, {263, 366}, {262, 365}, {262, 364}, {260, 362}, {260, 361}, {259, 360}, {259, 359}, {258, 358}, {258, 357}, {256, 355}, {256, 354}, {255, 353}, {255, 352}, {254, 351}, {254, 350}, {253, 349}, {253, 348}, {251, 346}, {251, 342}, {250, 341}, {252, 339}, {252, 332}, {251, 331}, {251, 328}, {250, 327}, {251, 326}, {251, 322}, {250, 321}, {250, 320}, {253, 317}, {253, 313}, {254, 312}, {254, 308}, {255, 307}, {255, 306}, {256, 305}, {256, 302}, {257, 301}, {257, 300}, {258, 299}, {260, 299}, {264, 295}, {264, 291}, {265, 290}, {265, 287}, {266, 286}, {266, 283}, {268, 281}, {269, 282}, {269, 286}, {270, 287}, {270, 289}, {271, 290}, {271, 291}, {272, 292}, {272, 293}, {273, 294}, {273, 295}, {275, 297}, {275, 298}, {280, 303}, {280, 304}, {282, 306}, {283, 306}, {286, 309}, {287, 309}, {288, 310}, {289, 310}, {291, 312}, {292, 312}, {293, 313}, {294, 313}, {295, 314}, {297, 314}, {298, 315}, {299, 315}, {300, 316}, {302, 316}, {306, 320}, {307, 320}, {310, 323}, {313, 323}, {315, 325}, {325, 325}, {327, 323}, {327, 306}, {325, 304}, {324, 304}, {321, 301}, {318, 301}, {313, 296}, {310, 296}, {308, 294}, {306, 294}, {305, 293}, {306, 292}, {308, 292}, {310, 290}, {310, 283}, {311, 282}, {313, 284}, {317, 284}, {319, 282}, {319, 281}, {321, 279}, {321, 278}, {322, 277}, {322, 276}, {323, 275}, {323, 273}, {324, 272}, {324, 268}, {325, 267}, {325, 265}, {326, 264}, {326, 263}, {327, 262}, {327, 261}, {328, 260}, {328, 259}, {331, 256}, {331, 255}, {338, 248}, {338, 247}, {342, 243}, {342, 240}, {343, 239}, {343, 236}, {344, 235}, {344, 234}, {345, 233}, {345, 229}, {344, 228}, {344, 224}, {345, 223}, {345, 217}, {344, 216}, {344, 212}, {343, 211}, {343, 209}, {341, 207}, {341, 206}, {340, 205}, {340, 201}, {337, 198}, {337, 197}, {336, 196}, {336, 195}, {335, 194}, {335, 193}, {333, 191}, {333, 190}, {332, 189}, {332, 188}, {330, 186}, {330, 185}, {329, 184}, {329, 183}, {327, 181}, {327, 179}, {325, 177}, {325, 175}, {324, 174}, {324, 157}, {321, 154}, {321, 152}, {319, 150}, {319, 147}, {317, 145}, {317, 141}, {314, 138}, {314, 132}, {312, 130}, {308, 130}, {307, 129}, {307, 128}, {305, 126}, {304, 126}, {303, 125}, {302, 125}, {300, 123}, {300, 122}, {298, 120}, {298, 119}, {297, 118}, {297, 117}, {296, 116}, {296, 115}, {295, 114}, {295, 113}, {294, 112}, {294, 111}, {293, 110}, {293, 109}, {291, 107}, {291, 106}, {290, 105}, {290, 104}, {288, 102}, {288, 101}, {289, 100}, {290, 100}, {292, 102}, {293, 102}, {295, 104}, {296, 104}, {304, 112}, {304, 113}, {305, 114}, {305, 117}, {307, 119}, {312, 119}, {314, 117}, {314, 103}, {305, 94}, {304, 94}, {299, 89}, {298, 89}, {295, 86}, {294, 86}, {291, 83}, {290, 83}, {288, 81}, {287, 81}, {286, 80}, {285, 80}, {284, 79}, {283, 79}, {282, 78}, {281, 78}, {279, 76}, {278, 76}, {277, 75}, {276, 75}, {275, 74}, {274, 74}, {273, 73}, {272, 73}, {270, 71}, {269, 71}, {266, 68}, {266, 67}, {265, 66}, {265, 65}, {263, 63}, {263, 62}, {262, 61}, {262, 60}, {260, 58}, {260, 57}, {258, 55}, {258, 54}, {256, 52}, {256, 51}, {253, 48}, {253, 47}, {250, 44}, {250, 43}, {246, 39}, {246, 38}, {232, 24}, {231, 24}, {228, 21}, {227, 21}, {225, 19}, {224, 19}, {222, 17}, {221, 17}, {220, 16}, {219, 16}, {218, 15}, {216, 15}, {215, 14}, {201, 14}, {200, 15}, {198, 15}, {197, 16}, {196, 16}, {194, 18}, {193, 18}, {192, 19}, {191, 19}, {189, 21}, {187, 19}, {187, 18}, {185, 16}, {185, 15}, {184, 14}, {184, 13}, {183, 12}, {183, 11}, {182, 10}, {182, 9}, {180, 7}, {180, 6}, {179, 5}, {179, 4}, {177, 2}, {177, 1}, {176, 0}};
  int numPoints = 683;

  // Iterate through the array and scale each point
  for (int i = 0; i < numPoints; ++i)
    {
      points[i].x = static_cast<LONG>(points[i].x * scale) - bubbleX; // Scale x and convert back to LONG
      points[i].y = static_cast<LONG>(points[i].y * scale) - bubbleY; // Scale y and convert back to LONG
    }

  HRGN hRgn = CreatePolygonRgn(points, numPoints, ALTERNATE);

  POINT bubblePoints[] = {{162, 13}, {162, 14}, {163, 13}, {165, 13}, {166, 14}, {166, 16}, {165, 17}, {159, 17}, {158, 18}, {156, 18}, {155, 19}, {153, 19}, {152, 20}, {150, 20}, {149, 21}, {147, 21}, {146, 22}, {144, 22}, {143, 23}, {140, 23}, {139, 24}, {137, 24}, {136, 25}, {135, 25}, {134, 26}, {131, 26}, {130, 27}, {128, 27}, {127, 28}, {125, 28}, {124, 29}, {121, 29}, {120, 30}, {119, 30}, {118, 31}, {114, 31}, {113, 32}, {110, 32}, {109, 33}, {107, 33}, {105, 35}, {104, 34}, {103, 34}, {102, 35}, {101, 35}, {100, 36}, {98, 36}, {97, 37}, {95, 37}, {94, 38}, {91, 38}, {90, 39}, {88, 39}, {87, 40}, {86, 40}, {85, 41}, {82, 41}, {81, 42}, {78, 42}, {77, 43}, {75, 43}, {74, 44}, {72, 44}, {71, 45}, {69, 45}, {68, 46}, {66, 46}, {65, 47}, {62, 47}, {61, 48}, {59, 48}, {58, 49}, {56, 49}, {55, 50}, {54, 50}, {53, 51}, {50, 51}, {49, 52}, {47, 52}, {45, 54}, {42, 54}, {41, 55}, {40, 55}, {39, 56}, {37, 56}, {36, 57}, {35, 57}, {34, 58}, {31, 58}, {30, 59}, {28, 59}, {27, 60}, {25, 60}, {24, 61}, {22, 61}, {21, 62}, {19, 62}, {18, 63}, {17, 63}, {16, 64}, {15, 63}, {15, 62}, {14, 62}, {13, 63}, {12, 63}, {12, 65}, {13, 65}, {14, 66}, {13, 67}, {13, 70}, {12, 71}, {13, 72}, {13, 75}, {14, 76}, {14, 77}, {15, 78}, {15, 79}, {16, 80}, {16, 81}, {17, 82}, {17, 83}, {18, 84}, {18, 85}, {19, 86}, {19, 87}, {20, 88}, {20, 89}, {21, 90}, {21, 93}, {22, 93}, {23, 94}, {23, 95}, {24, 96}, {24, 97}, {25, 98}, {25, 99}, {26, 100}, {26, 101}, {27, 102}, {27, 104}, {29, 106}, {29, 107}, {30, 108}, {30, 110}, {32, 112}, {32, 113}, {34, 115}, {34, 117}, {36, 119}, {36, 121}, {38, 123}, {38, 124}, {39, 125}, {39, 126}, {40, 127}, {40, 128}, {41, 129}, {41, 130}, {42, 131}, {42, 132}, {45, 135}, {46, 135}, {47, 136}, {48, 136}, {49, 137}, {54, 137}, {55, 136}, {59, 136}, {60, 135}, {61, 136}, {62, 136}, {63, 135}, {64, 136}, {65, 136}, {66, 137}, {67, 137}, {68, 138}, {69, 138}, {71, 140}, {73, 140}, {76, 143}, {77, 143}, {78, 144}, {79, 144}, {80, 145}, {81, 145}, {83, 147}, {84, 147}, {85, 148}, {86, 148}, {86, 128}, {88, 126}, {91, 126}, {92, 125}, {95, 125}, {96, 124}, {98, 124}, {99, 123}, {102, 123}, {103, 122}, {105, 122}, {106, 121}, {109, 121}, {110, 120}, {113, 120}, {114, 119}, {116, 119}, {117, 118}, {120, 118}, {121, 117}, {124, 117}, {125, 116}, {127, 116}, {128, 115}, {131, 115}, {132, 114}, {134, 114}, {135, 113}, {138, 113}, {139, 112}, {142, 112}, {143, 111}, {145, 111}, {146, 110}, {149, 110}, {150, 109}, {152, 109}, {153, 108}, {155, 108}, {156, 107}, {158, 107}, {159, 106}, {162, 106}, {163, 105}, {165, 105}, {166, 104}, {169, 104}, {170, 103}, {171, 103}, {173, 101}, {173, 100}, {174, 99}, {174, 98}, {175, 97}, {175, 84}, {174, 83}, {174, 60}, {173, 59}, {173, 37}, {172, 36}, {172, 25}, {171, 24}, {171, 22}, {169, 20}, {169, 19}, {168, 18}, {167, 18}, {166, 17}, {167, 16}, {169, 16}, {169, 14}, {167, 14}, {166, 13}};
  int bubbleNumPoints = 263;

  for (int i = 0; i < bubbleNumPoints; ++i)
    {
      bubblePoints[i].x = static_cast<LONG>(bubblePoints[i].x * scale);
      bubblePoints[i].y = static_cast<LONG>(bubblePoints[i].y * scale);
    }

  HRGN hRgnBubble = CreatePolygonRgn(bubblePoints, bubbleNumPoints, ALTERNATE);
  CombineRgn(hRgn, hRgn, hRgnBubble, RGN_OR);

  SetWindowRgn(hWnd, hRgn, 1);
}

std::wstring
getDpsAsString()
{
  updateStats();

  if (hasNoLogs())
    return L"Logs?!";

  int dps = getDps();

  if (dps > 0)
    sleeping = 0;

  if (sleeping or dps == 0)
    {
      sleeping = 1;
      return L"ZZZzzz";
    }

  // int dps = (rand() % 9999) + 1;
  return std::to_wstring(dps);
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT ps;
  static Bitmap* bitmap1 = NULL;
  static Bitmap* bitmap2 = NULL;
  static Bitmap* bitmapB = NULL;
  static Bitmap* bitmapZ = NULL;
  static Bitmap* bitmapU = NULL;

  switch (message)
    {
    case WM_CREATE:
      {
        // Initialize GDI+
        Status startupStatus = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        if (startupStatus != Ok) {
          MessageBox(hwnd, L"Failed to initialize GDI+.", L"Error", MB_OK);
          return -1;
        }
        std::wstring image1path = std::wstring(tempDir) + L"__girl1.png";
        std::wstring image2path = std::wstring(tempDir) + L"__girl2.png";
        std::wstring imageBpath = std::wstring(tempDir) + L"__girlb.png";
        std::wstring imageZpath = std::wstring(tempDir) + L"__girlz.png";
        std::wstring imageUpath = std::wstring(tempDir) + L"__bubble200.png";

        log(L"Image 1 Path: " + image1path);

        bitmap1 = Bitmap::FromFile(image1path.c_str());
        if (bitmap1->GetLastStatus() != Ok)
          {
            logError(L"Failed to load image1.png!");
            MessageBox(hwnd, L"Failed to load image1.png.", L"Error", MB_OK);
            return -1;
          }

        bitmap2 = Bitmap::FromFile(image2path.c_str());
        if (bitmap2->GetLastStatus() != Ok)
          {
            logError(L"Failed to load image2.png!");
            MessageBox(hwnd, L"Failed to load image2.png.", L"Error", MB_OK);
            return -1;
          }

        bitmapB = Bitmap::FromFile(imageBpath.c_str());
        if (bitmapB->GetLastStatus() != Ok)
          {
            logError(L"Failed to load imageB.png!");
            MessageBox(hwnd, L"Failed to load imageB.png.", L"Error", MB_OK);
            return -1;
          }

        bitmapZ = Bitmap::FromFile(imageZpath.c_str());
        if (bitmapZ->GetLastStatus() != Ok)
          {
            logError(L"Failed to load imageZ.png!");
            MessageBox(hwnd, L"Failed to load imageZ.png.", L"Error", MB_OK);
            return -1;
          }

        bitmapU = Bitmap::FromFile(imageUpath.c_str());
        if (bitmapU->GetLastStatus() != Ok)
          {
            logError(L"Failed to load imageU.png!");
            MessageBox(hwnd, L"Failed to load imageU.png.", L"Error", MB_OK);
            return -1;
          }

        //Set window as layered.
        SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                         GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

        // Make the window fully transparent.
        // SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
        SetLayeredWindowAttributes(hwnd, RGB(1, 1, 1), 0, LWA_COLORKEY);
        SetTimer(hwnd, 1, animation_delay, nullptr);
        break;
      }
    case WM_PAINT:
      {
        hdc = BeginPaint(hwnd, &ps);
        HDC hdcMem = CreateCompatibleDC(hdc); // Create an off-screen DC
        RECT rect;
        GetClientRect(hwnd, &rect);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top); // Create an off-screen bitmap
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem); // Select the bitmap into the off-screen DC
        Graphics graphicsMem(hdcMem); // Create a Graphics object for the off-screen DC

        // 1. Redraw the background (using the color key color)
        HBRUSH hbrBackground = CreateSolidBrush(RGB(1, 1, 1)); // Assuming your color key is RGB(1,1,1)
        FillRect(hdcMem, &rect, hbrBackground);
        DeleteObject(hbrBackground);

        //Draw the image onto the off-screen DC.
        if (currentImage == 0)
          {
            graphicsMem.DrawImage(bitmap1, 0 - bubbleX, 0 - bubbleY,
                                  static_cast<LONG>(bitmap1->GetWidth() * scale),
                                  static_cast<LONG>(bitmap1->GetHeight() * scale));
          }
        else if (currentImage == 1)
          {
            graphicsMem.DrawImage(bitmap2, 0 - bubbleX, 0 - bubbleY,
                                  static_cast<LONG>(bitmap2->GetWidth() * scale),
                                  static_cast<LONG>(bitmap2->GetHeight() * scale));
          }
        else if (currentImage == 2)
          {
            graphicsMem.DrawImage(bitmapB, 0 - bubbleX, 0 - bubbleY,
                                  static_cast<LONG>(bitmapB->GetWidth() * scale),
                                  static_cast<LONG>(bitmapB->GetHeight() * scale));
          }
        else if (currentImage == 3)
          {
            graphicsMem.DrawImage(bitmapZ, 0 - bubbleX, 0 - bubbleY,
                                  static_cast<LONG>(bitmapZ->GetWidth() * scale),
                                  static_cast<LONG>(bitmapZ->GetHeight() * scale));
          }

        graphicsMem.DrawImage(bitmapU, 0, 0,
                              static_cast<LONG>(bitmapU->GetWidth() * scale),
                              static_cast<LONG>(bitmapU->GetHeight() * scale));

        // --- Drawing the text ---
        FontFamily fontFamily(L"Arial"); // You can choose a different font
        Font font(&fontFamily, 30 * scale, FontStyleRegular, UnitPixel); // Adjust size as needed
        SolidBrush textBrush(Color(255, 0, 0, 0)); // Black color (ARGB)
        PointF textPosition(40 * scale, 70 * scale); // Adjust position within the bubble

        std::wstring textToDraw = getDpsAsString();
        graphicsMem.DrawString(
                               textToDraw.c_str(),
                               -1, // Draw the entire string
                               &font,
                               textPosition,
                               &textBrush
                               );

        FontFamily fontFamily2(L"Arial"); // You can choose a different font
        Font font2(&fontFamily2, 14 * scale, FontStyleRegular, UnitPixel); // Adjust size as needed
        SolidBrush textBrush2(Color(255, 0, 0, 0)); // Black color (ARGB)
        PointF textPosition2(130 * scale, 60 * scale); // Adjust position within the bubble

        std::wstring textToDraw2 = sleeping ? L"zzz" : L"dps";
        graphicsMem.DrawString(
                               textToDraw2.c_str(),
                               -1, // Draw the entire string
                               &font2,
                               textPosition2,
                               &textBrush2
                               );
        // --- End of text drawing ---

        // Copy the entire off-screen buffer to the screen
        BitBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcMem, 0, 0, SRCCOPY);

        // Clean up
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
        break;
      }
    case WM_TIMER:
      {
        if (wParam == 1)
          {
            if (currentImage > 1)
              currentImage = 0;

            currentImage ^= 1;

            int randomInRange = (rand() % 100) + 1;
            if (randomInRange < 10)
              currentImage = 2;

            if (sleeping == 1)
              currentImage = 3;

            InvalidateRect(hwnd, nullptr, FALSE);
            UpdateWindow(hwnd);
          }
        break;
      }
    case WM_DROPFILES:
      {
        HDROP hDrop = (HDROP)wParam;
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
        std::vector<wchar_t> filePathBuffer(MAX_PATH);

        for (UINT i = 0; i < fileCount; ++i)
          {
            if (DragQueryFile(hDrop, i, filePathBuffer.data(), MAX_PATH) > 0)
              {
                // std::wcout << L"Dropped file: " << filePathBuffer.data() << std::endl;
                // Process the file path here (e.g., open the file)
                log(L"Dropped file: " + std::wstring(filePathBuffer.data()));
                addToConfig(std::wstring(filePathBuffer.data()));
              }
            else
              {
                logError(L"Error getting dropped file path.");
              }
          }
        DragFinish(hDrop); // Release the HDROP handle
        break;
      }
    case WM_LBUTTONDOWN:
      sleeping ^= 1;
      break;
    case WM_RBUTTONUP:
      PostQuitMessage(0);
      break;
    case WM_DESTROY:
      KillTimer(hwnd, 1);
      delete bitmap1;
      delete bitmap2;
      delete bitmapB;
      delete bitmapZ;
      delete bitmapU;
      GdiplusShutdown(gdiplusToken);
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hwnd, message, wParam, lParam);
    }
  return 0;
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
  log(L"dpsgirl start");
  srand(static_cast<unsigned int>(time(0)));

  // Define a temporary directory (you might want to use GetTempPath)
  if (GetTempPathW(MAX_PATH, tempDir) == 0)
    {
      logError(L"GetTempPathW failed");
      return -1;
    }

  std::wstring image1path = std::wstring(tempDir) + L"__girl1.png";
  std::wstring image2path = std::wstring(tempDir) + L"__girl2.png";
  std::wstring imageBpath = std::wstring(tempDir) + L"__girlb.png";
  std::wstring imageZpath = std::wstring(tempDir) + L"__girlz.png";
  std::wstring imageUpath = std::wstring(tempDir) + L"__bubble200.png";

  if (!ExtractResourceToFile(MAKEINTRESOURCE(IDR_IMAGE_1), image1path.c_str()))
    {
      logError(L"image1 extract failed");
      return -1;
    }

  if (!ExtractResourceToFile(MAKEINTRESOURCE(IDR_IMAGE_2), image2path.c_str()))
    {
      logError(L"image2 extract failed");
      return -1;
    }

  if (!ExtractResourceToFile(MAKEINTRESOURCE(IDR_IMAGE_B), imageBpath.c_str()))
    {
      logError(L"imageB extract failed");
      return -1;
    }

  if (!ExtractResourceToFile(MAKEINTRESOURCE(IDR_IMAGE_Z), imageZpath.c_str()))
    {
      logError(L"imageZ extract failed");
      return -1;
    }

  if (!ExtractResourceToFile(MAKEINTRESOURCE(IDR_IMAGE_U), imageUpath.c_str()))
    {
      logError(L"imageU extract failed");
      return -1;
    }

  // ... (Window class setup)
  WNDCLASSEXW wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style          = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc    = WndProc;
  wcex.cbClsExtra     = 0;
  wcex.cbWndExtra     = 0;
  wcex.hInstance      = hInstance;
  wcex.hIcon          = NULL; //No Icon
  wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground  = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));  // Black background (ignored because of layered)
  wcex.lpszMenuName   = NULL; // No menu
  wcex.lpszClassName  = L"TransparentWindowClass";
  wcex.hIconSm        = NULL; //No small icon

  RegisterClassExW(&wcex);


  // Get screen dimensions.
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  // NOTE: Update these values if we change the base image size
  int scaledImageWidth = static_cast<LONG>(350 * scale) - bubbleX;
  int scaledImageHeight = static_cast<LONG>(384 * scale) - bubbleY;

  // Desired window size (adjust as needed).
  int windowWidth = scaledImageWidth;
  int windowHeight = scaledImageHeight;

  // Calculate the position for the bottom-right corner.
  int x = screenWidth - windowWidth;
  int y = screenHeight - windowHeight;

  //Use  WS_POPUP to remove title bar and borders.
  HWND hwnd = CreateWindowExW
    (
     WS_EX_LAYERED |
     WS_EX_TOPMOST |
     WS_EX_ACCEPTFILES,
     // Allows click-through, but makes clicking on it impossible
     // WS_EX_TRANSPARENT, // Extended styles:  Layered window
     L"TransparentWindowClass",      // Class name
     L"Transparent Window",          // Window title (will not be visible)
     WS_POPUP, // Window style:  Popup window (no border/title bar)
     x, // CW_USEDEFAULT,
     y, // CW_USEDEFAULT,
     scaledImageWidth,
     scaledImageHeight,
     nullptr,
     nullptr,
     hInstance,
     nullptr
     );

  if (!hwnd)
    {
      return FALSE;
    }

  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  setClippingRegion(hwnd);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

  DeleteFileW(image1path.c_str());
  DeleteFileW(image2path.c_str());
  DeleteFileW(imageBpath.c_str());
  DeleteFileW(imageZpath.c_str());
  DeleteFileW(imageUpath.c_str());

  log(L"dpsgirl end");

  return (int)msg.wParam;
}
