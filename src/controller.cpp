#include "controller.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>

using std::map;
using std::set;
using std::string;

using CS = ControllerSetup;

static const ControllerSetup::ValueDetails value_details[CS::VAL_LAST]{
    {"rotation",
     "rotation (0: 0 degs, 1: 90 degs, 2: 180 degs, 3: 270 degs)"},
    {"clock", "clock pin (GPIO number)"},
    {"data", "data pin (GPIO number)"},
    {"cs", "CS pin (GPIO number)"},
    {"dc", "DC pin (GPIO number)"},
    {"reset", "reset pin (GPIO number)"},
    {"d0", "D0 pin (GPIO number)"},
    {"d1", "D1 pin (GPIO number)"},
    {"d2", "D2 pin (GPIO number)"},
    {"d3", "D3 pin (GPIO number)"},
    {"d4", "D4 pin (GPIO number)"},
    {"d5", "D5 pin (GPIO number)"},
    {"d6", "D6 pin (GPIO number)"},
    {"d7", "D7 pin (GPIO number)"},
    {"wr", "WR pin (GPIO number)"},
    {"i2c_address", "I2C address (hex 01-fe)"},
    {"bus_number", "bus number"},
    {"cs_number", "CS number"},
};

map<string, int> value_name_to_id = {
    {"rotation", CS::VAL_ROTATION},
    {"clock", CS::VAL_CLOCK},
    {"data", CS::VAL_DATA},
    {"cs", CS::VAL_CS},
    {"dc", CS::VAL_DC},
    {"reset", CS::VAL_RESET},
    {"d0", CS::VAL_D0},
    {"d1", CS::VAL_D1},
    {"d2", CS::VAL_D2},
    {"d3", CS::VAL_D3},
    {"d4", CS::VAL_D4},
    {"d5", CS::VAL_D5},
    {"d6", CS::VAL_D6},
    {"d7", CS::VAL_D7},
    {"wr", CS::VAL_WR},
    {"i2c_address", CS::VAL_I2C_ADDRESS},
    {"bus_number", CS::VAL_BUS_NUMBER},
    {"cs_number", CS::VAL_CS_NUMBER},
};

int ControllerSetup::get_value_id(const string &value_name)
{
  auto it = value_name_to_id.find(value_name);
  if(it == value_name_to_id.end())
    return -1;  // not found

  return it->second;
}

const ControllerSetup::ValueDetails *ControllerSetup::get_value_details(int value_id)
{
  if(value_id >= 0 && value_id<VAL_LAST)
    return &value_details[value_id];
  else
    return nullptr;
}

static const ControllerSetup::ComDetails com_details[CS::COM_LAST] = {
    {
        "4W_SW_SPI",                                           // name
        "4 wire software SPI",                                 // desc
        {CS::VAL_CLOCK, CS::VAL_DATA, CS::VAL_CS, CS::VAL_DC}, // required
        {CS::VAL_RESET, CS::VAL_ROTATION}                      // optional
    },
    {
        "4W_HW_SPI",              // name
        "4 wire hardware SPI",    // desc
        {CS::VAL_CS, CS::VAL_DC}, // required
        {CS::VAL_BUS_NUMBER, CS::VAL_CS_NUMBER, CS::VAL_RESET,
         CS::VAL_ROTATION} // optional
    },
    {
        "3W_SW_SPI",                               // desc
        "3 wire software SPI",                     // desc
        {CS::VAL_CLOCK, CS::VAL_DATA, CS::VAL_CS}, // required
        {CS::VAL_RESET, CS::VAL_ROTATION}          // optional
    },
    {
        "SW_I2C",                                              // name
        "software I2C",                                        // desc
        {CS::VAL_CLOCK, CS::VAL_DATA},                         // required
        {CS::VAL_I2C_ADDRESS, CS::VAL_RESET, CS::VAL_ROTATION} // optional
    },
    {
        "HW_I2C",       // name
        "hardware I2C", // desc
        {},             // required
        {CS::VAL_CLOCK, CS::VAL_DATA, CS::VAL_BUS_NUMBER, CS::VAL_I2C_ADDRESS,
         CS::VAL_RESET, CS::VAL_ROTATION} // optional
    },
    {
        "8080", // name
        "8080", // desc
        {CS::VAL_D0, CS::VAL_D1, CS::VAL_D2, CS::VAL_D3, CS::VAL_D4, CS::VAL_D5,
         CS::VAL_D6, CS::VAL_D7, CS::VAL_WR, CS::VAL_CLOCK,
         CS::VAL_DATA},                   // required
        {CS::VAL_RESET, CS::VAL_ROTATION} // optional
    },
};

