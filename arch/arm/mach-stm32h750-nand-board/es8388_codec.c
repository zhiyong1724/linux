#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/sched.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include "softiic.h"
#define ES8388_HP_LEFT_VOLUME_REG 0x2e
#define ES8388_HP_RIGHT_VOLUME_REG 0x2f
#define ES8388_SPK_LEFT_VOLUME_REG 0x30
#define ES8388_SPK_RIGHT_VOLUME_REG 0x31
struct es8388_codec_struct {
	struct platform_device *device;
	uint8_t base;
};

static const struct snd_kcontrol_new es8388_kcontrol[] = {
	SOC_DOUBLE_R("HP Volume", ES8388_HP_LEFT_VOLUME_REG,
		     ES8388_HP_RIGHT_VOLUME_REG, 0, 33, 0),
	SOC_DOUBLE_R("SPK Volume", ES8388_SPK_LEFT_VOLUME_REG,
		     ES8388_SPK_RIGHT_VOLUME_REG, 0, 33, 0),
};

static int es8388_write_reg(struct es8388_codec_struct *es8388_codec, uint8_t reg, uint8_t value)
{
	struct device *parent = es8388_codec->device->dev.parent;
	if (!parent) {
		dev_info(&es8388_codec->device->dev, "Device is no mount to a bus.\n");
		return -ENXIO;
	}
	softiic_start(parent);
	softiic_send_byte(parent, es8388_codec->base << 1);
	if (!softiic_read_ack(parent)) {
		dev_info(&es8388_codec->device->dev, "A nack was received.\n");
		return -EIO;
	}
	softiic_send_byte(parent, reg);
	if (!softiic_read_ack(parent)) {
		dev_info(&es8388_codec->device->dev, "A nack was received.\n");
		return -EIO;
	}
	softiic_send_byte(parent, value);
	softiic_stop(parent);
	return 0;
}

static int es8388_read_reg(struct es8388_codec_struct *es8388_codec, uint8_t reg, uint8_t *value)
{
	struct device *parent = es8388_codec->device->dev.parent;
	if (!parent)
	{
		dev_info(&es8388_codec->device->dev, "Device is no mount to a bus.\n");
		return -ENXIO;
	}
	softiic_start(parent);
	softiic_send_byte(parent, es8388_codec->base << 1);
	if (!softiic_read_ack(parent))
	{
		dev_info(&es8388_codec->device->dev, "A nack was received.\n");
		return -EIO;
	}
	softiic_send_byte(parent, reg);
	softiic_stop(parent);

	softiic_start(parent);
	softiic_send_byte(parent, (es8388_codec->base << 1) | 0x01);
	if (!softiic_read_ack(parent))
	{
		dev_info(&es8388_codec->device->dev, "A nack was received.\n");
		return -EIO;
	}
	*value = softiic_read_byte(parent);
	softiic_stop(parent);
	return 0;
}

static unsigned int es8388_reg_read(struct snd_soc_component *component,
				    unsigned int reg)
{
	int ret;
	uint8_t value;
	struct es8388_codec_struct *es8388_codec =
		(struct es8388_codec_struct *)dev_get_drvdata(component->dev);
	ret = es8388_read_reg(es8388_codec, (uint8_t)reg, &value);
	if (ret < 0)
		return ret;
	return value;
}

static int es8388_reg_write(struct snd_soc_component *component, unsigned int reg, unsigned int val)
{
	struct es8388_codec_struct *es8388_codec = (struct es8388_codec_struct *)dev_get_drvdata(component->dev);
	return es8388_write_reg(es8388_codec, (uint8_t)reg, (uint8_t)val);
}

static const struct snd_soc_component_driver es8388_component = {
	.name = "es8388-codec",
	.controls = es8388_kcontrol,
	.num_controls = ARRAY_SIZE(es8388_kcontrol),
	.read = es8388_reg_read,
	.write = es8388_reg_write,
};

static int es8388_startup(struct snd_pcm_substream *substream,
			  struct snd_soc_dai *dai)
{
	return 0;
}

static int es8388_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	return 0;
}

static int es8388_prepare(struct snd_pcm_substream *substream,
			  struct snd_soc_dai *dai)
{
	return 0;
}

static int es8388_trigger(struct snd_pcm_substream *substream, int event,
			  struct snd_soc_dai *dai)
{
	struct es8388_codec_struct *es8388_codec = (struct es8388_codec_struct *)dev_get_drvdata(dai->dev);
	switch (event)
	{
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		es8388_write_reg(es8388_codec, 0x04, 0x3c); //dac通道使能
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		es8388_write_reg(es8388_codec, 0x04, 0x00); //dac通道使能
		break;
	default:
		break;
	}
	return 0;
}

