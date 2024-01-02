#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "math.h"

#include "pico-usbnet/USBNetwork.h"
#include "pico-usbnet/TCP.h"

#define LED_PIN 25

TCP tcp;

float frequency = 200.0;                        // Sine wave frequency in Hz
float sampleRate = 250000;                      // Sample rate in samples per second
int waveLength = (int)(sampleRate / frequency); // Number of samples per wave cycle
int counter = 0;

std::vector<dhcp_entry_t> dhcp = {
    {{0}, IPADDR4_INIT_BYTES(192, 168, 7, 3), 24 * 60 * 60},
    {{0}, IPADDR4_INIT_BYTES(192, 168, 7, 4), 24 * 60 * 60},
    {{0}, IPADDR4_INIT_BYTES(192, 168, 7, 5), 24 * 60 * 60},
};

USBNetwork network(
    IPADDR4_INIT_BYTES(192, 168, 7, 6),   // IP address
    IPADDR4_INIT_BYTES(255, 255, 255, 0), // Netmask
    IPADDR4_INIT_BYTES(192, 168, 7, 2),   // Gateway
    dhcp
);

void blinkPattern(int pin, int blinks, int duration)
{
    for (int i = 0; i < blinks; i++)
    {
        gpio_put(pin, 1);
        sleep_ms(duration);
        gpio_put(pin, 0);
        sleep_ms(duration);
    }
}

bool sendValue(struct repeating_timer *t)
{
    float wave = sin(2 * M_PI * frequency * (counter / sampleRate));
    tcp.send(&wave, sizeof(wave));

    // Reset counter after each cycle
    counter = (counter + 1) % waveLength;

    return true;
}

void acceptCallback(struct tcp_pcb *newpcb, err_t err)
{
    blinkPattern(LED_PIN, 15, 25);
}

void closeCallback()
{
    blinkPattern(LED_PIN, 15, 25);
}

int main()
{
    // Set up GPIO
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Set up ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    // Set up network
    network.init();

    // Initialize TCP
    tcp.init();

    // Setup TCP callbacks
    tcp.onAccept(acceptCallback);
    tcp.onClose(closeCallback);

    // Start listening
    tcp.bind(IP_ADDR_ANY, 5555);
    tcp.listen();

    while (true)
    {
        sendValue(NULL);
        sleep_us(1e6 / sampleRate);

        network.work();
    }

    return 0;
}
