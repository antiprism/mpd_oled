#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <libu8g2arm/u8g2_controllers.h>
#include <libu8g2arm/u8g2arm.h>
#include <libu8g2arm/U8g2lib.h>


#include <string>
#include <vector>
#include <set>
#include <map>

class ControllerSetup {
  public:
    enum {
      VAL_ROTATION,
      VAL_CLOCK,
      VAL_DATA,
      VAL_CS,
      VAL_DC,
      VAL_RESET,
      VAL_D0,
      VAL_D1,
      VAL_D2,
      VAL_D3,
      VAL_D4,
      VAL_D5,
      VAL_D6,
      VAL_D7,
      VAL_WR,
      VAL_I2C_ADDRESS,
      VAL_BUS_NUMBER,
      VAL_CS_NUMBER,
      VAL_LAST
    };

    struct ValueDetails {
      std::string id;
      std::string desc;
    };

    static int get_value_id(const std::string &value_name);
    static const ValueDetails *get_value_details(int value_id);

    enum {
      COM_4W_SW_SPI,
      COM_4W_HW_SPI,
      COM_3W_SW_SPI,
      COM_SW_I2C,
      COM_HW_I2C,
      COM_8080,
      COM_LAST
    };

    struct ComDetails {
      std::string name;
      std::string desc;
      std::set<int> values_required;
      std::set<int> values_optional;
    };

    static int get_com_id(const std::string &com_name);
    static const ComDetails *get_com_details(int com_id);
    static std::map<std::string,
                    std::map<std::string, std::map<std::string, size_t>>>
    get_controllers();

    bool set_controller(const std::string &type, const std::string &name,
                        const std::string &com, std::string &msg);
    std::string set_value(int val_type, uint8_t val);
    uint8_t get_value(int val_type);
    std::string check_values(const std::string &com_name);

    std::string to_str();

    std::string init(U8G2 *u8g2);

  private:
    std::vector<uint8_t> values = std::vector<uint8_t>(VAL_LAST, U8X8_PIN_NONE);
    const controller_details_t *controller = nullptr;
};

std::string set_setup_value(ControllerSetup &setup, const std::string &setting);

#endif // CONTROLLER_H

