#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h> // If needed for direct Xlib surfaces
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <cstdlib> // For srand, rand
#include "../cppparser/parser.cpp"

// Global variables (similar to your Windows version)
int animation_delay = 100;
int currentImage = 0;
int sleeping = 0;
float scale = 1.0f;
int bubbleX = -30;
int bubbleY = -60;

// Function to get the current timestamp for logging
std::wstring GetTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::tm now_tm;
#ifdef _WIN32
  localtime_s(&now_tm, &now_c);
#else
  localtime_r(&now_c, &now_tm);
#endif
  std::wstringstream wss;
  wss.imbue(std::locale("")); // Use the user's default locale for wide characters
  wss << std::put_time(&now_tm, L"%Y-%m-%d %H:%M:%S");
  return wss.str();
}

int spit(std::string fileName, std::wstring msg)
{
  std::wofstream file(fileName, std::ios::app);
  if (file.is_open()) {
    file << msg;
    std::wcerr << msg;
    file.close();
  }
  return 0;
}

int spit(std::string fileName, std::string msg)
{
  std::ofstream file(fileName, std::ios::app);
  if (file.is_open()) {
    file << msg;
    std::cerr << msg;
    file.close();
  }
  return 0;
}

int log(std::wstring msg) {
  return spit("dpsgirl.log", GetTimestamp() + L" - " + msg + L"\n");
}

int log(std::string msg) {
  std::wstring wstr = GetTimestamp();
  char buffer[100];
  std::wcstombs(buffer, wstr.c_str(), sizeof(buffer));
  std::string timestamp(buffer);
  return spit("dpsgirl.log", timestamp + " - " + msg + "\n");
}

void logError(std::wstring msg) {
  log(msg);
  return;
}

struct Point {
  int x;
  int y;
};

void setClippingRegion(GtkWidget *window){
  cairo_rectangle_int_t rect = { 0, 0, 0, 0 };
  cairo_region_t *shape = cairo_region_create_rectangle(&rect);

  GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(window));
  if (gdk_window) { // This might be NULL if this gets called during initialisation
    printf("Found a window to realize\n");
    gdk_window_input_shape_combine_region(gdk_window, shape, 0,0);
    gtk_widget_input_shape_combine_region(window, shape);
  }
  cairo_region_destroy(shape);
}

// Function to get DPS as a wide string (same as before)
std::wstring getDpsAsString() {
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

// Function to load an image using Cairo
cairo_surface_t* load_image(const std::wstring& filename) {
  // Convert wstring to UTF-8 string for file access
  std::string utf8_filename;
  for (wchar_t wc : filename) {
    // Basic conversion, might need more robust handling
    utf8_filename += static_cast<char>(wc);
  }
  cairo_surface_t* surface = cairo_image_surface_create_from_png(utf8_filename.c_str());
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    logError(L"Failed to load image: " + filename);
    if (surface) cairo_surface_destroy(surface);
    return nullptr;
  }
  return surface;
}

// GTK draw callback function
// static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
static gboolean on_draw(GtkWidget *, cairo_t *cr, gpointer) {
  static cairo_surface_t *bitmap1 = nullptr;
  static cairo_surface_t *bitmap2 = nullptr;
  static cairo_surface_t *bitmapB = nullptr;
  static cairo_surface_t *bitmapZ = nullptr;
  static cairo_surface_t *bitmapU = nullptr;
  static bool loaded = false;

  if (!loaded) {
    wchar_t tempDir[] = L"../"; // Adjust path as needed for Linux
    std::wstring image1path = std::wstring(tempDir) + L"girl1.png";
    std::wstring image2path = std::wstring(tempDir) + L"girl2.png";
    std::wstring imageBpath = std::wstring(tempDir) + L"girlb.png";
    std::wstring imageZpath = std::wstring(tempDir) + L"girlz.png";
    std::wstring imageUpath = std::wstring(tempDir) + L"bubble200.png";

    bitmap1 = load_image(image1path);
    bitmap2 = load_image(image2path);
    bitmapB = load_image(imageBpath);
    bitmapZ = load_image(imageZpath);
    bitmapU = load_image(imageUpath);
    loaded = true;
  }

  // Clear the background (if needed, or make window transparent)
  // cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  // cairo_paint(cr);

  // Draw the image
  cairo_surface_t *current_bitmap = nullptr;
  if (currentImage == 0 && bitmap1) current_bitmap = bitmap1;
  else if (currentImage == 1 && bitmap2) current_bitmap = bitmap2;
  else if (currentImage == 2 && bitmapB) current_bitmap = bitmapB;
  else if (currentImage == 3 && bitmapZ) current_bitmap = bitmapZ;

  if (current_bitmap) {
    cairo_set_source_surface(cr, current_bitmap, -bubbleX, -bubbleY);
    cairo_paint(cr);
  }

  if (bitmapU) {
    cairo_set_source_surface(cr, bitmapU, 0, 0);
    cairo_paint(cr);
  }

  // Draw text
  cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 30 * scale);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black
  std::wstring textToDraw = getDpsAsString();
  // Convert wstring to UTF-8 for Cairo
  std::string utf8_text;
  for (wchar_t wc : textToDraw) utf8_text += static_cast<char>(wc);
  cairo_move_to(cr, 50 * scale, 90 * scale);
  cairo_show_text(cr, utf8_text.c_str());

  cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 14 * scale);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  std::wstring textToDraw2 = sleeping ? L"zzz" : L"dps";
  std::string utf8_text2;
  for (wchar_t wc : textToDraw2) utf8_text2 += static_cast<char>(wc);
  cairo_move_to(cr, 130 * scale, 60 * scale);
  cairo_show_text(cr, utf8_text2.c_str());

  return TRUE;
}

