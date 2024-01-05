#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <sound/soc.h>
#include "softiic.h"
struct pcf8574_struct
{
    uint8_t base;
};

static int pcf8574_write_io(struct platform_device *device, uint8_t value)
{
	struct pcf8574_struct *pcf8574 = (struct pcf8574_struct *)platform_get_drvdata(device);
	struct device *parent = device->dev.parent;
	if (!parent)
	{
		dev_info(&device->dev, "Device is no mount to a bus.\n");
		return -ENXIO;
	}
	softiic_start(parent);
	softiic_send_byte(parent, pcf8574->base << 1);
	if (!softiic_read_ack(parent))
	{
		dev_info(&device->dev, "A nack was received.\n");
		return -EIO;
	}
	softiic_send_byte(parent, value);
	if (!softiic_read_ack(parent))
	{
		dev_info(&device->dev, "A nack was received.\n");
		return -EIO;
	}
	softiic_stop(parent);
	return 0;
}

static int pcf8574_parse_value(struct platform_device *device)
{
    int ret = 0;
    const uint8_t *value;
    value = of_get_property(device->dev.of_node, "value", NULL);
    dev_info(&device->dev, "Find value:0x%x.\n", *value);
    if (IS_ERR(value))
    {
        ret = pcf8574_write_io(device, 0xff);
    }
    else
    {
        ret = pcf8574_write_io(device, *value);
    }
    return 0;
}

static int pcf8574_platform_probe(struct platform_device *device) 
{
	int ret = 0;
    struct resource *res;
	struct pcf8574_struct *pcf8574 = devm_kzalloc(&device->dev, sizeof(struct pcf8574_struct), GFP_KERNEL);
	if (!pcf8574)
	{
		return -ENOMEM;
	}
	platform_set_drvdata(device, pcf8574);
    res = platform_get_resource(device, IORESOURCE_MEM, 0);
	pcf8574->base = (uint8_t)res->start;
    ret = pcf8574_parse_value(device);
    return ret;
}

static int pcf8574_platform_remove(struct platform_device *device)
{
    return 0;
}

static const struct of_device_id pcf8574_ids[] = {
	{ .compatible = "pcf8574" },
	{ }
};
MODULE_DEVICE_TABLE(of, pcf8574_ids);

static struct platform_driver pcf8574_driver = {
	.driver = {
		.name = "pcf8574",
		.of_match_table = pcf8574_ids,
        .owner = THIS_MODULE,
	},
	.probe = pcf8574_platform_probe,
	.remove = pcf8574_platform_remove,
};
module_platform_driver(pcf8574_driver);
MODULE_ALIAS("pcf8574 driver");
MODULE_AUTHOR("chenzhiyong<172471067@qq.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PCF8574 IO expander driver.");
