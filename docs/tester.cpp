#include <Arduino.h>

#include <lvgl.h>
#include <TFT_eSPI.h>
#include "HX711.h"
#include <ESP_FlexyStepper.h>
#include "Adafruit_VL53L0X.h"

//////////////// hardware /////////////////
// screen
TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
/*screen resolution*/
const uint32_t screenWidth = 320;
const uint32_t screenHeight = 240;

lv_disp_draw_buf_t draw_buf;
lv_color_t buf[screenWidth * 10];

//bascula:
HX711 scale;
const uint8_t HX711_dout = 26;  //mcu > HX711 dout pin
const uint8_t HX711_sck = 25;   //mcu > HX711 sck pin

// motor
const uint8_t MOTOR_STEP_PIN = 32;
const uint8_t MOTOR_DIRECTION_PIN = 33;
ESP_FlexyStepper stepper;

// "medidor laser"
Adafruit_VL53L0X lox;  // = Adafruit_VL53L0X();

////////////  app   ////////////
// limite de maquina (mm)
const uint8_t MAX_LIMIT = 220;
const uint8_t MIN_LIMIT = 45;

const uint8_t MAX_WEIGHT = 50;

float weight_sensor = 0.0;
int distance_sensor = 50;
float rel_dist_to_move = 0.5;
float home_pos = 78.0;
uint32_t fps;
//test
const uint8_t res_data_len = 200;
lv_coord_t res_data[res_data_len];

uint8_t test_step = 0, test_dist = 5;
float test_max_weight, test_zero_pos, test_diff_pos, test_trigger_weigth;
bool test_stopping;
uint8_t test_saved_index;

////////////////   UI   ////////////

lv_obj_t *tiles;
lv_obj_t *fps_label;

lv_obj_t *tileTest;
lv_obj_t *test_weight_label, *test_weight1_label;
lv_obj_t *test_dist_label, *test_pos_label;
lv_obj_t *test_spin_trigger, *test_spin_dist;

lv_obj_t *tileResult;
lv_obj_t *res_chart, *res_label;
lv_chart_series_t *res_series;
lv_chart_cursor_t *res_cursor;

lv_obj_t *tileWeight;
lv_obj_t *wei_meter, *wei_weight_label;
lv_meter_indicator_t *wei_indicator;

lv_obj_t *tileRelative;
lv_obj_t *rel_weight_label, *rel_position_label;

lv_obj_t *tileAbsolute;
lv_obj_t *abs_spin_dist, *abs_spin_speed;
lv_obj_t *abs_spin_acc;
lv_obj_t *abs_weight_label, *abs_position_label;

lv_style_t st_cont, st_font_large;


/////////////// arduino  /////////////////

void setup() {
  Serial.begin(115200);

  setupTft();
  setupSensors();
  setupMotor();

  mainView();
}

void loop() {
  //int ti = millis();

  readSensors();

  //limit
  if (isLimit()) stepper.setTargetPositionToStop();

  calcFps(1);

  update();

  lv_timer_handler(); /* let the GUI do its work */

  //ti = millis() - ti;
  //if (ti > 100) Serial.printf("%d ms \n", ti);
}

/////////////// app  ///////////////////

void mainView() {
  init_styles();

  tiles = lv_tileview_create(lv_scr_act());

  tileTest = lv_tileview_add_tile(tiles, 0, 0, LV_DIR_BOTTOM | LV_DIR_RIGHT);
  tileResult = lv_tileview_add_tile(tiles, 1, 0, LV_DIR_LEFT);

  tileAbsolute = lv_tileview_add_tile(tiles, 0, 1, LV_DIR_BOTTOM | LV_DIR_RIGHT | LV_DIR_TOP);
  tileRelative = lv_tileview_add_tile(tiles, 1, 1, LV_DIR_LEFT);
  tileWeight = lv_tileview_add_tile(tiles, 0, 2, LV_DIR_TOP);

  makeTileTest(tileTest);
  makeTileAbsolute(tileAbsolute);
  makeTileRelative(tileRelative);
  makeTileWeight(tileWeight);
  makeTileResult(tileResult);

  lv_obj_set_tile(tiles, tileTest, LV_ANIM_OFF);

  fps_label = lv_label_create(lv_scr_act());
  lv_obj_align(fps_label, LV_ALIGN_TOP_RIGHT, -15, 10);
}

