#include <application.h>

// Service mode interval defines how much time
#define SERVICE_MODE_INTERVAL (15 * 60 * 1000)
#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)
#define ACCELEROMETER_UPDATE_NORMAL_INTERVAL (100)

#define RADIO_DELAY 5000


bc_lis2dh12_t lis2dh12;
bc_lis2dh12_result_g_t result;

bc_led_t led;

float magnitude;

// This function dispatches battery events
void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    // Update event?
    if (event == BC_MODULE_BATTERY_EVENT_UPDATE)
    {
        float voltage;

        // Read battery voltage
        if (bc_module_battery_get_voltage(&voltage))
        {
            bc_log_info("APP: Battery voltage = %.2f", voltage);

            // Publish battery voltage
            bc_radio_pub_battery(&voltage);
        }
    }
}

// This function dispatches accelerometer events
void lis2dh12_event_handler(bc_lis2dh12_t *self, bc_lis2dh12_event_t event, void *event_param)
{
    // Update event?
    if (event == BC_LIS2DH12_EVENT_UPDATE)
    {
        // Successfully read accelerometer vectors?
        if (bc_lis2dh12_get_result_g(self, &result))
        {
            magnitude = pow(result.x_axis, 2) + pow(result.y_axis, 2) + pow(result.z_axis, 2);
            magnitude = sqrt(magnitude);
            //bc_log_info("%.4f", magnitude);
        }

        if(magnitude > 4)
        {
            static bc_tick_t radio_delay = 0;
            if (bc_tick_get() >= radio_delay)
            {
                bc_led_pulse(&led, 100);
                bc_radio_pub_bool("future/trigger", true);

                radio_delay = bc_tick_get() + RADIO_DELAY;
            }
        }
        
    }
}


void application_init(void)
{

    // Initialize log
    bc_log_init(BC_LOG_LEVEL_INFO, BC_LOG_TIMESTAMP_ABS);
    bc_log_info("APP: Reset");

    // Initialize battery
    bc_module_battery_init();
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    // Initialize accelerometer
    bc_lis2dh12_init(&lis2dh12, BC_I2C_I2C0, 0x19);
    bc_lis2dh12_set_event_handler(&lis2dh12, lis2dh12_event_handler, NULL);
    bc_lis2dh12_set_resolution(&lis2dh12, BC_LIS2DH12_RESOLUTION_8BIT);
    bc_lis2dh12_set_scale(&lis2dh12, BC_LIS2DH12_SCALE_16G);
    bc_lis2dh12_set_update_interval(&lis2dh12, ACCELEROMETER_UPDATE_NORMAL_INTERVAL);

    // Initialize radio
    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING);

    // Send radio pairing request
    bc_radio_pairing_request("future-teller", VERSION);
}
