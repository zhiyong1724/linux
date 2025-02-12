#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <sound/soc.h>
#include <drm/drm_panel.h>
#include <drm/drm_mode.h>
struct nt35510_struct
{
    struct drm_panel panel;
};

static int nt35510_enable(struct drm_panel *panel)
{
    pr_info("nt35510_enable\n");
    return 0;
}

static int nt35510_disable(struct drm_panel *panel)
{
    pr_info("nt35510_disable\n");
    return 0;
}

static int nt35510_prepare(struct drm_panel *panel)
{
    pr_info("nt35510_prepare\n");
    return 0;
}

static int nt35510_unprepare(struct drm_panel *panel)
{
    pr_info("nt35510_unprepare\n");
    return 0;
}

static int nt35510_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
    pr_info("nt35510_get_modes\n");
    return 0;
}

int nt35510_get_timings(struct drm_panel *panel, unsigned int num_timings, struct display_timing *timings)
{
    pr_info("nt35510_get_timings\n");
    return 0;
}


static const struct drm_panel_funcs nt35510_panel_funcs = 
{
    .enable = nt35510_enable,
    .disable = nt35510_disable,
    .prepare = nt35510_prepare,
    .unprepare = nt35510_unprepare,
    .get_modes = nt35510_get_modes,
    .get_timings = nt35510_get_timings,
};

static int nt35510_platform_probe(struct platform_device *device) 
{
	int ret = 0;
	struct nt35510_struct *nt35510 = devm_kzalloc(&device->dev, sizeof(struct nt35510_struct), GFP_KERNEL);
	if (!nt35510)
	{
		return -ENOMEM;
	}
	platform_set_drvdata(device, nt35510);
    drm_panel_init(&nt35510->panel, &device->dev, &nt35510_panel_funcs, DRM_MODE_CONNECTOR_DPI);
    drm_panel_add(&nt35510->panel);
    return ret;
}

static int nt35510_platform_remove(struct platform_device *device)
{
    return 0;
}

static const struct of_device_id nt35510_ids[] = {
	{ .compatible = "nt35510" },
	{ }
};
MODULE_DEVICE_TABLE(of, nt35510_ids);

static struct platform_driver nt35510_driver = {
	.driver = {
		.name = "nt35510",
		.of_match_table = nt35510_ids,
        .owner = THIS_MODULE,
	},
	.probe = nt35510_platform_probe,
	.remove = nt35510_platform_remove,
};
module_platform_driver(nt35510_driver);
MODULE_ALIAS("nt35510 driver");
MODULE_AUTHOR("chenzhiyong<172471067@qq.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("NT35510 lcd driver.");