map<string, int> com_name_to_id = {
    {"4W_SW_SPI", CS::COM_4W_SW_SPI}, {"4W_HW_SPI", CS::COM_4W_HW_SPI},
    {"SPI", CS::COM_4W_HW_SPI},       {"3W_SW_SPI", CS::COM_3W_SW_SPI},
    {"SW_I2C", CS::COM_SW_I2C},       {"HW_I2C", CS::COM_HW_I2C},
    {"I2C", CS::COM_HW_I2C},          {"8080", CS::COM_8080},
};


int ControllerSetup::get_com_id(const string &com_name)
{
  auto it = com_name_to_id.find(com_name);
  if(it == com_name_to_id.end())
    return -1;  // not found

  return it->second;
}

const ControllerSetup::ComDetails *ControllerSetup::get_com_details(int com_id)
{
  if(com_id >= 0 && com_id<COM_LAST)
    return &com_details[com_id];
  else
    return nullptr;
}


string ControllerSetup::set_value(int val_type, uint8_t val)
{
  if (val_type < 0 || val_type >= VAL_LAST)
    return "invalid value type";

  if (val == U8X8_PIN_NONE)
    return "invalid value";

  if (val_type == VAL_ROTATION && val > 3)
    return "invalid value (must be 0, 1, 2 or 3)";

  if (val_type == VAL_I2C_ADDRESS && val == 0)
    return "invalid value (cannot be 0)";

  values[val_type] = val;
  return string();
}

uint8_t ControllerSetup::get_value(int val_type)
{
  if (val_type >= 0 && val_type < VAL_LAST)
    return values[val_type];
  else
    return U8X8_PIN_NONE;
}

string ControllerSetup::check_values(const string &com_name)
{
  auto com_details = get_com_details(get_com_id(com_name));
  if (com_details == nullptr)
    return "invalid protocol";

  // check for required values which have not been set
  for (auto val_type : com_details->values_required) {
    if (values[val_type] == U8X8_PIN_NONE)
      return value_details[val_type].id + " value has not been specified";
  }

  // check for values set which are not required (already checked) or optional
  for (int val_type = 0; val_type < VAL_LAST; val_type++) {
    if (values[val_type] != U8X8_PIN_NONE &&
        com_details->values_optional.count(val_type) == 0)
      return value_details[val_type].id + " value has been specified but is "
                                          "not compatible with this protocol";
  }
  return string(); // success
}