void update() {

  static int time = 0;
  lv_obj_t *curentTile = lv_tileview_get_tile_act(tiles);

  if (millis() - time > 100) {

    if (curentTile == tileRelative) {

      lv_label_set_text_fmt(rel_weight_label, "#ff0000 %05.2fKg#", weight_sensor);
      lv_label_set_text_fmt(rel_position_label, "#0000ff %06.2fmm#",
                            stepper.getCurrentPositionInMillimeters());

    } else if (curentTile == tileWeight) {

      lv_meter_set_indicator_value(wei_meter, wei_indicator, weight_sensor);
      lv_label_set_text_fmt(wei_weight_label, "%05.2f", weight_sensor);

    } else if (curentTile == tileAbsolute) {

      lv_label_set_text_fmt(abs_weight_label, "#ff0000 %05.2fKg#", weight_sensor);
      lv_label_set_text_fmt(abs_position_label, "#0000ff   %06.2fmm#",
                            stepper.getCurrentPositionInMillimeters());

    } else if (curentTile == tileTest || curentTile == tileResult) {
      updateTest();
    }


    lv_label_set_text_fmt(fps_label, "%d", fps);
    time = millis();
  }
}

bool isLimit() {
  int dir = stepper.getDirectionOfMotion();
  return distance_sensor > MAX_LIMIT && dir > 0 ||  //
         distance_sensor < MIN_LIMIT && dir < 0 ||  //
         weight_sensor > MAX_WEIGHT + 1;
}

void readSensors() {

  if (scale.is_ready())
    weight_sensor = scale.get_units(1);

  if (lox.isRangeComplete()) {
    int range = lox.readRange() + 2;
    distance_sensor = lowPassFilter(0.12, range, distance_sensor);
    //Serial.print("raw:"); Serial.print(range);
    //Serial.print(",filter:"); Serial.println(measure);
  }
}

void updateTest() {

  switch (test_step) {
    case 1:
      // cuando se supera los X kg
      if (weight_sensor >= test_trigger_weigth) {
        //guardamos la position actual

        test_zero_pos = stepper.getCurrentPositionInMillimeters();
        stepper.setSpeedInMillimetersPerSecond(0.5);
        stepper.setTargetPositionRelativeInMillimeters(test_dist);
        test_step = 2;
      } else if (stepper.motionComplete()) {
        test_step = 0;
        stepper.setSpeedInMillimetersPerSecond(2.0);
      }
      break;
    case 2:
      //cojemos el valor del peso mas alto
      test_max_weight = max(test_max_weight, weight_sensor);
      test_diff_pos = stepper.getCurrentPositionInMillimeters() - test_zero_pos;
      //cada 0.05mm (1mm/20)  guardamos el peso
      uint8_t index = (uint8_t)(test_diff_pos * 20);
      if (index > test_saved_index && 
        test_saved_index < res_data_len) 
      {
        res_data[test_saved_index++] = test_max_weight * 100;
      }
      // cuando superamos 1kg preparamos para parar
      if (weight_sensor > 1) test_stopping = true;

      if (
        test_stopping && weight_sensor < 0.5 ||  // cuando se rompio o
        weight_sensor > MAX_WEIGHT-2 ||            // el sensor mide mas 23kg o
        stepper.motionComplete()                 //se acabo de mover
      ) {

        //paramos el tinglado
        stepper.setTargetPositionToStop();
        stepper.setSpeedInMillimetersPerSecond(2.0);
        //rellenamos el resto del array con 0
        if (test_saved_index < res_data_len) {
          for (int i = test_saved_index; i < res_data_len; i++) {
            res_data[i] = 0;
          }
        }

        test_step = 0;
        // mostramos el resultado
        lv_label_set_text_fmt(res_label, "#ff0000 %05.2fKg#", test_max_weight);
        lv_chart_refresh(res_chart);
        lv_obj_set_tile(tiles, tileResult, LV_ANIM_ON);
      }
      break;
  }

  lv_label_set_text_fmt(test_weight_label, "#ff0000 %05.2fKg#", weight_sensor);
  lv_label_set_text_fmt(test_weight1_label, "#ff0000 %05.2fKg#", test_max_weight);
  lv_label_set_text_fmt(test_dist_label, "#0000ff %05.3fmm#", test_diff_pos);
  lv_label_set_text_fmt(test_pos_label, "#0000ff %05.3fmm#", stepper.getCurrentPositionInMillimeters());
}

