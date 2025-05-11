#include "iot/thing.h"
#include "board.h"

#include <driver/gpio.h>
#include <esp_log.h>

#define TAG "Motor"

namespace iot {

class Motor : public Thing {
private:
    gpio_num_t gpio_num_ = GPIO_NUM_12;
    bool power_ = false;
    int speed_ = 0;

    void InitializeGpio() {
        gpio_config_t config = {
            .pin_bit_mask = (1ULL << gpio_num_),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&config));
        gpio_set_level(gpio_num_, 0);
    }

public:
    Motor() : Thing("Motor", "一个简单的马达"), power_(false) {
        InitializeGpio();

        // 定义设备的属性
        properties_.AddBooleanProperty("power", "马达是否运行", [this]() -> bool
                                       { return power_; });

        properties_.AddNumberProperty("speed", "马达速度", [this]() -> int {
            return speed_;
        });

        // 定义设备可以被远程执行的指令
        methods_.AddMethod("TurnOn", "启动马达", ParameterList(), [this](const ParameterList& parameters) {
            power_ = true;
            speed_ = 100;
            gpio_set_level(gpio_num_, 1);
        });

        methods_.AddMethod("TurnOff", "停止马达", ParameterList(), [this](const ParameterList& parameters) {
            power_ = false;
            speed_ = 0;
            gpio_set_level(gpio_num_, 0);
        });
        
        methods_.AddMethod("SetSpeed", "设置马达速度", ParameterList({
            Parameter("speed", "速度(0-100)", kValueTypeNumber, true)
        }), [this](const ParameterList& parameters) {
            int new_speed = static_cast<int>(parameters["speed"].number());
            if (new_speed <= 0) {
                power_ = false;
                speed_ = 0;
                gpio_set_level(gpio_num_, 0);
            } else {
                power_ = true;
                speed_ = (new_speed > 100) ? 100 : new_speed;
                gpio_set_level(gpio_num_, 1);
            }
        });
    }
};

} // namespace iot

DECLARE_THING(Motor);