string ControllerSetup::init(U8G2 *u8g2)
{
  u8x8_SetUserPtr(u8g2->getU8x8(), NULL);
  const u8g2_cb_t *rot_funcs[] = {U8G2_R0, U8G2_R1, U8G2_R2, U8G2_R3};
  uint8_t rotation =
      (values[VAL_ROTATION] == U8X8_PIN_NONE) ? 0 : values[VAL_ROTATION];
  auto setup_func = controller->setup_func;

  string ret_msg; // return empty for success, or error message for error

  // Check compatibility of values set/unset with com protocol
  const string com_type = controller->com_type;
  if (!(ret_msg = check_values(com_type)).empty())
    return ret_msg;

  if (com_type == "4W_SW_SPI") {
    setup_func(u8g2->getU8g2(), rot_funcs[rotation],
               u8x8_byte_arm_linux_4wire_sw_spi, u8x8_arm_linux_gpio_and_delay);
    u8x8_SetPin_4Wire_SW_SPI(u8g2->getU8x8(), values[VAL_CLOCK],
                             values[VAL_DATA], values[VAL_CS], values[VAL_DC],
                             values[VAL_RESET]);
  }
  else if (com_type == "4W_HW_SPI") {
    setup_func(u8g2->getU8g2(), rot_funcs[rotation], u8x8_byte_arm_linux_hw_spi,
               u8x8_arm_linux_gpio_and_delay);
    u8x8_SetPin_4Wire_HW_SPI(u8g2->getU8x8(), values[VAL_CS], values[VAL_DC],
                             values[VAL_RESET]);
    uint8_t bus_num =
        (values[VAL_BUS_NUMBER] == U8X8_PIN_NONE) ? 0 : values[VAL_BUS_NUMBER];
    uint8_t cs_num =
        (values[VAL_CS_NUMBER] == U8X8_PIN_NONE) ? 0 : values[VAL_CS_NUMBER];
    u8g2arm_arm_init_hw_spi(u8g2->getU8x8(), bus_num, cs_num);
  }
  else if (com_type == "3W_SW_SPI") {
    setup_func(u8g2->getU8g2(), rot_funcs[rotation],
               u8x8_byte_arm_linux_3wire_sw_spi, u8x8_arm_linux_gpio_and_delay);
    u8x8_SetPin_3Wire_SW_SPI(u8g2->getU8x8(), values[VAL_CLOCK],
                             values[VAL_DATA], values[VAL_CS],
                             values[VAL_RESET]);
  }
  else if (com_type == "SW_I2C") {
    setup_func(u8g2->getU8g2(), rot_funcs[rotation], u8x8_byte_arm_linux_sw_i2c,
               u8x8_arm_linux_gpio_and_delay);
    u8x8_SetPin_SW_I2C(u8g2->getU8x8(), values[VAL_CLOCK], values[VAL_DATA],
                       values[VAL_RESET]);
    if (values[VAL_I2C_ADDRESS] != U8X8_PIN_NONE)
      u8g2->setI2CAddress(VAL_I2C_ADDRESS * 2);
  }
  else if (com_type == "HW_I2C") {
    setup_func(u8g2->getU8g2(), rot_funcs[rotation], u8x8_byte_arm_linux_hw_i2c,
               u8x8_arm_linux_gpio_and_delay);
    u8x8_SetPin_HW_I2C(u8g2->getU8x8(), values[VAL_RESET], values[VAL_CLOCK],
                       values[VAL_DATA]);
    uint8_t bus_num =
        (values[VAL_BUS_NUMBER] == U8X8_PIN_NONE) ? 1 : values[VAL_BUS_NUMBER];
    u8g2arm_arm_init_hw_i2c(u8g2->getU8x8(), bus_num);
    if (values[VAL_I2C_ADDRESS] != U8X8_PIN_NONE)
      u8g2->setI2CAddress(VAL_I2C_ADDRESS * 2);
  }
  else if (com_type == "8080") {
    setup_func(u8g2->getU8g2(), rot_funcs[rotation],
               u8x8_byte_arm_linux_8bit_8080mode,
               u8x8_arm_linux_gpio_and_delay);
    u8x8_SetPin_8Bit_8080(u8g2->getU8x8(), values[VAL_D0], values[VAL_D1],
                          values[VAL_D2], values[VAL_D3], values[VAL_D4],
                          values[VAL_D5], values[VAL_D6], values[VAL_D7],
                          values[VAL_WR], values[VAL_CS], values[VAL_DC],
                          values[VAL_RESET]);
  }
  else { // unsupported protocol for display type (shouldn't get here)
    ret_msg = "invalid protocol";
  }

  return ret_msg;
}

bool add_name_alias(map<string, map<string, map<string, size_t>>> &setup,
                    const string &type, const string &name,
                    const string &name_alias)
{
  // check type and original name before setting name alias
  auto kp0 = setup.find(type);
  if (kp0 != setup.end()) { // type found
    auto kp1 = kp0->second.find(name);
    if (kp1 != kp0->second.end()) {          // name found
      kp0->second[name_alias] = kp1->second; // set alias for name entry
      return true;
    }
  }
  return false; // failed to find either type or name
}

map<string, map<string, map<string, size_t>>> ControllerSetup::get_controllers()
{
  map<string, map<string, map<string, size_t>>> setup;
  for (size_t i = 0; i < controller_details_size; i++) {
    auto *p = &controller_details[i];
    setup[p->controller_type][p->display_name][p->com_type] = i;
  }
  add_name_alias(setup, "SH1106", "128X32_VISIONOX", "128X32");
  add_name_alias(setup, "SH1106", "128X64_NONAME", "128X64");
  add_name_alias(setup, "SSD1305", "128X32_NONAME", "128X32");
  add_name_alias(setup, "SSD1306", "128X64_NONAME", "128X64");
  add_name_alias(setup, "SSD1306", "64X32_NONAME", "64X32");
  add_name_alias(setup, "SSD1309", "128X64_NONAME0", "128X64");
  add_name_alias(setup, "SSD1329", "128X96_NONAME", "128X96");

  /*
  for (auto &kp0 : setup) {
    for (auto &kp1 : kp0.second) {
      for (auto &kp2 : kp1.second) {
        auto *p = &controller_details[kp2.second];
        //fprintf(stderr, "%s, %s, %s (%d, %d)\n", p->controller_type,
        //        p->display_name, p->com_type, p->width, p->height);
        fprintf(stderr, "%s, %s, %s (%d, %d)\n", kp0.first.c_str(),
                kp1.first.c_str(), kp2.first.c_str(), p->width, p->height);
      }
    }
  }
  */
  return setup;
}

