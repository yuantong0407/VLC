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
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x39c4f73d, "nf_unregister_hook" },
	{ 0xaa0a3862, "init_net" },
	{ 0x2d87a93d, "nf_register_hook" },
	{ 0x6430013d, "dev_get_by_name" },
	{ 0x27e1a049, "printk" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x2a18c74, "nf_conntrack_destroy" },
	{ 0xe113bbbc, "csum_partial" },
	{ 0x604d7920, "skb_make_writable" },
	{ 0x3e460849, "__pskb_pull_tail" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

