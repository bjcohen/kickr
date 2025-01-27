#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
             "Console device is not ACM CDC UART device");

#define P0_NODE DT_NODELABEL(gpio0)
#define P1_NODE DT_NODELABEL(gpio1)
static const struct device *p0 = DEVICE_DT_GET(P0_NODE);
static const struct device *p1 = DEVICE_DT_GET(P1_NODE);
static struct gpio_callback cb_data;

void callback(const struct device *dev, struct gpio_callback *cb,
              uint32_t pins) {
  int v = gpio_pin_get(p0, 6);
  printk("Callback at %d %d\n", pins, v);
}

int main(void) {
  const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
  uint32_t dtr = 0;

  if (usb_enable(NULL)) {
    return 0;
  }

  if (!device_is_ready(p0)) {
    return 0;
  }

  if (!device_is_ready(p1)) {
    return 0;
  }

  if (gpio_pin_configure(p0, 15, GPIO_OUTPUT_ACTIVE) < 0) {
    return 0;
  }

  int p0_pins[] = {2, 6, 8, 9, 10, 11, 17, 20, 22, 24, 29, 31};
  int p1_pins[] = {0, 1, 2, 4, 6, 7, 11, 13, 15};

  for (int i = 0; i < 12; i++) {
    if (gpio_pin_configure(p0, p0_pins[i], GPIO_INPUT) < 0) {
      return 0;
    }
  }

  for (int i = 0; i < 9; i++) {
    if (gpio_pin_configure(p1, p1_pins[i], GPIO_INPUT) < 0) {
      return 0;
    }
  }

  if (gpio_pin_interrupt_configure(p0, 6, GPIO_INT_EDGE_BOTH) < 0) {
    return 0;
  }

  gpio_init_callback(&cb_data, callback, BIT(6));
  gpio_add_callback(p0, &cb_data);

  /* Poll if the DTR flag was set */
  while (!dtr) {
    uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
    /* Give CPU resources to low priority threads. */
    k_sleep(K_MSEC(100));
  }

  while (1) {
    printk("Hello World! %s\n", CONFIG_ARCH);
    if (gpio_pin_toggle(p0, 15) < 0) {
      return 0;
    }
    printk("p0: ");
    for (int i = 0; i < 12; i++) {
      int v = gpio_pin_get(p0, p0_pins[i]);
      printk("%d", v);
    }
    printk("\n");
    printk("p1: ");
    for (int i = 0; i < 9; i++) {
      int v = gpio_pin_get(p1, p1_pins[i]);
      printk("%d", v);
    }
    printk("\n");
    k_sleep(K_SECONDS(1));
  }
}
