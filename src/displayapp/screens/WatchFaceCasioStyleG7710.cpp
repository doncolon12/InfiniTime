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

WatchFaceCasioStyleG7710::WatchFaceCasioStyleG7710(Controllers::DateTime& dateTimeController,
                                                   const Controllers::Battery& batteryController,
                                                   const Controllers::Ble& bleController,
                                                   Controllers::NotificationManager& notificatioManager,
                                                   Controllers::Settings& settingsController,
                                                   Controllers::HeartRateController& heartRateController,
                                                   Controllers::MotionController& motionController,
                                                   Controllers::FS& filesystem)
  : currentDateTime {{}},
    batteryIcon(false),
    label_time_seconds{nullptr},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificatioManager {notificatioManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {

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

  // Battery
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

  // BLE
  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(bleIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(bleIcon, Symbols::bluetooth);
  lv_obj_align(bleIcon, batteryPlug, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // Notifications
  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, bleIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // Day of week / week number / day of year
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

  // Styles
  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, LV_STATE_DEFAULT, 2);
  lv_style_set_line_color(&style_line, LV_STATE_DEFAULT, color_text);
  lv_style_set_line_rounded(&style_line, LV_STATE_DEFAULT, true);

  lv_style_init(&style_border);
  lv_style_set_line_width(&style_border, LV_STATE_DEFAULT, 6);
  lv_style_set_line_color(&style_border, LV_STATE_DEFAULT, color_text);
  lv_style_set_line_rounded(&style_border, LV_STATE_DEFAULT, true);

  // Lines
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

  // Date
  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 100, 70);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_segment40);
  lv_label_set_text_static(label_date, "6-30");

  line_date = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(line_date, line_date_points, 3);
  lv_obj_add_style(line_date, LV_LINE_PART_MAIN, &style_line);
  lv_obj_align(line_date, nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 100);

  // Main time (HH:MM)
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_segment115);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, 0, 40);
  
  // Seconds (SS) to the right and slightly lower
  label_time_seconds = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time_seconds, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(label_time_seconds, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font_segment40);
  lv_obj_align(label_time_seconds, label_time, LV_ALIGN_OUT_RIGHT_MID, 5, 20);  // right + lower

  line_time = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(line_time, line_time_points, 3);
  lv_obj_add_style(line_time, LV_LINE_PART_MAIN, &style_line);
  lv_obj_align(line_time, nullptr, LV_ALIGN_IN_BOTTOM_RIGHT, 0, -25);

  label_time_ampm = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time_ampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(label_time_ampm, "");
  lv_obj_align(label_time_ampm, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 5, -5);

  // Background
  backgroundLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_click(backgroundLabel, true);
  lv_label_set_long_mode(backgroundLabel, LV_LABEL_LONG_CROP);
  lv_obj_set_size(backgroundLabel, 240, 240);
  lv_obj_set_pos(backgroundLabel, 0, 0);
  lv_label_set_text_static(backgroundLabel, "");

  // Heartbeat
  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 5, -2);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // Steps
  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(stepValue, "0");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, -5, -2);

  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, stepValue, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // Refresh task
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceCasioStyleG7710::~WatchFaceCasioStyleG7710() {
  lv_task_del(taskRefresh);

  lv_style_reset(&style_line);
  lv_style_reset(&style_border);

  if (font_dot40 != nullptr) lv_font_free(font_dot40);
  if (font_segment40 != nullptr) lv_font_free(font_segment40);
  if (font_segment115 != nullptr) lv_font_free(font_segment115);

  lv_obj_clean(lv_scr_act());
}

void WatchFaceCasioStyleG7710::Refresh() {
  // Battery
  powerPresent = batteryController.IsPowerPresent();
  if (powerPresent.IsUpdated()) {
    lv_label_set_text_static(batteryPlug, BatteryIcon::GetPlugIcon(powerPresent.Get()));
  }

  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated()) {
    auto batteryPercent = batteryPercentRemaining.Get();
    batteryIcon.SetBatteryPercentage(batteryPercent);
    lv_label_set_text_fmt(label_battery_value, "%d%%", batteryPercent);
  }

  // BLE
  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    lv_label_set_text_static(bleIcon, BleIcon::GetIcon(bleState.Get()));
  }

  lv_obj_realign(label_battery_value);
  lv_obj_realign(batteryIcon.GetObject());
  lv_obj_realign(batteryPlug);
  lv_obj_realign(bleIcon);
  lv_obj_realign(notificationIcon);

  // Notifications
  notificationState = notificatioManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  // Date and Time
  currentDateTime = dateTimeController.CurrentDateTime();
  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();
    uint8_t second = dateTimeController.Seconds();

    // Compute tm struct for month/day/week calculations
    time_t ttTime = std::chrono::system_clock::to_time_t(
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(currentDateTime.Get()));
    tm* tmTime = std::localtime(&ttTime);

    char monthStr[4];
    strftime(monthStr, sizeof(monthStr), "%b", tmTime);

    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      char ampmChar[2] = "A";
      if (hour == 0) hour = 12;
      else if (hour == 12) ampmChar[0] = 'P';
      else if (hour > 12) {
        hour -= 12;
        ampmChar[0] = 'P';
      }
      lv_label_set_text(label_time_ampm, ampmChar);
    }

    // Blinking colon for HH:MM
    char timeBuf[6]; // "HH:MM" or "HH MM"
    char colon = (second % 2 == 0) ? ':' : ' ';
    snprintf(timeBuf, sizeof(timeBuf), "%02d%c%02d", hour, colon, minute);
    lv_label_set_text(label_time, timeBuf);

    // Seconds label
    lv_label_set_text_fmt(label_time_seconds, "%02d", second);
    lv_obj_realign(label_time);

    // Date / Week / Day of Year
    uint16_t year = dateTimeController.Year();
    Controllers::DateTime::Months month = dateTimeController.Month();
    uint8_t day = dateTimeController.Day();
    int dayOfYear = dateTimeController.DayOfYear();

    const char* weekNumberFormat = (settingsController.GetClockType() == Controllers::Settings::ClockType::H24) ? "%V" : "%U";

    int daysInCurrentYear = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0 ? 366 : 365;
    uint16_t daysTillEndOfYearNumber = daysInCurrentYear - dayOfYear;

    char buffer[8];
    strftime(buffer, 8, weekNumberFormat, tmTime);
    uint8_t weekNumber = atoi(buffer);

    lv_label_set_text_fmt(label_date, (settingsController.GetClockType() == Controllers::Settings::ClockType::H24) ? "%02d %s" : "%s %02d", 
                          (settingsController.GetClockType() == Controllers::Settings::ClockType::H24) ? day : monthStr,
                          (settingsController.GetClockType() == Controllers::Settings::ClockType::H24) ? monthStr : day);

    lv_label_set_text_fmt(label_day_of_week, "%s", dateTimeController.DayOfWeekShortToString());
    lv_label_set_text_fmt(label_day_of_year, "%3d-%3d", dayOfYear, daysTillEndOfYearNumber);
    lv_label_set_text_fmt(label_week_number, "WK%02d", weekNumber);

    lv_obj_realign(label_day_of_week);
    lv_obj_realign(label_day_of_year);
    lv_obj_realign(label_week_number);
    lv_obj_realign(label_date);
  }

  // Heartbeat
  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x1B1B1B));
      lv_label_set_text_static(heartbeatValue, "");
    }
    lv_obj_realign(heartbeatIcon);
    lv_obj_realign(heartbeatValue);
  }

  // Steps
  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
    lv_obj_realign(stepValue);
    lv_obj_realign(stepIcon);
  }
}

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
