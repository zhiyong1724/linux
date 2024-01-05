#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/of_platform.h>
#include "softiic.h"
struct softiic_struct
{
    long clock;
    long high_ns;
    long low_ns;
    long scl_delay_ns;
    long sda_delay_ns;
    struct gpio_desc *scl;
    struct gpio_desc *sda;
};

static int parse_dt(struct platform_device *device)
{
    int ret = 0;
    u32 clock;
    struct softiic_struct *softiic = (struct softiic_struct *)platform_get_drvdata(device);
    ret = of_property_read_u32(device->dev.of_node, "clock-frequency", &clock);
    if (ret < 0)
    {
        clock = 100000;
    }
    softiic->clock = clock;
    return ret;
}

static int calculateTime(struct platform_device *device)
{
    struct softiic_struct *softiic = (struct softiic_struct *)platform_get_drvdata(device);
    softiic->high_ns = 1000000000 / softiic->clock / 2;
    softiic->low_ns = softiic->high_ns;
    softiic->sda_delay_ns = softiic->low_ns / 5;
    softiic->low_ns -= softiic->sda_delay_ns;
    softiic->scl_delay_ns = softiic->sda_delay_ns;
    return 0;
}

static int softiic_init(struct platform_device *device)
{
    struct softiic_struct *softiic = (struct softiic_struct *)platform_get_drvdata(device);
    gpiod_set_config(softiic->scl, 5);
    gpiod_direction_output(softiic->scl, 1);
    gpiod_set_config(softiic->sda, 5);
    gpiod_direction_output(softiic->sda, 1);
    return 0;
}

int softiic_start(struct device *device)
{
    struct softiic_struct *softiic = (struct softiic_struct *)dev_get_drvdata(device);
    gpiod_direction_output(softiic->scl, 1);
    gpiod_direction_output(softiic->sda, 0);
    ndelay(softiic->scl_delay_ns);
    gpiod_direction_output(softiic->scl, 0);
    ndelay(softiic->low_ns);
    return 0;
}
EXPORT_SYMBOL_GPL(softiic_start);

int softiic_stop(struct device *device)
{
    struct softiic_struct *softiic = (struct softiic_struct *)dev_get_drvdata(device);
    gpiod_direction_output(softiic->scl, 0);
    ndelay(softiic->sda_delay_ns);
    gpiod_direction_output(softiic->sda, 0);
    ndelay(softiic->low_ns);
    gpiod_direction_output(softiic->scl, 1);
    ndelay(softiic->sda_delay_ns);
    gpiod_direction_output(softiic->sda, 1);
    ndelay(softiic->high_ns);
    return 0;
}
EXPORT_SYMBOL_GPL(softiic_stop);

int softiic_send_byte(struct device *device, uint8_t data)
{
    int i = 0;
    struct softiic_struct *softiic = (struct softiic_struct *)dev_get_drvdata(device);
    for (i = 0; i < 8; i++)
    {
        gpiod_direction_output(softiic->scl, 0);
        ndelay(softiic->sda_delay_ns);
        gpiod_direction_output(softiic->sda, (data & 0x80) >> 7);
        ndelay(softiic->low_ns);
        gpiod_direction_output(softiic->scl, 1);
        ndelay(softiic->high_ns);
        data <<= 1;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(softiic_send_byte);

uint8_t softiic_read_byte(struct device *device)
{
    int i = 0;
    uint8_t data = 0;
    struct softiic_struct *softiic = (struct softiic_struct *)dev_get_drvdata(device);
    for (i = 0; i < 8; i++) 
    {
	    gpiod_direction_output(softiic->scl, 0);
	    gpiod_direction_input(softiic->sda);
	    ndelay(softiic->low_ns);
	    gpiod_direction_output(softiic->scl, 1);
	    ndelay(softiic->high_ns);
	    data <<= 1;
	    if (gpiod_get_value(softiic->sda) > 0)
		    data++;
    }
    return data;
}
EXPORT_SYMBOL_GPL(softiic_read_byte);

int softiic_read_ack(struct device *device)
{
    struct softiic_struct *softiic = (struct softiic_struct *)dev_get_drvdata(device);
    gpiod_direction_output(softiic->scl, 0);
    gpiod_direction_input(softiic->sda);
    ndelay(softiic->low_ns);
    gpiod_direction_output(softiic->scl, 1);
    ndelay(softiic->high_ns);
    return !gpiod_get_value(softiic->sda);
}
EXPORT_SYMBOL_GPL(softiic_read_ack);

int softiic_send_nack(struct device *device)
{
    struct softiic_struct *softiic = (struct softiic_struct *)dev_get_drvdata(device);
	gpiod_direction_output(softiic->scl, 0);
	ndelay(softiic->sda_delay_ns);
	gpiod_direction_output(softiic->sda, 1);
	ndelay(softiic->low_ns);
	gpiod_direction_output(softiic->scl, 1);
	ndelay(softiic->high_ns);
    return 0;
}
EXPORT_SYMBOL_GPL(softiic_send_nack);

static int softiic_probe(struct platform_device *device)
{
    struct softiic_struct *softiic = devm_kzalloc(&device->dev, sizeof(struct softiic_struct), GFP_KERNEL);
    if (!softiic)
	    return -ENOMEM;
    platform_set_drvdata(device, softiic);
    parse_dt(device);
    calculateTime(device);
    softiic->scl = gpiod_get(&device->dev, "scl", 0);
    if (IS_ERR(softiic->scl))
    {
        dev_info(&device->dev, "Can't find SCL pin.\n");
        return -ENOENT;
    }
    softiic->sda = gpiod_get(&device->dev, "sda", 0);
    if (IS_ERR(softiic->scl))
    {
        gpiod_put(softiic->scl);
        dev_info(&device->dev, "Can't find SDA pin.\n");
        return -ENOENT;
    }
    softiic_init(device);
    return devm_of_platform_populate(&device->dev);
}

static int softiic_remove(struct platform_device *device)
{
    struct softiic_struct *softiic = (struct softiic_struct *)platform_get_drvdata(device);
    gpiod_put(softiic->scl);
    gpiod_put(softiic->sda);
    return 0;
}

static const struct of_device_id softiic_ids[] = {
	{ .compatible = "softiic" },
	{ }
};
MODULE_DEVICE_TABLE(of, softiic_ids);

static struct platform_driver softiic_driver = {
	.driver = {
		.name = "softiic",
		.of_match_table = softiic_ids,
        .owner = THIS_MODULE,
	},
	.probe = softiic_probe,
	.remove = softiic_remove,
};
module_platform_driver(softiic_driver);
MODULE_ALIAS("softiic driver");
MODULE_AUTHOR("chenzhiyong<172471067@qq.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Softiic driver.");
