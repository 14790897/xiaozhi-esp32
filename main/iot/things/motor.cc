#include "iot/thing.h"
#include "board.h"

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>

#define TAG "Motor"

// PWM 配置
#define MOTOR_PWM_CHANNEL LEDC_CHANNEL_2
#define MOTOR_PWM_TIMER LEDC_TIMER_2
#define MOTOR_PWM_FREQ 5000                   // 5kHz
#define MOTOR_PWM_RESOLUTION LEDC_TIMER_8_BIT // 8位分辨率，值范围0-255

namespace iot {

class Motor : public Thing {
private:
    gpio_num_t gpio_num_ = GPIO_NUM_13; // 可以替换成其他支持 PWM 的 GPIO
    bool power_ = false;
    int speed_ = 0; // 0-100 范围

    void InitializePwm()
    {
        // 配置 LEDC 定时器
        ledc_timer_config_t timer_config = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = MOTOR_PWM_RESOLUTION,
            .timer_num = MOTOR_PWM_TIMER,
            .freq_hz = MOTOR_PWM_FREQ,
            .clk_cfg = LEDC_AUTO_CLK};
        ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

        // 配置 LEDC 通道
        ledc_channel_config_t channel_config = {
            .gpio_num = gpio_num_,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = MOTOR_PWM_CHANNEL,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = MOTOR_PWM_TIMER,
            .duty = 0, // 初始占空比为0
            .hpoint = 0};
        ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

        // 初始化渐变功能
        // ESP_ERROR_CHECK(ledc_fade_func_install(0));

        ESP_LOGI(TAG, "PWM initialized for motor control on GPIO %d", gpio_num_);
    }

    // 将0-100的速度值转换为0-255的PWM值
    uint32_t SpeedToDuty(int speed)
    {
        return (speed * 255) / 100;
    }

public:
    Motor() : Thing("Motor", "一个可调速的震动蛋"), power_(false)
    {
        InitializePwm();

        // 定义设备的属性
        properties_.AddBooleanProperty("power", "震动蛋是否运行", [this]() -> bool
                                       { return power_; });

        properties_.AddNumberProperty("speed", "震动蛋速度", [this]() -> int
                                      { return speed_; });

        // 定义设备可以被远程执行的指令
        methods_.AddMethod("TurnOn", "启动震动蛋", ParameterList(), [this](const ParameterList &parameters)
                           {
            power_ = true;
            speed_ = 100;
            ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, SpeedToDuty(speed_));
            ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);
            ESP_LOGI(TAG, "Motor turned ON with speed %d", speed_); });

        methods_.AddMethod("TurnOff", "停止震动蛋", ParameterList(), [this](const ParameterList &parameters)
                           {
            power_ = false;
            speed_ = 0;
            ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);
            ESP_LOGI(TAG, "Motor turned OFF"); });

        methods_.AddMethod("SetSpeed", "设置震动蛋速度", ParameterList({Parameter("speed", "速度(0-100)", kValueTypeNumber, true)}), [this](const ParameterList &parameters)
                           {
            int new_speed = static_cast<int>(parameters["speed"].number());
            if (new_speed <= 0) {
                power_ = false;
                speed_ = 0;
                ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, 0);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);
            } else {
                power_ = true;
                speed_ = (new_speed > 100) ? 100 : new_speed;
                ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, SpeedToDuty(speed_));
                ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);
            }
            ESP_LOGI(TAG, "Motor speed set to %d", speed_); });

        methods_.AddMethod("FadeSpeed", "渐变调节震动蛋速度", ParameterList({Parameter("target_speed", "目标速度(0-100)", kValueTypeNumber, true), Parameter("fade_time", "渐变时间(毫秒)", kValueTypeNumber, true)}), [this](const ParameterList &parameters)
                           {
            int target_speed = static_cast<int>(parameters["target_speed"].number());
            int fade_time = static_cast<int>(parameters["fade_time"].number());
            
            if (target_speed < 0) target_speed = 0;
            if (target_speed > 100) target_speed = 100;
            
            power_ = (target_speed > 0);
            speed_ = target_speed;
            
            // 使用LEDC的渐变功能
            ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, 
                                   SpeedToDuty(target_speed), fade_time);
            ledc_fade_start(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, LEDC_FADE_NO_WAIT);
            
            ESP_LOGI(TAG, "Motor fading to speed %d over %d ms", target_speed, fade_time); });
    }

    ~Motor()
    {
        // 清理资源
        ledc_stop(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, 0);
        ledc_fade_func_uninstall();
    }
};

} // namespace iot

DECLARE_THING(Motor);