// GTK timer callback
static gboolean on_timer(gpointer user_data) {
  if (currentImage > 1)
    currentImage = 0;
  currentImage ^= 1;

  int randomInRange = (rand() % 100) + 1;
  if (randomInRange < 10)
    currentImage = 2;

  if (sleeping == 1)
    currentImage = 3;

  GtkWidget *window = GTK_WIDGET(user_data);
  gtk_widget_queue_draw(window); // Request a redraw
  return TRUE; // Continue the timer
}

// GTK button press event handler
// static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer) {
  if (event->button == 1) {
    log(L"Left clicked!");
    sleeping ^= 1;
    gtk_widget_queue_draw(widget);
  } else if (event->button == 3) {
    log(L"Right clicked!");
    gtk_main_quit();
  }
  return TRUE;
}

// GTK drag data received handler
// static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
//     if (data->length >= 0 && data->format == 8) { // Assuming UTF-8 encoded paths
//         gchar **uris = g_uri_list_extract_uris((const gchar *)data->data);
//         if (uris) {
//             for (gint i = 0; uris[i] != NULL; i++) {
//                 gchar *filename = g_uri_parse_path(uris[i]);
//                 if (filename) {
//                     // Convert gchar* (UTF-8) to wstring (if needed)
//                     std::string utf8_filename = filename;
//                     std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
//                     std::wstring wfilename = converter.from_bytes(utf8_filename);
//                     log(L"Dropped file: " + wfilename);
//                     g_free(filename);
//                 } else {
//                     logError(L"Error parsing dropped URI.");
//                 }
//                 g_free(uris[i]);
//             }
//             g_strfreev(uris);
//         }
//         gtk_drag_finish(context, TRUE, FALSE, time);
//     } else {
//         gtk_drag_finish(context, FALSE, FALSE, time);
//     }
// }

// // Function to set up drag and drop
// static void setup_drag_and_drop(GtkWidget *widget) {
//     GtkTargetList *targets = gtk_target_list_new(NULL, 0);
//     gtk_target_list_add_uri_targets(targets, 0);
//     gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, gtk_target_list_get_targets(targets, NULL), gtk_target_list_get_n_targets(targets), GDK_ACTION_COPY);
//     g_signal_connect(widget, "drag-data-received", G_CALLBACK(on_drag_data_received), NULL);
//     gtk_target_list_unref(targets);
// }

int main(int argc, char *argv[]) {
  srand(static_cast<unsigned int>(time(0)));

  gtk_init(&argc, &argv);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Transparent Window");

  // Get screen dimensions (using GdkScreen)
  GdkScreen *screen = gdk_screen_get_default();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  int screen_width = gdk_screen_get_width(screen);
  int screen_height = gdk_screen_get_height(screen);
#pragma GCC diagnostic pop

  // NOTE: Update these values based on your image dimensions and scale
  int scaled_image_width = static_cast<int>(350 * scale) - bubbleX;
  int scaled_image_height = static_cast<int>(384 * scale) - bubbleY;

  gtk_window_set_default_size(GTK_WINDOW(window), scaled_image_width, scaled_image_height);

  // Calculate position for the bottom-right corner
  int x = screen_width - scaled_image_width;
  int y = screen_height - scaled_image_height;
  gtk_window_move(GTK_WINDOW(window), x, y);

  gtk_window_set_keep_above(GTK_WINDOW(window), 1);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), 1);
  gtk_window_set_skip_pager_hint(GTK_WINDOW(window), 1);
  gtk_window_set_focus_on_map(GTK_WINDOW(window), 0);
  gtk_window_set_accept_focus(GTK_WINDOW(window), 1);
  gtk_window_set_decorated(GTK_WINDOW(window), 0);
  gtk_window_set_resizable(GTK_WINDOW(window), 0);

  // Make the window background transparent (if compositor is running)
  GdkScreen *screen_visual = gtk_widget_get_screen(window);
  GdkVisual *visual = gdk_screen_get_rgba_visual(screen_visual);
  if (visual != NULL && gdk_screen_is_composited(screen_visual)) {
    gtk_widget_set_visual(window, visual);
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  const GdkRGBA color = { .alpha = 0.0 };
  gtk_widget_override_background_color(window, (GtkStateFlags)0, &color);
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

  GtkWidget *drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area, scaled_image_width, scaled_image_height);
  gtk_container_add(GTK_CONTAINER(window), drawing_area);

  g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(on_draw), NULL);
  g_signal_connect(G_OBJECT(drawing_area), "button-press-event", G_CALLBACK(on_button_press), NULL);

  // Enable button press events
  gtk_widget_set_events(drawing_area, gtk_widget_get_events(drawing_area) | GDK_BUTTON_PRESS_MASK);

  // setup_drag_and_drop(drawing_area);

  // Set a timer for animation
  g_timeout_add(animation_delay, (GSourceFunc)on_timer, window);

  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show_all(window);
  // setClippingRegion(window);

  gtk_main();

  // Cleanup (though GTK usually handles this on exit)
  return 0;
}
