#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xcf79a31, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xadf42bd5, "__request_region" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x788fe103, "iomem_resource" },
	{ 0xdb38ca4a, "arm_dma_ops" },
	{ 0x46608fa0, "getnstimeofday" },
	{ 0x196aa8d0, "dev_get_by_name" },
	{ 0x1f87bfc, "__register_chrdev" },
	{ 0xd0d7f169, "platform_device_register_full" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x27e1a049, "printk" },
	{ 0x71c90087, "memcmp" },
	{ 0x7f4358dd, "skb_push" },
	{ 0x57dcb3e3, "platform_device_unregister" },
	{ 0x2072ee9b, "request_threaded_irq" },
	{ 0x82a5eea7, "init_net" },
	{ 0xbc477a2, "irq_set_irq_type" },
	{ 0xfdb3aca8, "__alloc_skb" },
	{ 0xc2165d85, "__arm_iounmap" },
	{ 0x9bce482f, "__release_region" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0x40a6f522, "__arm_ioremap" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xe113bbbc, "csum_partial" },
	{ 0x62b74781, "dev_queue_xmit" },
	{ 0xb7b76e6, "skb_put" },
	{ 0xf20dabd8, "free_irq" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