bool ControllerSetup::set_controller(const string &type, const string &name,
                                     const string &com, string &errmsg)
{
  controller = nullptr;
  auto setup = get_controllers();
  auto it0 = setup.find(type);
  if (it0 == setup.end()) {
    errmsg = "invalid controller type '" + type + "' (select from";
    for (const auto &kp : setup) // controller display names
      errmsg += " " + kp.first + ",";
    errmsg.back() = ')';
  }
  else {
    auto it1 = it0->second.find(name);
    if (it1 == it0->second.end()) {
      errmsg = "controller type '" + type + "': invalid display name '" + name +
               "' (select from";
      for (const auto &kp : it0->second) // controller display names
        errmsg += " " + kp.first + ",";
      errmsg.back() = ')';
    }
    else {
      int com_id = get_com_id(com);
      string com_name = (com_id > 0) ? get_com_details(com_id)->name : com;

      auto it2 = it1->second.find(com_name);
      if (it2 == it1->second.end()) {
        errmsg = "controller type '" + type + "', name '" + name +
                 "': invalid communication protocol '" + com_name + "' (select from";
        for (const auto &kp : it1->second) // controller display protocols
          errmsg += " " + kp.first + ",";
        errmsg.back() = ')';
      }
      else // setup values were valid
        controller = &controller_details[it2->second];
    }
  }

  if (controller != nullptr) {
    auto *p = controller;
    fprintf(stderr, "%s, %s, %s (%d, %d)\n", p->controller_type,
            p->display_name, p->com_type, p->width, p->height);
  }

  return controller;
}

std::string ControllerSetup::to_str()
{
  using std::to_string;
  string s;
  s += "Controller=";
  if (controller != nullptr) {
    s += string(controller->controller_type) + "," + controller->display_name +
         "," + controller->com_type + "   [resolution " +
         to_string(controller->width) + "X" + to_string(controller->height) +
         "]\n";
  }
  else
    s += "not set\n";

  for (int val_num = 0; val_num < VAL_LAST; val_num++) {
    if (val_num == VAL_I2C_ADDRESS) {
      char i2caddr_str[3] = {0};
      snprintf(i2caddr_str, 3, "%02x", values[val_num]);
      s += value_details[val_num].id + "=" + i2caddr_str + "\n";
    }
    else
      s += value_details[val_num].id + "=" + to_string(values[val_num]) + "\n";
  }
  return s;
};

string set_setup_value(ControllerSetup &setup, const string &setting)
{
  // Prefix for setting error messages
  auto setting_msg = "setting '" + setting + "': ";

  auto c = setting.find('=');
  if (c == string::npos)
    return setting_msg + "does not contain '='";
  auto val_type_str = setting.substr(0, c);
  if (val_type_str.empty())
    return setting_msg + "no value type before '='";
  auto val_str = setting.substr(c + 1, string::npos);
  if (val_str.empty())
    return setting_msg + "no value after '='";

  // Prefix for setting value error messages
  string setting_value_msg = setting_msg + "value '" + val_str + "': ";

  long val = 255;
  char *p;
  if (val_type_str == "i2caddr")
    val = strtol(val_str.c_str(), &p, 16); // hexadecimal
  else
    val = strtol(val_str.c_str(), &p, 10); // decimal

  if (*p != '\0') // not the value string terminator
    return setting_value_msg + "invalid value character '" + (*p) + "'";

  if (val < 0)
    return setting_value_msg + "is negative";

  if (val > 255)
    return setting_value_msg + "is too big (>255)";

  // check one at at time, efficiency not needed
  int val_type_id = ControllerSetup::get_value_id(val_type_str);
  if (val_type_id < 0)
    return setting_msg + "value type '" + val_type_str + "': unknown type";

  string errmsg = setup.set_value(val_type_id, val);
  if (!errmsg.empty())
    return setting_value_msg + errmsg;

  return string(); // no error
};
