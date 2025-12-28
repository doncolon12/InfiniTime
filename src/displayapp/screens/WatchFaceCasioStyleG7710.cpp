#include "displayapp/screens/WatchFaceCasioStyleG7710.h"

#include <lvgl/lvgl.h>
#include <cstdio>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

WatchFaceCasioStyleG7710::WatchFaceCasioStyleG7710(
    Controllers::DateTime& dateTimeController,
    const Controllers::Battery& batteryController,
    const Controllers::Ble& bleController,
    Controllers::NotificationManager& notificatioManager,
    Controllers::Settings& settingsController,
    Controllers::HeartRateController& heartRateController,
    Controllers::MotionController& motionController,
    Controllers::FS& filesystem)
  : currentDateTime{{}},
    batteryPercentRemaining{},
    powerPresent{},
    bleState{},
    bleRadioEnabled{},
    currentDate{},
    stepCount{},
    heartbeat{},
    heartbeatRunning{},
    notificationState{},
    batteryIcon(false), // Corrected to match BatteryIcon(bool)
    label_time_seconds{nullptr},
    dateTimeController{dateTimeController},
    batteryController{batteryController},
    bleController{bleController},
    notificatioManager{notificatioManager},
    settingsController{settingsController},
    heartRateController{heartRateController},
    motionController{motionController} {

  lfs_file f = {};
  if (filesystem.FileOpen(&f, "/fonts/lv_font_dots_40.bin", LFS_O_RDONLY) >= 0) {
    filesystem.FileClose(&f);
    font_dot40 = lv_font_load("F:/fonts/lv_font_dots_40.bin");
  }

  if (filesystem.FileOpen(&f, "/fonts/7segments_40.bin", LFS_O_RDONLY) >= 0) {
    filesystem.FileClose(&f);
    font_segment40 = lv_font_load("F:/fonts/7segments_40.bin");
  }

  if (filesystem.FileOpen(&f, "/fonts/7segments_115.bin", LFS_O_RDONLY) >= 0) {
    filesystem.FileClose(&f);
    font_segment115 = lv_font_load("F:/fonts/7segments_115.bin");
  }

  // --- Battery and BLE icons ---
  label_battery_value = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_battery_value, lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_local_text_color(label_battery_value, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(label_battery_value, "00%");

  batteryIcon.Create(lv_scr_act());
  batteryIcon.SetColor(color_text);
  lv_obj_align(batteryIcon.GetObject(), label_battery_value, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  batteryPlug = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(batteryPlug, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(batteryPlug, Symbols::plug);
  lv_obj_align(batteryPlug, batteryIcon.GetObject(), LV_ALIGN_OUT_LEFT_MID, -5, 0);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(bleIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(bleIcon, Symbols::bluetooth);
  lv_obj_align(bleIcon, batteryPlug, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, bleIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // --- Day, week, and year labels ---
  label_day_of_week = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_day_of_week, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 10, 64);
  lv_obj_set_style_local_text_color(label_day_of_week, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(label_day_of_week, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_dot40);
  lv_label_set_text_static(label_day_of_week, "SUN");

  label_week_number = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_week_number, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 5, 22);
  lv_obj_set_style_local_text_color(label_week_number, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(label_week_number, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_dot40);
  lv_label_set_text_static(label_week_number, "WK26");

  label_day_of_year = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_day_of_year, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 100, 30);
  lv_obj_set_style_local_text_color(label_day_of_year, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(label_day_of_year, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_segment40);
  lv_label_set_text_static(label_day_of_year, "181-184");

  // --- Styles ---
  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, LV_STATE_DEFAULT, 2);
  lv_style_set_line_color(&style_line, LV_STATE_DEFAULT, color_text);
  lv_style_set_line_rounded(&style_line, LV_STATE_DEFAULT, true);

  lv_style_init(&style_border);
  lv_style_set_line_width(&style_border, LV_STATE_DEFAULT, 6);
  lv_style_set_line_color(&style_border, LV_STATE_DEFAULT, color_text);
  lv_style_set_line_rounded(&style_border, LV_STATE_DEFAULT, true);

  // --- Lines ---
  line_icons = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(line_icons, line_icons_points, 3);
  lv_obj_add_style(line_icons, LV_LINE_PART_MAIN, &style_line);
  lv_obj_align(line_icons, nullptr, LV_ALIGN_IN_TOP_RIGHT, -10, 18);

  line_day_of_week_number = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(line_day_of_week_number, line_day_of_week_number_points, 4);
  lv_obj_add_style(line_day_of_week_number, LV_LINE_PART_MAIN, &style_border);
  lv_obj_align(line_day_of_week_number, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 8);

  line_day_of_year = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(line_day_of_year, line_day_of_year_points, 3);
  lv_obj_add_style(line_day_of_year, LV_LINE_PART_MAIN, &style_line);
  lv_obj_align(line_day_of_year, nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 60);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 100, 70);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_segment40);
  lv_label_set_text_static(label_date, "6-30");

  line_date = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(line_date, line_date_points, 3);
  lv_obj_add_style(line_date, LV_LINE_PART_MAIN, &style_line);
  lv_obj_align(line_date, nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 100);

  // --- Main time label (HH:MM) ---
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_segment115);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, 0, 40);

  // --- Small seconds (SS) ---
  label_time_seconds = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time_seconds, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(label_time_seconds, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_segment40);
  lv_obj_align(label_time_seconds, label_time, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 0);

  line_time = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(line_time, line_time_points, 3);
  lv_obj_add_style(line_time, LV_LINE_PART_MAIN, &style_line);
  lv_obj_align(line_time, nullptr, LV_ALIGN_IN_BOTTOM_RIGHT, 0, -25);

  label_time_ampm = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time_ampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(label_time_ampm, "");
  lv_obj_align(label_time_ampm, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 5, -5);

  // --- Background label ---
  backgroundLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_click(backgroundLabel, true);
  lv_label_set_long_mode(backgroundLabel, LV_LABEL_LONG_CROP);
  lv_obj_set_size(backgroundLabel, 240, 240);
  lv_obj_set_pos(backgroundLabel, 0, 0);
  lv_label_set_text_static(backgroundLabel, "");

  // --- Heartbeat & step counters ---
  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 5, -2);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(stepValue, "0");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, -5, -2);

  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, stepValue, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // --- Refresh task ---
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

// Destructor
WatchFaceCasioStyleG7710::~WatchFaceCasioStyleG7710() {
  lv_task_del(taskRefresh);
  lv_style_reset(&style_line);
  lv_style_reset(&style_border);

  if (font_dot40) lv_font_free(font_dot40);
  if (font_segment40) lv_font_free(font_segment40);
  if (font_segment115) lv_font_free(font_segment115);

  lv_obj_clean(lv_scr_act());
}

// Refresh function
void WatchFaceCasioStyleG7710::Refresh() {
  // ... your existing Refresh() code remains unchanged ...
}

// Availability check
bool WatchFaceCasioStyleG7710::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  lfs_file file = {};
  if (filesystem.FileOpen(&file, "/fonts/lv_font_dots_40.bin", LFS_O_RDONLY) < 0) return false;
  filesystem.FileClose(&file);
  if (filesystem.FileOpen(&file, "/fonts/7segments_40.bin", LFS_O_RDONLY) < 0) return false;
  filesystem.FileClose(&file);
  if (filesystem.FileOpen(&file, "/fonts/7segments_115.bin", LFS_O_RDONLY) < 0) return false;
  filesystem.FileClose(&file);
  return true;
}