void event_test_run(lv_event_t *e) {

  lv_obj_t *obj = lv_event_get_target(e);

  uint8_t btn_id = lv_btnmatrix_get_selected_btn(obj);

  test_diff_pos = test_max_weight = 0.0;
  test_saved_index = 0;
  test_stopping = false;

  test_trigger_weigth = lv_spinbox_get_value(test_spin_trigger) / 100.0;
  test_dist = lv_spinbox_get_value(test_spin_dist);

  scale.tare(1);

  if (btn_id == 0) {  //run
    stepper.setSpeedInMillimetersPerSecond(0.8);
    stepper.setTargetPositionRelativeInMillimeters(test_dist);
    test_step = 1;
  } else if (btn_id == 1) {  //go home
    stepper.setSpeedInMillimetersPerSecond(2);
    stepper.setTargetPositionInMillimeters(home_pos);
  } else {//reset
    if (stepper.getDirectionOfMotion() != 0) stepper.setTargetPositionToStop();
    test_step = 0;
    stepper.setSpeedInMillimetersPerSecond(2.0);
  }
}

/////////////// tools UI  ///////////////////

lv_obj_t *makeLabel(lv_obj_t *cont, const char *text, bool large_font = true) {
  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_recolor(label, true);
  lv_label_set_text(label, text);
  if (large_font) lv_obj_add_style(label, &st_font_large, 0);
  return label;
}


