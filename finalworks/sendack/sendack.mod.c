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
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xaa0a3862, "init_net" },
	{ 0x27e1a049, "printk" },
	{ 0x692087d7, "dev_queue_xmit" },
	{ 0xe113bbbc, "csum_partial" },
	{ 0x1c34f6ad, "skb_push" },
	{ 0xebc1f623, "skb_put" },
	{ 0xc38579a6, "__alloc_skb" },
	{ 0x6430013d, "dev_get_by_name" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

