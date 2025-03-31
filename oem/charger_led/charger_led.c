#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>

static unsigned int charger_led_gpio = 0;
static struct class *led_class;
static dev_t dev;
static struct device *devices;

static ssize_t led_power_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value = 0;
	value = gpio_get_value (charger_led_gpio);
	pr_info("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return sprintf(buf, "%d\n", value);
}

static ssize_t led_power_write(struct device *dev , struct device_attribute *attr , const char *buf, size_t len)
{

	pr_info("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	if (buf[0] == '0') {
		pr_info("%s %d write charger_led_gpio low\n", __FUNCTION__, __LINE__);
		gpio_direction_output(charger_led_gpio, 0);
	} else if (buf[0] == '1') {
		pr_info("%s %d write charger_led_gpio high\n", __FUNCTION__, __LINE__);
		gpio_direction_output(charger_led_gpio, 1);
	} else {
		pr_info("%s %d buf is other number ! ! !\n", __FUNCTION__, __LINE__);
	}
	return 1;
}

static DEVICE_ATTR(led_enable, 0644, led_power_read, led_power_write);


static int led_parse_dt(struct platform_device *pdev)
{
	int rc = 0;
	struct device_node *np = pdev->dev.of_node;

	charger_led_gpio = of_get_named_gpio(np, "charger_led_gpio", 0);
	if (!gpio_is_valid(charger_led_gpio)) {
		pr_err("%s : Unable to request charger_led_gpio\n", __func__);
		return -EFAULT;
	}

	pr_info("%s: get charger_led_gpio %d\n", __func__, charger_led_gpio);

	rc = gpio_request(charger_led_gpio, "charger_led_gpio");
	if (rc < 0) {
		pr_err("%s: request gpio failed: %d\n", __func__, rc);
		return -EFAULT;
	}
	return rc;
}

static struct attribute *led_class_attrs[] = {
	&dev_attr_led_enable.attr,
	NULL,
};

static const struct attribute_group led_group = {
	.attrs = led_class_attrs,
};


static const struct attribute_group *led_groups[] = {
	&led_group,
	NULL,
};

static int gpio_led_probe(struct platform_device *pdev)
{
	int rc = 0;
	int ret = 0;

	pr_info("gpio_led_probe probe\n");

	ret = alloc_chrdev_region(&dev, 0, 1, "charger_leds");
	pr_info(" MAJOR = %d MINOR = %d\r\n", MAJOR(dev), MINOR(dev));
	if (ret < 0) {
		pr_info("device registration failed!\r\n");
	}

	if (!pdev || !pdev->dev.of_node) {
		pr_err("%s:Unable to load device node\n", __func__);
		return -ENOTSUPP;
	}

	rc = led_parse_dt(pdev);
	if (rc) {
		pr_err("%s Unable to set property\n", __func__);
		return rc;
	}

	led_class = class_create(THIS_MODULE, "charger_leds");
	if (IS_ERR(led_class)) {
		pr_err(" Unable to create myleds class; errno = %ld\n",
		PTR_ERR(led_class));
		return PTR_ERR(led_class);
	}
	led_class->dev_groups = led_groups;

	devices = device_create(led_class, NULL, dev, NULL, "led_control");
	if (NULL == devices) {
		printk(" device_create error\r\n");
	}

	pr_info("%s: exit\n", __func__);
	return 0;
}

static int gpio_led_remove(struct platform_device *pdev)
{
	if (gpio_is_valid(charger_led_gpio)) {
		gpio_free(charger_led_gpio);
	}
	class_destroy(led_class);
	return 0;
}

static const struct of_device_id of_gpio_leds_match[] = {
	{ .compatible = "charger-leds", },
	{},
};

MODULE_DEVICE_TABLE(of, of_gpio_leds_match);

static struct platform_driver gpio_led_driver = {
	.probe		= gpio_led_probe,
	.remove		= gpio_led_remove,
	.driver		= {
		.name	= "charger-leds",
		.of_match_table = of_gpio_leds_match,
	},
};

module_platform_driver(gpio_led_driver);

MODULE_AUTHOR("zelin.pan");
MODULE_DESCRIPTION("GPIO LED driver");
MODULE_LICENSE("GPL");