static int es8388_init(struct es8388_codec_struct *es8388_codec)
{
	//软复位ES8388
	es8388_write_reg(es8388_codec, 0x00, 0x80);
	es8388_write_reg(es8388_codec, 0x00, 0x00);
	msleep(100);

	es8388_write_reg(es8388_codec, 0x01, 0x58);
	es8388_write_reg(es8388_codec, 0x01, 0x50);
	es8388_write_reg(es8388_codec, 0x02, 0xF3);
	es8388_write_reg(es8388_codec, 0x02, 0xF0);

	es8388_write_reg(es8388_codec, 0x03, 0x09); //麦克风偏置电源关闭
	es8388_write_reg(es8388_codec, 0x00, 0x06); //使能参考		500K驱动使能
	es8388_write_reg(es8388_codec, 0x04, 0x00); //DAC电源管理，不打开任何通道
	es8388_write_reg(es8388_codec, 0x08, 0x00); //MCLK不分频
	es8388_write_reg(es8388_codec, 0x2B, 0x80); //DAC控制	DACLRC与ADCLRC相同

	es8388_write_reg(es8388_codec, 0x09, 0x88); //ADC L/R PGA增益配置为+24dB
	es8388_write_reg(es8388_codec, 0x0C, 0x4C); //ADC	数据选择为left data = left ADC, right data = left ADC 	音频数据为16bit
	es8388_write_reg(es8388_codec, 0x0D, 0x02); //ADC配置 MCLK/采样率=256
	es8388_write_reg(es8388_codec, 0x10, 0x00); //ADC数字音量控制将信号衰减 L	设置为最小！！！
	es8388_write_reg(es8388_codec, 0x11, 0x00); //ADC数字音量控制将信号衰减 R	设置为最小！！！

	es8388_write_reg(es8388_codec, 0x17, 0x18); //DAC 音频数据为16bit
	es8388_write_reg(es8388_codec, 0x18, 0x02); //DAC	配置 MCLK/采样率=256
	es8388_write_reg(es8388_codec, 0x1A, 0x00); //DAC数字音量控制将信号衰减 L	设置为最小！！！
	es8388_write_reg(es8388_codec, 0x1B, 0x00); //DAC数字音量控制将信号衰减 R	设置为最小！！！
	es8388_write_reg(es8388_codec, 0x27, 0xB8); //L混频器
	es8388_write_reg(es8388_codec, 0x2A, 0xB8); //R混频器
	es8388_write_reg(es8388_codec, 0x17, 0x18); //i2s模式，16bit
	es8388_write_reg(es8388_codec, 0x02, 0x0a); //dac使能

	es8388_write_reg(es8388_codec, ES8388_HP_LEFT_VOLUME_REG, 0x10);
	es8388_write_reg(es8388_codec, ES8388_HP_RIGHT_VOLUME_REG, 0x10);
	return 0;
}

static int es8388_probe(struct snd_soc_dai *dai)
{
	struct es8388_codec_struct *es8388_codec = (struct es8388_codec_struct *)dev_get_drvdata(dai->dev);
	es8388_init(es8388_codec);
	return 0;
}

static const struct snd_soc_dai_ops es8388_dai_ops = {
	.startup = es8388_startup,
	.hw_params = es8388_hw_params,
	.prepare = es8388_prepare,
	.trigger = es8388_trigger,
};

static struct snd_soc_dai_driver es8388_dai = {
		.probe = es8388_probe,
		.id = 1, /* avoid call to fmt_single_name() */
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rate_min = 8000,
			.rate_max = 192000,
			.rates = SNDRV_PCM_RATE_CONTINUOUS,
			.formats =
				SNDRV_PCM_FMTBIT_S8 |
				SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &es8388_dai_ops,
};

static int es8388_platform_probe(struct platform_device *device)
{
	int ret = 0;
	struct resource *res;
	struct es8388_codec_struct *es8388_codec = devm_kzalloc(
		&device->dev, sizeof(struct es8388_codec_struct), GFP_KERNEL);
	if (!es8388_codec) {
		return -ENOMEM;
	}
	platform_set_drvdata(device, es8388_codec);
	res = platform_get_resource(device, IORESOURCE_MEM, 0);
	es8388_codec->device = device;
	es8388_codec->base = (uint8_t)res->start;
	ret = snd_soc_register_component(&device->dev, &es8388_component,
					 &es8388_dai, 1);
	return ret;
}

static int es8388_platform_remove(struct platform_device *device)
{
	return 0;
}

static const struct of_device_id es8388_ids[] = { { .compatible =
							    "es8388-codec" },
						  {} };
MODULE_DEVICE_TABLE(of, es8388_ids);

static struct platform_driver es8388_driver = {
	.driver = {
		.name = "es8388-codec",
		.of_match_table = es8388_ids,
		.owner = THIS_MODULE,
	},
	.probe = es8388_platform_probe,
	.remove = es8388_platform_remove,
};
module_platform_driver(es8388_driver);
MODULE_ALIAS("es8388 codec driver");
MODULE_AUTHOR("chenzhiyong<172471067@qq.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ES8388 codec driver.");
