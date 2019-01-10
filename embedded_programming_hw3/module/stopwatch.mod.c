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
	{ 0x8a2e525e, "module_layout" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x91c309f1, "cdev_del" },
	{ 0x86cb7b28, "init_timer_key" },
	{ 0x3e69ffac, "cdev_add" },
	{ 0x1c94b3e2, "cdev_init" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x72542c85, "__wake_up" },
	{ 0xff178f6, "__aeabi_idivmod" },
	{ 0x2196324, "__aeabi_idiv" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x3a8ad4dc, "interruptible_sleep_on" },
	{ 0x45a55ec8, "__iounmap" },
	{ 0xf20dabd8, "free_irq" },
	{ 0xd6b8e852, "request_threaded_irq" },
	{ 0x65d6d0f0, "gpio_direction_input" },
	{ 0x40a6f522, "__arm_ioremap" },
	{ 0xf7b574c2, "del_timer_sync" },
	{ 0x6c8d5ae8, "__gpio_get_value" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x22d88e1d, "add_timer" },
	{ 0x37e74642, "get_jiffies_64" },
	{ 0x27e1a049, "printk" },
	{ 0x7a728ef4, "up" },
	{ 0xb5a0e267, "down" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

