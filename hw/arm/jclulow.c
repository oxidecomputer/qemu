/*
 * XXX
 * ARM Cortex-M3 Shenanigans
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/arm/boot.h"
#include "sysemu/runstate.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "hw/arm/armv7m.h"
#include "hw/qdev-properties.h"
#include "hw/irq.h"


typedef struct {
	MachineState parent;
} jclulow_t;

static void
jclulow_io_write(void *arg, hwaddr offset, uint64_t value, unsigned size)
{
	if (offset == 0 && size == 4) {
		fprintf(stderr, "%c", (uint8_t)value);
		fflush(stderr);
	} else {
		fprintf(stderr,
		    "JCLULOW IO: write (%u) offset 0x%x\n",
		        size, (unsigned)offset);
	}
}

static uint64_t
jclulow_io_read(void *arg, hwaddr offset, unsigned size)
{
	fprintf(stderr,
	    "JCLULOW IO: read at offset 0x%x\n", (unsigned)offset);

	return (0);
}

static const MemoryRegionOps jclulow_io_ops = {
	.read =		jclulow_io_read,
	.write =	jclulow_io_write,
	.endianness =	DEVICE_NATIVE_ENDIAN,
};

#define	JCLULOW_MACHINE(o)					\
	OBJECT_CHECK(jclulow_t, o, TYPE_JCLULOW_MACHINE)

#if 0
/*
 * XXX?
 */
static void
do_sys_reset(void *arg, int n, int level)
{
	if (level) {
		qemu_system_reset_request(SHUTDOWN_CAUSE_GUEST_RESET);
	}
}
#endif

static void
jclulow_init(MachineState *ms)
{
	// jclulow_t *j = JCLULOW_MACHINE(ms);

	MemoryRegion *mem = get_system_memory();

	/*
	 * Anchor ROM, size 512KB, at 0x00000000
	 */
	size_t rom_size = 512 * 1024;
	MemoryRegion *rom = g_new(MemoryRegion, 1);
	memory_region_init_ram(rom, NULL, "jclulow.rom", rom_size,
	    &error_fatal);
	memory_region_set_readonly(rom, true);
	memory_region_add_subregion(mem, 0x00000000, rom);

	/*
	 * Anchor RAM, size 1024KB, at 0x20000000
	 */
	size_t ram_size = 1024 * 1024;
	MemoryRegion *ram = g_new(MemoryRegion, 1);
	memory_region_init_ram(ram, NULL, "jclulow.ram", ram_size,
	    &error_fatal);
	memory_region_add_subregion(mem, 0x20000000, ram);

	/*
	 * Create ARM Cortex-M3 MCU
	 */
	DeviceState *arm = qdev_create(NULL, TYPE_ARMV7M);
#define	NUM_IRQ_LINES	16
	qdev_prop_set_uint32(arm, "num-irq", NUM_IRQ_LINES);
	qdev_prop_set_string(arm, "cpu-type", ms->cpu_type);
	qdev_prop_set_bit(arm, "enable-bitband", true); /* XXX? */
	object_property_set_link(OBJECT(arm), OBJECT(mem), "memory",
	    &error_abort);

	qdev_init_nofail(arm);

#if 0
	qdev_connect_gpio_out_named(arm, "SYSRESETREQ", 0,
	    qemu_allocate_irq(&do_sys_reset, NULL, 0)); /* XXX? */
#endif

	/*
	 * XXX?
	 */
	MemoryRegion *io = g_new(MemoryRegion, 1);
	memory_region_init_io(io, NULL, &jclulow_io_ops, NULL, "jclulow.io",
	    0x00001000);
	memory_region_add_subregion(mem, 0x40000000, io);

	/*
	 * Load kernel from ELF file (etc)...
	 */
	armv7m_load_kernel(ARM_CPU(first_cpu), ms->kernel_filename,
	    rom_size);
}

static void
jclulow_class_init(ObjectClass *oc, void *data)
{
	MachineClass *mc = MACHINE_CLASS(oc);

	mc->desc = "jclulow shenigan board";
	mc->init = jclulow_init;
	mc->max_cpus = 1;
	mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-m3");
}

static const TypeInfo jclulow_type = {
	.name = MACHINE_TYPE_NAME("jclulow"),
	.parent = TYPE_MACHINE,
	.instance_size = sizeof (jclulow_t),
	.class_init = jclulow_class_init,
};

static void jclulow_machine_init(void)
{
	type_register_static(&jclulow_type);
}

type_init(jclulow_machine_init)
