#include <windows.h>
#include <gdiplus.h>

// For dll inclusion
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

// #pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

ULONG_PTR gdiplusToken;
GdiplusStartupInput gdiplusStartupInput;
wchar_t tempDir[MAX_PATH]; // = L".\\";

int currentImage = 0;
float scale = 0.5f;

// Helper function to get a formatted error message for a given error code
std::wstring GetFormattedErrorMessage(DWORD error) {
    LPWSTR buffer = nullptr;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPWSTR)&buffer,
        0,
        nullptr
    );

    std::wstring message;
    if (size > 0) {
        message = buffer;
        // Trim trailing newline characters
        while (!message.empty() && (message.back() == L'\n' || message.back() == L'\r')) {
            message.pop_back();
        }
        LocalFree(buffer); // Free the allocated buffer
    } else {
        std::wostringstream oss;
        oss << L"Error code: " << error;
        message = oss.str();
    }
    return message;
}

// Function to get the current timestamp for logging
std::wstring GetTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::tm now_tm;
  localtime_s(&now_tm, &now_c);
  std::wostringstream woss;
  woss << std::put_time(&now_tm, L"%Y-%m-%d %H:%M:%S");
  return woss.str();
}

int log(std::wstring msg)
{
  std::wstring logFileName = L"dpsgirl.log";
  FILE* logFile;
  errno_t err = _wfopen_s(&logFile, logFileName.c_str(), L"a+"); // Open for appending

  if (logFile == nullptr) return -1;
  if (err > 0) return -1;

  fwprintf(logFile, L"%ls - %ls\n", GetTimestamp().c_str(), msg.c_str());
  fflush(logFile); // Ensure the data is written immediately
  fclose(logFile);

  return 0;
}

void logError(std::wstring msg)
{
    DWORD lastError = GetLastError();
    std::wstring errorMessage = GetFormattedErrorMessage(lastError);
    log(msg + L": [" + errorMessage + L"]");
}

