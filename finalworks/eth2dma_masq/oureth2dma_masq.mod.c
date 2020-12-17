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
	{ 0xadf42bd5, "__request_region" },
	{ 0x788fe103, "iomem_resource" },
	{ 0xdb38ca4a, "arm_dma_ops" },
	{ 0x10785a91, "malloc_sizes" },
	{ 0x46608fa0, "getnstimeofday" },
	{ 0x6430013d, "dev_get_by_name" },
	{ 0x2d87a93d, "nf_register_hook" },
	{ 0x3e460849, "__pskb_pull_tail" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x27e1a049, "printk" },
	{ 0x71c90087, "memcmp" },
	{ 0xaa0a3862, "init_net" },
	{ 0xfa1d060f, "kmem_cache_alloc" },
	{ 0xc2165d85, "__arm_iounmap" },
	{ 0x9bce482f, "__release_region" },
	{ 0x39c4f73d, "nf_unregister_hook" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0x40a6f522, "__arm_ioremap" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x7c8a0afd, "skb_copy_bits" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