// makeSpinbox(container, "Trigger (Kg)", value, max, digit, separator, min);
lv_obj_t *makeSpinbox(lv_obj_t *cont1,
                      const char *text, int value, int max,
                      uint8_t digit, uint8_t separator, int min = 0) {
  lv_obj_t *cont = lv_obj_create(cont1);
  lv_obj_set_size(cont, 280, LV_SIZE_CONTENT);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_style(cont, &st_cont, 0);

  lv_obj_t *label = lv_label_create(cont);
  lv_obj_set_width(label, 110);
  lv_label_set_text(label, text);

  lv_obj_t *spin = lv_spinbox_create(cont);
  lv_spinbox_set_range(spin, min, max);
  lv_spinbox_set_digit_format(spin, digit, separator);
  lv_spinbox_set_value(spin, value);
  lv_spinbox_step_prev(spin);
  lv_obj_set_width(spin, 80);

  lv_coord_t h = lv_obj_get_height(spin);

  lv_obj_t *btn = lv_btn_create(cont);
  lv_obj_set_size(btn, h, h);
  lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_PLUS, 0);
  lv_obj_add_event_cb(btn, event_spin_plus, LV_EVENT_ALL, spin);

  lv_obj_t *btn1 = lv_btn_create(cont);
  lv_obj_set_size(btn1, h, h);
  lv_obj_set_style_bg_img_src(btn1, LV_SYMBOL_MINUS, 0);
  lv_obj_add_event_cb(btn1, event_spin_minus, LV_EVENT_ALL, spin);

  lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_align_to(btn1, label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
  lv_obj_align_to(spin, btn1, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
  lv_obj_align_to(btn, spin, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

  return spin;
}

lv_obj_t *makeBtn(lv_obj_t *cont, const char *text, int w, int h) {
  lv_obj_t *btn = lv_btn_create(cont);
  lv_obj_set_size(btn, w, h);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

lv_obj_t *makeBtnPanic(lv_obj_t *cont, int w, int h) {
  lv_obj_t *btn = makeBtn(cont, "   " LV_SYMBOL_WARNING "\nStop", w, h);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_state(btn, LV_STATE_CHECKED);
  lv_obj_add_event_cb(btn, event_panic, LV_EVENT_VALUE_CHANGED, NULL);

  return btn;
}

/////////////////// Tiles UI ///////////////////

void makeTileTest(lv_obj_t *cont) {

  lv_obj_t *container = lv_obj_create(cont);
  lv_obj_set_size(container, 300, 60);
  lv_obj_align(container, LV_ALIGN_TOP_LEFT, 5, 5);
  lv_obj_set_height(container, LV_SIZE_CONTENT);
  lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

  test_weight_label = makeLabel(container, "#ff0000 00.00Kg#");
  lv_obj_set_width(test_weight_label, 140);
  test_weight1_label = makeLabel(container, "#ff0000 00.00Kg#");
  lv_obj_set_width(test_weight1_label, 140);
  test_dist_label = makeLabel(container, "#ff00ff 000.00mm#");
  lv_obj_set_width(test_dist_label, 140);
  test_pos_label = makeLabel(container, "#ff00ff 000.00mm#");
  lv_obj_set_width(test_pos_label, 140);

  lv_obj_align(test_weight_label, LV_ALIGN_TOP_LEFT, 5, -6);
  lv_obj_align_to(test_weight1_label, test_weight_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  lv_obj_align_to(test_pos_label, test_weight_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_obj_align_to(test_dist_label, test_pos_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  test_spin_trigger = makeSpinbox(container, "Trigger (Kg)", 30, 500, 3, 1);
  lv_obj_t *parent = lv_obj_get_parent(test_spin_trigger);
  lv_obj_align_to(parent, test_pos_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

  test_spin_dist = makeSpinbox(container, "Dist (mm)", 5, 10, 2, 0, 1);
  lv_obj_t *p = lv_obj_get_parent(test_spin_dist);
  lv_obj_align_to(p, parent, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 3);

  static const char *btnmd_map[] = { "Test", "  Go\nhome", "Reset", "" };

  lv_obj_t *btnmd = lv_btnmatrix_create(cont);
  lv_obj_set_size(btnmd, 210, 60);
  lv_btnmatrix_set_map(btnmd, btnmd_map);
  lv_obj_add_event_cb(btnmd, event_test_run, LV_EVENT_CLICKED, NULL);
  lv_obj_align(btnmd, LV_ALIGN_BOTTOM_LEFT, 5, -10);

  lv_obj_t *btnPanic = makeBtnPanic(cont, 80, 60);
  lv_obj_align_to(btnPanic, btnmd, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}

void makeTileResult(lv_obj_t *cont) {

  res_chart = lv_chart_create(cont);

  lv_obj_set_size(res_chart, 235, 190);
  lv_chart_set_zoom_x(res_chart, LV_IMG_ZOOM_NONE * 3);
  lv_obj_align(res_chart, LV_ALIGN_TOP_LEFT, 55, 10);


  lv_chart_set_type(res_chart, LV_CHART_TYPE_LINE);
  lv_chart_set_range(res_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 3000);

  const uint8_t num_x_label = 41;
  const uint8_t num_y_label = 11;

  lv_chart_set_div_line_count(res_chart, num_y_label, num_x_label);

  lv_chart_set_axis_tick(res_chart, LV_CHART_AXIS_PRIMARY_X, 10, 5, num_x_label, 5, true, 50);
  lv_chart_set_axis_tick(res_chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, num_y_label, 3, true, 30);

  lv_obj_add_event_cb(res_chart, event_res_chart, LV_EVENT_ALL, NULL);
  lv_obj_refresh_ext_draw_size(res_chart);

  res_cursor = lv_chart_add_cursor(res_chart, lv_palette_main(LV_PALETTE_GREY), LV_DIR_LEFT | LV_DIR_BOTTOM);
  res_series = lv_chart_add_series(res_chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  lv_obj_set_style_line_width(res_chart, 1, LV_PART_INDICATOR);
  lv_obj_set_style_size(res_chart, 0, LV_PART_INDICATOR);

  lv_chart_set_ext_y_array(res_chart, res_series, res_data);
  lv_chart_set_point_count(res_chart, sizeof(res_data) / sizeof(res_data[0]));

  lv_obj_t *slider = lv_slider_create(cont);
  lv_slider_set_range(slider, LV_IMG_ZOOM_NONE, LV_IMG_ZOOM_NONE * 7);
  lv_obj_set_size(slider, 10, 190);
  lv_obj_align_to(slider, res_chart, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  lv_slider_set_value(slider, LV_IMG_ZOOM_NONE * 3, LV_ANIM_ON);
  lv_obj_add_event_cb(slider, event_res_slider, LV_EVENT_VALUE_CHANGED, NULL);

  res_label = makeLabel(cont, "#0000ff Result#");
  lv_obj_align_to(res_label, res_chart, LV_ALIGN_TOP_MID, -10, 5);

  lv_obj_t *btn = makeBtn(cont, "Print", LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_align_to(btn, res_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  lv_obj_add_event_cb(btn, event_res_print, LV_EVENT_CLICKED, NULL);

  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_text(label, "\tKg\nmm");
  lv_obj_align_to(label, res_chart, LV_ALIGN_BOTTOM_LEFT, -55, 45);
}

void makeTileRelative(lv_obj_t *cont) {

  lv_obj_t *cont1 = lv_obj_create(cont);
  lv_obj_set_size(cont1, 300, 60);
  lv_obj_align(cont1, LV_ALIGN_TOP_LEFT, 5, 10);
  lv_obj_clear_flag(cont1, LV_OBJ_FLAG_SCROLLABLE);

  rel_weight_label = makeLabel(cont1, "#ff0000 00.00Kg#");
  rel_position_label = makeLabel(cont1, "#ff00ff 00.00 mm#");

  lv_obj_align(rel_weight_label, LV_ALIGN_TOP_LEFT, -5, 0);
  lv_obj_align_to(rel_position_label, rel_weight_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  static const char *btnm_map[] = { "0.5mm", "2mm", "10mm", "50mm", "" };

  lv_obj_t *btnm = lv_btnmatrix_create(cont);
  lv_obj_set_size(btnm, 300, 60);
  lv_btnmatrix_set_map(btnm, btnm_map);
  lv_btnmatrix_set_one_checked(btnm, true);
  lv_btnmatrix_set_btn_ctrl_all(btnm, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_btn_ctrl(btnm, 0, LV_BTNMATRIX_CTRL_CHECKED);

  lv_obj_align_to(btnm, cont1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

  lv_obj_add_event_cb(btnm, event_rel_dist, LV_EVENT_VALUE_CHANGED, NULL);

  static const char *btnmd_map[] = { LV_SYMBOL_LEFT " " LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT " " LV_SYMBOL_RIGHT,"  Save\nHome", "" };

  lv_obj_t *btnmd = lv_btnmatrix_create(cont);
  lv_obj_set_size(btnmd, 210, 75);
  lv_btnmatrix_set_map(btnmd, btnmd_map);

  lv_obj_align_to(btnmd, btnm, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

  lv_obj_add_event_cb(btnmd, event_rel_run, LV_EVENT_CLICKED, NULL);


  lv_obj_t *btnPanic = makeBtnPanic(cont, 80, 75);
  lv_obj_align_to(btnPanic, btnmd, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}

void makeTileWeight(lv_obj_t *cont) {
  wei_meter = lv_meter_create(cont);
  lv_obj_center(wei_meter);
  lv_obj_set_size(wei_meter, 200, 200);

  /*Add a scale first*/
  lv_meter_scale_t *scale = lv_meter_add_scale(wei_meter);
  lv_meter_set_scale_ticks(wei_meter, scale, 26, 2, 10, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(wei_meter, scale, 5, 4, 15, lv_color_black(), 10);
  lv_meter_set_scale_range(wei_meter, scale, 0, 25, 270, 135);

  /*Add a red arc to the end*/
  lv_meter_indicator_t *indic = lv_meter_add_arc(wei_meter, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(wei_meter, indic, 20);
  lv_meter_set_indicator_end_value(wei_meter, indic, 25);

  /*Make the tick lines red at the end of the scale*/
  indic = lv_meter_add_scale_lines(wei_meter, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
  lv_meter_set_indicator_start_value(wei_meter, indic, 20);
  lv_meter_set_indicator_end_value(wei_meter, indic, 25);

  /*Add a needle line indicator*/
  wei_indicator = lv_meter_add_needle_line(wei_meter, scale, 4, lv_palette_main(LV_PALETTE_GREY), -10);
  lv_meter_set_indicator_value(wei_meter, wei_indicator, 0);

  //label
  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_text(label, "Kg");
  lv_obj_align_to(label, wei_meter, LV_ALIGN_BOTTOM_MID, 0, 10);

  wei_weight_label = lv_label_create(cont);
  lv_label_set_text(wei_weight_label, "00.00");
  lv_obj_align_to(wei_weight_label, label, LV_ALIGN_OUT_TOP_MID, 0, 0);


  lv_obj_t *btn = lv_btn_create(cont);
  lv_obj_set_size(btn, 40, 40);

  label = lv_label_create(btn);
  lv_label_set_text(label, "Tare");
  lv_obj_center(label);


  lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -25, -20);

  lv_obj_add_event_cb(btn, event_wei_tare, LV_EVENT_CLICKED, NULL);
}

void makeTileAbsolute(lv_obj_t *cont) {

  abs_position_label = makeLabel(cont, "#ff00ff 00.00 mm#");
  lv_obj_set_width(abs_position_label, 150);
  lv_obj_align(abs_position_label, LV_ALIGN_TOP_LEFT, 10, 6);

  abs_weight_label = makeLabel(cont, "#0000ff 2.00mm/s#");
  lv_obj_set_width(abs_weight_label, 150);
  lv_obj_align_to(abs_weight_label, abs_position_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  lv_obj_t *cont1 = lv_obj_create(cont);
  lv_obj_set_size(cont1, 300, LV_SIZE_CONTENT);
  lv_obj_align_to(cont1, abs_position_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_obj_clear_flag(cont1, LV_OBJ_FLAG_SCROLLABLE);

  ////////////////////////////////////////////////////////////

  abs_spin_dist = makeSpinbox(cont1, "Distance", 9000, 22000, 5, 3);
  lv_obj_t *parent = lv_obj_get_parent(abs_spin_dist);
  lv_obj_align(parent, LV_ALIGN_TOP_LEFT, 0, 0);

  ////////////////////////////////////////////////////////

  abs_spin_speed = makeSpinbox(cont1, "Speed", 200, 500, 3, 1);
  lv_obj_t *parent1 = lv_obj_get_parent(abs_spin_speed);
  lv_obj_align_to(parent1, parent, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

  ////////////////////////////////////////////////////////

  abs_spin_acc = makeSpinbox(cont1, "Acc/Dec", 300, 500, 3, 1);
  parent = lv_obj_get_parent(abs_spin_acc);
  lv_obj_align_to(parent, parent1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

  //////////////////////////////////////////////////////////

  lv_obj_t *btn1 = makeBtn(cont, "Run", 65, 40);
  lv_obj_add_event_cb(btn1, event_abs_run, LV_EVENT_CLICKED, NULL);
  lv_obj_align(btn1, LV_ALIGN_BOTTOM_LEFT, 10, -6);

  lv_obj_t *btn = makeBtn(cont, "  Save\nHome", 65, 40);
  lv_obj_add_event_cb(btn, event_abs_home, LV_EVENT_CLICKED, NULL);
  lv_obj_align_to(btn, btn1, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  btn1 = makeBtnPanic(cont, 144, 40);
  lv_obj_align_to(btn1, btn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}

/////////////// events UI  //////////////////////

void event_rel_run(lv_event_t *e) {

  lv_obj_t *obj = lv_event_get_target(e);

  int direction;
  uint btn_id = lv_btnmatrix_get_selected_btn(obj);
  
  if(btn_id == 2){
    home_pos = stepper.getCurrentPositionInMillimeters();
    return;
  }
  else if (btn_id == 0) direction = 1;
  else if(btn_id == 1) direction = -1;
  

  if (!isLimit())
    stepper.setTargetPositionRelativeInMillimeters(rel_dist_to_move * direction);
}

void event_rel_dist(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);
  uint32_t btn_id = lv_btnmatrix_get_selected_btn(obj);

  if (btn_id == 0) rel_dist_to_move = 0.5;
  else if (btn_id == 1) rel_dist_to_move = 2.0;
  else if (btn_id == 2) rel_dist_to_move = 10.0;
  else if (btn_id == 3) rel_dist_to_move = 50.0;
}

void event_abs_run(lv_event_t *e) {

  float dist = lv_spinbox_get_value(abs_spin_dist) / 100.0;
  float speed = lv_spinbox_get_value(abs_spin_speed) / 100.0;
  float acc = lv_spinbox_get_value(abs_spin_acc) / 100.0;

  if (!isLimit()) {
    stepper.setSpeedInMillimetersPerSecond(speed);
    stepper.setAccelerationInMillimetersPerSecondPerSecond(acc);
    stepper.setDecelerationInMillimetersPerSecondPerSecond(acc);
    stepper.setTargetPositionInMillimeters(dist);
  }
}

void event_abs_home(lv_event_t *e) {
  home_pos = stepper.getCurrentPositionInMillimeters();
}

void event_spin_plus(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    lv_obj_t *spin = (lv_obj_t *)lv_event_get_user_data(e);
    lv_spinbox_increment(spin);
  }
}

void event_spin_minus(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    lv_obj_t *spin = (lv_obj_t *)lv_event_get_user_data(e);
    lv_spinbox_decrement(spin);
  }
}

void event_panic(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);
  bool checked = lv_obj_has_state(obj, LV_STATE_CHECKED);
  //Serial.printf("Toggled %d\n", checked);

  if (!checked) stepper.emergencyStop(true);
  else stepper.releaseEmergencyStop();
}

void event_wei_tare(lv_event_t *e) {
  //Serial.println("Tare");
  scale.tare();  // reset the scale to 0
}

void event_res_slider(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);
  int32_t v = lv_slider_get_value(obj);
  lv_chart_set_zoom_x(res_chart, v);
}

void event_res_print(lv_event_t *e) {
  for (int i = 0; i < res_data_len; i++) {
    float kg = res_data[i] / 100.0;
    float mm = i / 20.0;  // , %.3f, mm
    Serial.printf("%.2f\n", kg);
  }
}

void event_res_chart(lv_event_t *e) {
  static int32_t last_id = -1;
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  if (code == LV_EVENT_VALUE_CHANGED) {
    last_id = lv_chart_get_pressed_point(obj);
    if (last_id != LV_CHART_POINT_NONE) {
      lv_chart_set_cursor_point(obj, res_cursor, NULL, last_id);
    }
  } else if (code == LV_EVENT_DRAW_PART_END) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (!lv_obj_draw_part_check_type(dsc, &lv_chart_class, LV_CHART_DRAW_PART_CURSOR)) return;
    if (dsc->p1 == NULL || dsc->p2 == NULL || dsc->p1->y != dsc->p2->y || last_id < 0) return;

    lv_coord_t *data_array = lv_chart_get_y_array(res_chart, res_series);
    lv_coord_t v = data_array[last_id];
    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%4.2f\n %3.2f", v / 100.0, last_id / 20.0);

    lv_point_t size;
    lv_txt_get_size(&size, buf, LV_FONT_DEFAULT, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

    lv_area_t a;
    a.y2 = dsc->p1->y - 5;
    a.y1 = a.y2 - size.y - 10;
    a.x1 = dsc->p1->x + 10;
    a.x2 = a.x1 + size.x + 10;

    lv_draw_rect_dsc_t draw_rect_dsc;
    lv_draw_rect_dsc_init(&draw_rect_dsc);
    draw_rect_dsc.bg_color = lv_palette_main(LV_PALETTE_GREY);
    draw_rect_dsc.radius = 3;

    lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);

    lv_draw_label_dsc_t draw_label_dsc;
    lv_draw_label_dsc_init(&draw_label_dsc);
    draw_label_dsc.color = lv_color_white();
    a.x1 += 5;
    a.x2 -= 5;
    a.y1 += 5;
    a.y2 -= 5;
    lv_draw_label(dsc->draw_ctx, &draw_label_dsc, &a, buf, NULL);

  } else if (code == LV_EVENT_DRAW_PART_BEGIN) {

    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (!lv_obj_draw_part_check_type(dsc, &lv_chart_class, LV_CHART_DRAW_PART_TICK_LABEL) /* || (dsc->value > 4)*/) return;

    char buf[40];

    if (dsc->id == LV_CHART_AXIS_PRIMARY_Y && dsc->text) {

      snprintf(buf, sizeof(buf), "%3.1f", (float)dsc->value / 100.0);
      lv_snprintf(dsc->text, dsc->text_length, "%s", buf);

    } else if (dsc->id == LV_CHART_AXIS_PRIMARY_X && dsc->text) {

      //if(dsc->value < NUM_HIST_X_LABELS -1 )  {
      snprintf(buf, sizeof(buf), "%3.2f", (float)dsc->value * 0.25);
      lv_snprintf(dsc->text, dsc->text_length, "%s", buf);
      //} else dsc->text[0] = 0;
    } /**/
  }
}
/////////////// style   ///////////////////

void init_styles() {
  lv_style_init(&st_cont);
  lv_style_set_border_side(&st_cont, LV_BORDER_SIDE_NONE);
  lv_style_set_radius(&st_cont, 0);
  lv_style_set_pad_ver(&st_cont, 0);
  lv_style_set_pad_left(&st_cont, 0);

  lv_style_init(&st_font_large);
  lv_style_set_text_font(&st_font_large, &lv_font_montserrat_20);
}

/////////////// setups  ///////////////////

void setupMotor() {
  stepper.connectToPins(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN);

  stepper.setStepsPerMillimeter(7680);
  stepper.setSpeedInMillimetersPerSecond(2);
  stepper.setAccelerationInMillimetersPerSecondPerSecond(4);
  stepper.setDecelerationInMillimetersPerSecondPerSecond(4);
  stepper.setCurrentPositionInMillimeters(distance_sensor);
  stepper.setTargetPositionInMillimeters(distance_sensor);
  //update
  stepper.startAsService(0);
}

void setupSensors() {

  scale.begin(HX711_dout, HX711_sck);
  float s = -1299724 / 25.0;
  scale.set_scale(s);  // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();        // reset the scale to 0

  //vl.begin();
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while (true)
      ;
  } else {
    lox.startRangeContinuous();

    int t = millis();
    while (millis() - t < 1000) {
      if (lox.isRangeComplete()) {
        int range = lox.readRange() + 2;
        distance_sensor = lowPassFilter(0.14, range, distance_sensor);
      }
    }
  }
}

void setupTft() {

  lv_init();


  tft.begin();        /* TFT init */
  tft.setRotation(1); /* Landscape orientation, flipped */

  /*Set the touchscreen calibration data,
     the actual data for your display can be aquired using
     the Generic -> Touch_calibrate example from the TFT_eSPI library 275, 3620, 264, 3532, 1 */
  uint16_t calData[5] = { 371, 3495, 277, 3490, 7 };
  tft.setTouch(calData);

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
}

/////////////// metod TFT ////////////////////

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY, 600);

  if (!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates/**/
    data->point.x = touchX;
    data->point.y = touchY;
  }
}

/////////////// ect  ///////////////////

inline void calcFps(const int seconds) {
  // Create static variables so that the code and variables can
  // all be declared inside a function
  static unsigned long lastMillis;
  static unsigned long frameCount;

  // It is best if we declare millis() only once
  unsigned long now = millis();
  frameCount++;
  if (now - lastMillis >= seconds * 1000) {
    fps = frameCount / seconds;
    //Serial.println(framesPerSecond);
    frameCount = 0;
    lastMillis = now;
  }
}

//para suavizar datos
int lowPassFilter(float alpha, int value, int prev) {
  // si alpha vale 0.1 = 10% valor actual + 90% valor anterior
  return alpha * value + (1 - alpha) * prev;
}
float lowPassFilter(float alpha, float value, float prev) {
  return alpha * value + (1.0 - alpha) * prev;
}