bool ExtractResourceToFile(LPCSTR resourceName, const wchar_t* filePath)
{
  HRSRC hRes = FindResource(GetModuleHandle(NULL), resourceName, RT_RCDATA);
  if (hRes == NULL)
    {
      logError(L"FindResource failed");
      return false;
    }

  //Load the resource.
  HGLOBAL hResData = LoadResource(GetModuleHandle(NULL), hRes);
  if (hResData == NULL) {
    logError(L"LoadResource failed");
    return false;
  }

  //Get a pointer to the resource data.
  const void* pData = LockResource(hResData);
  if (pData == NULL) {
    logError(L"LockResource failed");
    return false;
  }

  //Get the size of the resource.
  DWORD dataSize = SizeofResource(GetModuleHandle(NULL), hRes);
  if (dataSize == 0) {
    logError(L"SizeofResource failed");
    return false;
  }

  //Write the resource data to a file.
  std::ofstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
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

void setClippingRegion(HWND hWnd){
  RECT winrect, rect;
  POINT client_origin;

  // Get window and client rects
  GetWindowRect( hWnd, &winrect );
  GetClientRect( hWnd, &rect );

  // Get the client origin in window coordinates
  client_origin.x = 0; client_origin.y = 0;
  ClientToScreen(hWnd, &client_origin);
  client_origin.x -= winrect.left;
  client_origin.y -= winrect.top;

  POINT points[] = {{170, 0}, {167, 3}, {167, 4}, {166, 5}, {166, 6}, {165, 7}, {165, 9}, {164, 10}, {164, 11}, {163, 12}, {163, 14}, {162, 15}, {162, 16}, {161, 17}, {161, 19}, {160, 20}, {160, 23}, {159, 24}, {159, 28}, {157, 30}, {157, 36}, {156, 37}, {155, 37}, {153, 35}, {134, 35}, {133, 36}, {127, 36}, {126, 37}, {124, 37}, {123, 38}, {121, 38}, {120, 39}, {118, 39}, {117, 40}, {116, 40}, {115, 41}, {113, 41}, {112, 42}, {110, 42}, {108, 44}, {107, 44}, {106, 45}, {105, 45}, {103, 47}, {102, 47}, {101, 48}, {99, 48}, {98, 49}, {97, 49}, {95, 51}, {94, 51}, {93, 50}, {93, 46}, {94, 45}, {94, 43}, {95, 42}, {95, 40}, {103, 32}, {111, 32}, {113, 30}, {113, 25}, {109, 21}, {105, 21}, {104, 20}, {100, 20}, {99, 21}, {97, 21}, {96, 22}, {95, 22}, {86, 31}, {86, 32}, {83, 35}, {83, 36}, {82, 37}, {82, 39}, {81, 40}, {81, 43}, {80, 44}, {80, 51}, {81, 52}, {81, 54}, {82, 55}, {82, 56}, {83, 57}, {83, 58}, {84, 59}, {83, 60}, {81, 60}, {80, 59}, {79, 59}, {77, 57}, {76, 57}, {75, 56}, {74, 56}, {72, 54}, {71, 54}, {70, 53}, {69, 53}, {68, 52}, {67, 52}, {66, 51}, {65, 51}, {64, 50}, {62, 50}, {61, 49}, {57, 49}, {55, 51}, {55, 59}, {56, 60}, {56, 62}, {57, 63}, {57, 66}, {58, 67}, {58, 68}, {54, 72}, {54, 73}, {52, 75}, {52, 76}, {51, 77}, {51, 78}, {50, 79}, {50, 80}, {49, 81}, {49, 83}, {48, 84}, {48, 86}, {47, 87}, {47, 90}, {46, 91}, {46, 94}, {45, 95}, {45, 98}, {44, 99}, {44, 105}, {43, 106}, {43, 118}, {42, 119}, {42, 141}, {41, 142}, {41, 144}, {40, 145}, {40, 146}, {39, 147}, {39, 149}, {38, 150}, {38, 152}, {37, 153}, {37, 154}, {36, 155}, {36, 158}, {35, 159}, {35, 160}, {34, 161}, {34, 163}, {33, 164}, {33, 165}, {32, 166}, {32, 167}, {31, 168}, {31, 169}, {29, 171}, {29, 172}, {28, 173}, {28, 174}, {26, 176}, {26, 177}, {25, 178}, {25, 179}, {23, 181}, {23, 182}, {22, 183}, {22, 184}, {21, 185}, {21, 186}, {18, 189}, {18, 193}, {17, 194}, {17, 195}, {15, 197}, {15, 199}, {14, 200}, {14, 204}, {13, 205}, {13, 211}, {14, 212}, {14, 216}, {13, 217}, {13, 221}, {14, 222}, {14, 223}, {15, 224}, {15, 227}, {16, 228}, {16, 231}, {20, 235}, {20, 236}, {28, 244}, {29, 244}, {33, 248}, {33, 255}, {34, 256}, {34, 260}, {35, 261}, {35, 263}, {36, 264}, {36, 265}, {37, 266}, {37, 267}, {41, 271}, {41, 272}, {45, 276}, {46, 276}, {50, 280}, {51, 280}, {52, 281}, {53, 281}, {54, 282}, {55, 282}, {56, 283}, {57, 283}, {58, 284}, {60, 284}, {61, 285}, {61, 291}, {62, 292}, {62, 293}, {64, 295}, {63, 296}, {61, 296}, {60, 297}, {59, 297}, {55, 293}, {55, 292}, {52, 289}, {51, 289}, {50, 288}, {49, 288}, {45, 284}, {45, 283}, {44, 282}, {44, 281}, {43, 280}, {43, 279}, {42, 278}, {42, 277}, {41, 276}, {41, 275}, {38, 272}, {33, 272}, {28, 277}, {28, 290}, {29, 291}, {29, 292}, {30, 293}, {30, 294}, {29, 295}, {23, 295}, {21, 293}, {10, 293}, {9, 294}, {3, 294}, {0, 297}, {0, 318}, {2, 318}, {3, 317}, {4, 318}, {4, 329}, {9, 334}, {9, 340}, {14, 345}, {14, 348}, {18, 352}, {20, 352}, {21, 353}, {22, 353}, {23, 354}, {25, 354}, {26, 355}, {36, 355}, {37, 354}, {38, 354}, {40, 352}, {41, 352}, {43, 350}, {44, 350}, {45, 349}, {46, 349}, {49, 346}, {50, 347}, {52, 347}, {53, 348}, {57, 348}, {58, 349}, {59, 349}, {61, 351}, {69, 351}, {70, 352}, {71, 352}, {73, 354}, {74, 354}, {79, 359}, {80, 359}, {83, 362}, {84, 362}, {87, 365}, {88, 365}, {91, 368}, {92, 368}, {97, 373}, {98, 373}, {100, 375}, {101, 375}, {102, 376}, {103, 376}, {104, 377}, {105, 377}, {106, 378}, {107, 378}, {108, 379}, {110, 379}, {111, 380}, {112, 380}, {113, 381}, {114, 381}, {115, 382}, {125, 382}, {128, 379}, {128, 378}, {129, 377}, {129, 375}, {130, 374}, {130, 372}, {131, 371}, {131, 370}, {132, 369}, {132, 368}, {133, 367}, {133, 366}, {134, 365}, {134, 364}, {135, 363}, {135, 362}, {136, 361}, {136, 359}, {137, 358}, {137, 357}, {139, 355}, {140, 355}, {141, 356}, {142, 356}, {143, 357}, {149, 357}, {150, 356}, {151, 356}, {153, 358}, {154, 358}, {155, 359}, {155, 360}, {154, 361}, {154, 363}, {153, 364}, {153, 366}, {152, 367}, {152, 369}, {151, 370}, {151, 376}, {150, 377}, {150, 380}, {149, 381}, {149, 383}, {274, 383}, {274, 381}, {273, 380}, {273, 379}, {268, 374}, {267, 374}, {266, 373}, {266, 372}, {265, 371}, {265, 370}, {264, 369}, {264, 368}, {263, 367}, {263, 366}, {262, 365}, {262, 364}, {260, 362}, {260, 361}, {259, 360}, {259, 359}, {258, 358}, {258, 357}, {256, 355}, {256, 354}, {255, 353}, {255, 352}, {254, 351}, {254, 350}, {253, 349}, {253, 348}, {251, 346}, {251, 342}, {250, 341}, {252, 339}, {252, 332}, {251, 331}, {251, 328}, {250, 327}, {251, 326}, {251, 322}, {250, 321}, {250, 320}, {253, 317}, {253, 313}, {254, 312}, {254, 308}, {255, 307}, {255, 306}, {256, 305}, {256, 302}, {257, 301}, {257, 300}, {258, 299}, {260, 299}, {264, 295}, {264, 291}, {265, 290}, {265, 287}, {266, 286}, {266, 283}, {268, 281}, {269, 282}, {269, 286}, {270, 287}, {270, 289}, {271, 290}, {271, 291}, {272, 292}, {272, 293}, {273, 294}, {273, 295}, {275, 297}, {275, 298}, {280, 303}, {280, 304}, {282, 306}, {283, 306}, {286, 309}, {287, 309}, {288, 310}, {289, 310}, {291, 312}, {292, 312}, {293, 313}, {294, 313}, {295, 314}, {297, 314}, {298, 315}, {299, 315}, {300, 316}, {302, 316}, {306, 320}, {307, 320}, {310, 323}, {313, 323}, {315, 325}, {325, 325}, {327, 323}, {327, 306}, {325, 304}, {324, 304}, {321, 301}, {318, 301}, {313, 296}, {310, 296}, {308, 294}, {306, 294}, {305, 293}, {306, 292}, {308, 292}, {310, 290}, {310, 283}, {311, 282}, {313, 284}, {317, 284}, {319, 282}, {319, 281}, {321, 279}, {321, 278}, {322, 277}, {322, 276}, {323, 275}, {323, 273}, {324, 272}, {324, 268}, {325, 267}, {325, 265}, {326, 264}, {326, 263}, {327, 262}, {327, 261}, {328, 260}, {328, 259}, {331, 256}, {331, 255}, {338, 248}, {338, 247}, {342, 243}, {342, 240}, {343, 239}, {343, 236}, {344, 235}, {344, 234}, {345, 233}, {345, 229}, {344, 228}, {344, 224}, {345, 223}, {345, 217}, {344, 216}, {344, 212}, {343, 211}, {343, 209}, {341, 207}, {341, 206}, {340, 205}, {340, 201}, {337, 198}, {337, 197}, {336, 196}, {336, 195}, {335, 194}, {335, 193}, {333, 191}, {333, 190}, {332, 189}, {332, 188}, {330, 186}, {330, 185}, {329, 184}, {329, 183}, {327, 181}, {327, 179}, {325, 177}, {325, 175}, {324, 174}, {324, 157}, {321, 154}, {321, 152}, {319, 150}, {319, 147}, {317, 145}, {317, 141}, {314, 138}, {314, 132}, {312, 130}, {308, 130}, {307, 129}, {307, 128}, {305, 126}, {304, 126}, {303, 125}, {302, 125}, {300, 123}, {300, 122}, {298, 120}, {298, 119}, {297, 118}, {297, 117}, {296, 116}, {296, 115}, {295, 114}, {295, 113}, {294, 112}, {294, 111}, {293, 110}, {293, 109}, {291, 107}, {291, 106}, {290, 105}, {290, 104}, {288, 102}, {288, 101}, {289, 100}, {290, 100}, {292, 102}, {293, 102}, {295, 104}, {296, 104}, {304, 112}, {304, 113}, {305, 114}, {305, 117}, {307, 119}, {312, 119}, {314, 117}, {314, 103}, {305, 94}, {304, 94}, {299, 89}, {298, 89}, {295, 86}, {294, 86}, {291, 83}, {290, 83}, {288, 81}, {287, 81}, {286, 80}, {285, 80}, {284, 79}, {283, 79}, {282, 78}, {281, 78}, {279, 76}, {278, 76}, {277, 75}, {276, 75}, {275, 74}, {274, 74}, {273, 73}, {272, 73}, {270, 71}, {269, 71}, {266, 68}, {266, 67}, {265, 66}, {265, 65}, {263, 63}, {263, 62}, {262, 61}, {262, 60}, {260, 58}, {260, 57}, {258, 55}, {258, 54}, {256, 52}, {256, 51}, {253, 48}, {253, 47}, {250, 44}, {250, 43}, {246, 39}, {246, 38}, {232, 24}, {231, 24}, {228, 21}, {227, 21}, {225, 19}, {224, 19}, {222, 17}, {221, 17}, {220, 16}, {219, 16}, {218, 15}, {216, 15}, {215, 14}, {201, 14}, {200, 15}, {198, 15}, {197, 16}, {196, 16}, {194, 18}, {193, 18}, {192, 19}, {191, 19}, {189, 21}, {187, 19}, {187, 18}, {185, 16}, {185, 15}, {184, 14}, {184, 13}, {183, 12}, {183, 11}, {182, 10}, {182, 9}, {180, 7}, {180, 6}, {179, 5}, {179, 4}, {177, 2}, {177, 1}, {176, 0}};
  int numPoints = 683;

  // Iterate through the array and scale each point
  for (int i = 0; i < numPoints; ++i) {
    points[i].x = static_cast<LONG>(points[i].x * scale); // Scale x and convert back to LONG
    points[i].y = static_cast<LONG>(points[i].y * scale); // Scale y and convert back to LONG
  }

  HRGN hRgn = CreatePolygonRgn(points, numPoints, ALTERNATE);

  SetWindowRgn(hWnd,hRgn,1);
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  HDC hdc;
  PAINTSTRUCT ps;
  static Bitmap* bitmap1 = NULL;
  static Bitmap* bitmap2 = NULL;

  switch (message) {
  case WM_CREATE: {
    // Initialize GDI+
    Status startupStatus = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    if (startupStatus != Ok) {
      MessageBox(hwnd, "Failed to initialize GDI+.", "Error", MB_OK);
      return -1;
    }
    std::wstring image1path = std::wstring(tempDir) + L"__girl1.png";
    std::wstring image2path = std::wstring(tempDir) + L"__girl2.png";
    bitmap1 = Bitmap::FromFile(image1path.c_str());
    if (bitmap1->GetLastStatus() != Ok) {
      logError(L"Failed to load image1.png!");
      MessageBox(hwnd, "Failed to load image1.png.", "Error", MB_OK);
      return -1;
    }

    bitmap2 = Bitmap::FromFile(image2path.c_str());
    if (bitmap2->GetLastStatus() != Ok) {
      logError(L"Failed to load image2.png!");
      MessageBox(hwnd, "Failed to load image2.png.", "Error", MB_OK);
      return -1;
    }
    //Set window as layered.
    SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                     GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    //Make the window fully transparent.
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
    // SetLayeredWindowAttributes(hwnd, RGB(1, 1, 1), 0, LWA_COLORKEY);
    SetTimer(hwnd, 1, 100, nullptr);
    break;
  }
  case WM_PAINT: {
    hdc = BeginPaint(hwnd, &ps);
    Graphics graphics(hdc);

    // 1. Redraw the background
    HBRUSH hbrBackground = CreateSolidBrush(RGB(0,0,0));
    FillRect(hdc, &ps.rcPaint, hbrBackground);
    DeleteObject(hbrBackground);

    //Draw the image.
    if (currentImage == 0)
      {
        graphics.DrawImage(bitmap1, 0, 0,
                           static_cast<LONG>(bitmap1->GetWidth() * scale),
                           static_cast<LONG>(bitmap1->GetHeight() * scale));
      }
    else if (currentImage == 1)
      {
        graphics.DrawImage(bitmap2, 0, 0,
                           static_cast<LONG>(bitmap2->GetWidth() * scale),
                           static_cast<LONG>(bitmap2->GetHeight() * scale));
      }

    EndPaint(hwnd, &ps);
    break;
  }
  case WM_TIMER: {
    if (wParam == 1) {
      currentImage ^= 1;
      InvalidateRect(hwnd, nullptr, FALSE);
      UpdateWindow(hwnd);
    }
    break;
  }
  case WM_LBUTTONDOWN:
    log(L"Left clicked!");
    break;
  case WM_RBUTTONUP:
    log(L"Right clicked!");
    PostQuitMessage(0);
    break;
  case WM_DESTROY:
    KillTimer(hwnd, 1);
    delete bitmap1;
    delete bitmap2;
    GdiplusShutdown(gdiplusToken);
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hwnd, message, wParam, lParam);
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
  log(L"dpsgirl start");

  // Define a temporary directory (you might want to use GetTempPath)
  if (GetTempPathW(MAX_PATH, tempDir) == 0) {
    logError(L"GetTempPathW failed");
    return -1;
  }

  // // Extract the DLLs.
  std::wstring image1path = std::wstring(tempDir) + L"__girl1.png";
  std::wstring image2path = std::wstring(tempDir) + L"__girl2.png";

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
  int scaledImageWidth = static_cast<LONG>(350 * scale);
  int scaledImageHeight = static_cast<LONG>(384 * scale);

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
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  DeleteFileW(image1path.c_str());
  DeleteFileW(image2path.c_str());

  log(L"dpsgirl end");

  return (int)msg.wParam;
}
