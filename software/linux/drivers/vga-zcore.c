// SPDX-License-Identifier: GPL-2.0-only
/* Write one 320x200 RGB332 frame to /dev/zcore-vga. */
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/of.h>

#define FRAME_SIZE (320 * 200)
static void __iomem *base;
static u32 frame[FRAME_SIZE / 4];
static DEFINE_MUTEX(frame_lock);
static ssize_t write_frame(struct file *f, const char __user *buf, size_t n, loff_t *pos)
{
	int i;
	if (n != FRAME_SIZE) return -EINVAL;
	if (mutex_lock_interruptible(&frame_lock)) return -ERESTARTSYS;
	if (copy_from_user(frame, buf, n)) {
		mutex_unlock(&frame_lock);
		return -EFAULT;
	}
	writel(0, base);
	for (i = 0; i < FRAME_SIZE / 4; i++) writel(frame[i], base + 12);
	mutex_unlock(&frame_lock);
	return n;
}
static const struct file_operations ops = {
	.owner = THIS_MODULE, .write = write_frame, .open = nonseekable_open,
};
static struct miscdevice dev = {
	.minor = MISC_DYNAMIC_MINOR, .name = "zcore-vga", .fops = &ops, .mode = 0600,
};
static int probe(struct platform_device *p)
{
	base = devm_platform_ioremap_resource(p, 0);
	if (IS_ERR(base)) return PTR_ERR(base);
	return misc_register(&dev);
}
static const struct of_device_id ids[] = { { .compatible = "zcore,vga" }, {} };
static struct platform_driver zcore_vga_driver = {
	.probe = probe, .driver = { .name = "zcore-vga", .of_match_table = ids },
};
builtin_platform_driver(zcore_vga_driver);
