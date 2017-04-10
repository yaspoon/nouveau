/*
 * Copyright 2013 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */
#define gf100_ram(p) container_of((p), struct gf100_ram, base)
#include "ram.h"
#include "ramfuc.h"

#include <core/option.h>
#include <subdev/bios.h>
#include <subdev/bios/pll.h>
#include <subdev/bios/rammap.h>
#include <subdev/bios/timing.h>
#include <subdev/clk.h>
#include <subdev/clk/pll.h>
#include <subdev/ltc.h>

struct gf100_ramfuc {
	struct ramfuc base;

	struct ramfuc_reg r_0x10fe20;
	struct ramfuc_reg r_0x10fe24;
	struct ramfuc_reg r_0x137320;
	struct ramfuc_reg r_0x137330;

	struct ramfuc_reg r_0x132000;
	struct ramfuc_reg r_0x132004;
	struct ramfuc_reg r_0x132100;

	struct ramfuc_reg r_0x137390;

	struct ramfuc_reg r_0x10f290;
	struct ramfuc_reg r_0x10f294;
	struct ramfuc_reg r_0x10f298;
	struct ramfuc_reg r_0x10f29c;
	struct ramfuc_reg r_0x10f2a0;

	struct ramfuc_reg r_0x10f300;
	struct ramfuc_reg r_0x10f338;
	struct ramfuc_reg r_0x10f340;
	struct ramfuc_reg r_0x10f344;
	struct ramfuc_reg r_0x10f348;

	struct ramfuc_reg r_0x10f910;
	struct ramfuc_reg r_0x10f914;

	struct ramfuc_reg r_0x100b0c;
	struct ramfuc_reg r_0x10f050;
	struct ramfuc_reg r_0x10f090;
	struct ramfuc_reg r_0x10f200;
	struct ramfuc_reg r_0x10f210;
	struct ramfuc_reg r_0x10f310;
	struct ramfuc_reg r_0x10f314;
	struct ramfuc_reg r_0x10f610;
	struct ramfuc_reg r_0x10f614;
	struct ramfuc_reg r_0x10f800;
	struct ramfuc_reg r_0x10f808;
	struct ramfuc_reg r_0x10f824;
	struct ramfuc_reg r_0x10f830;
	struct ramfuc_reg r_0x10f988;
	struct ramfuc_reg r_0x10f98c;
	struct ramfuc_reg r_0x10f990;
	struct ramfuc_reg r_0x10f998;
	struct ramfuc_reg r_0x10f9b0;
	struct ramfuc_reg r_0x10f9b4;
	struct ramfuc_reg r_0x10fb04;
	struct ramfuc_reg r_0x10fb08;
	struct ramfuc_reg r_0x137300;
	struct ramfuc_reg r_0x137310;
	struct ramfuc_reg r_0x137360;
	struct ramfuc_reg r_0x1373ec;
	struct ramfuc_reg r_0x1373f0;
	struct ramfuc_reg r_0x1373f8;

	struct ramfuc_reg r_0x61c140;
	struct ramfuc_reg r_0x611200;

	struct ramfuc_reg r_0x13d8f4;

	struct ramfuc_reg r_mr[16]; /* MR0 - MR7, MR15 */
};

struct gf100_ram {
	struct nvkm_ram base;
	struct gf100_ramfuc fuc;
	struct nvbios_pll refpll;
	struct nvbios_pll mempll;
};

static void
gf100_ram_train(struct gf100_ramfuc *fuc, u32 magic1, u32 magic2)
{
	struct gf100_ram *ram = container_of(fuc, typeof(*ram), fuc);
	struct nvkm_fb *fb = ram->base.fb;
	struct nvkm_device *device = fb->subdev.device;
	u32 part = nvkm_rd32(device, 0x022438), i;
	u32 mask = nvkm_rd32(device, 0x022554);
	u32 addr = 0x110974;

	ram_wr32(fuc, 0x10f910, magic1);
	ram_wr32(fuc, 0x10f914, magic1);
	if (magic2 != ~0)
		ram_wr32(fuc, 0x13d8f4, magic2);

	for (i = 0; (magic1 & 0x80000000) && i < part; addr += 0x1000, i++) {
		if (mask & (1 << i))
			continue;
		ram_wait(fuc, addr, 0x0000000f, 0x00000000, 500000);
	}
}

static int
gf100_ram_calc_xits(struct gf100_ram *ram,
		    struct nvkm_ram_data *prev,
		    struct nvkm_ram_data *next)
{
	struct nvbios_ramcfg *diff = &ram->base.diff;
	struct gf100_ramfuc *fuc = &ram->fuc;
	u32 mask, data;
	int ret, i;

	ret = ram_init(fuc, ram->base.fb);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(fuc->r_mr); i++) {
		if (ram_have(fuc, mr[i]))
			ram->base.mr[i] = ram_rd32(fuc, mr[i]);
	}

	switch (ram->base.type) {
	case NVKM_RAM_TYPE_GDDR5:
		ret = nvkm_gddr5_calc(&ram->base, false);
		if (ret)
			return ret;
		break;
	default:
		return -ENOSYS;
	}

	/* Prepare for script execution.
	 *
	 * NOTE: RM performs these operations from the host, instead of
	 * 	 from the script itself.
	 */
	ram_wr32(fuc, 0x132000, 0x18010002);
	ram_wr32(fuc, 0x132000, 0x18010000);

	/* Generate memory clock frequency change script. */

	// 0x00020039 // 0x0000039e
	ram_wr32(fuc, 0x132100, 0x00000001);
	ram_wr32(fuc, 0x10f988, 0x20010000);
	ram_wr32(fuc, 0x10f98c, 0x00000000);
	ram_wr32(fuc, 0x10f990, 0x20012001);
	ram_wr32(fuc, 0x10f998, 0x00010a00);
	// 0x00020039 // 0x0000039e
	ram_wr32(fuc, 0x100b0c, 0x00080012);

	ram_wait_vblank(fuc);
	ram_wr32(fuc, 0x611200, 0x00003300);
	// 0x00020034 // 0x0000000a	WRITE OUT TIMESTAMP
	ram_block(fuc);

	ram_wr32(fuc, 0x10f200, 0x008f8000);
	ram_wr32(fuc, 0x10f824, 0x000079f7);
	ram_wr32(fuc, 0x10f210, 0x00000000);
	ram_nsec(fuc, 1000);
	ram_wr32(fuc, 0x10f310, 0x00000001);
	ram_nsec(fuc, 1000);
	ram_wr32(fuc, 0x10f090, 0x00000061);
	ram_wr32(fuc, 0x10f090, 0xc000007f);
	ram_nsec(fuc, 1000);
	ram_wr32(fuc, 0x13d8f4, 0x00000000);
	// 0x0002003a // 0x00000001
	ram_wr32(fuc, 0x137300, 0x00000003);
	ram_wr32(fuc, 0x10f090, 0x4000007e);
	ram_nsec(fuc, 2000);
	ram_wr32(fuc, 0x10f314, 0x00000001);
	ram_wr32(fuc, 0x10f210, 0x80000000);
	ram_wr32(fuc, mr[0], ram->base.mr[0]);
	ram_nsec(fuc, 1000);

	ram_mask(fuc, 0x10f290, 0x7ffe07ff,
		      (next->bios.timing_10_RP  & 0x7f) << 24 |
		      (next->bios.timing_10_RAS & 0x7f) << 17 |
		      (next->bios.timing_10_RFC & 0x07) <<  8 |
		      (next->bios.timing_10_RC  & 0xff));

	data = (next->bios.timing_10_RCDWR & 0x3f) << 20 |
	       (next->bios.timing_10_RCDRD & 0x3f) << 14 |
	       (next->bios.timing_10_CL    & 0x7f);
	mask = 0x03ffc07f;
	if (next->bios.timing_10_CWL || diff->timing_10_CWL) {
		data |= (next->bios.timing_10_CWL & 0x7f) << 7;
		mask |= 0x00003f80;
	}
	ram_mask(fuc, 0x10f294, mask, data);

	ram_mask(fuc, 0x10f298, 0x007f7f00,
		      (next->bios.timing_10_WR  & 0x7f) << 16 |
		      (next->bios.timing_10_WTR & 0x7f) << 8);
	ram_mask(fuc, 0x10f29c, 0x0000001f,
		      (next->bios.timing_10_0d & 0x1f));
	ram_mask(fuc, 0x10f2a0, 0x001f8000,
		      (next->bios.timing_10_RRD & 0x3f) << 15);

	ram_mask(fuc, 0x10f808, 0xffffffff, 0x08020050);

	if (ram_mask(fuc, mr[1], 0x000f, ram->base.mr[1]),
	    ram_diff(fuc, mr[1], 0x000f))
		ram_nsec(fuc, 1000);

	ram_nuke(fuc, 0x10f830);
	ram_mask(fuc, 0x10f830, 0x00000000, 0x00000000);
	gf100_ram_train(fuc, 0x80021001, 0x00000000);
	// 0x0002003a // 0x00000002
	gf100_ram_train(fuc, 0x80081001, ~0);
	ram_mask(fuc, 0x10f830, 0x01000000, 0x01000000);
	ram_mask(fuc, 0x10f830, 0x01000000, 0x00000000);

	ram_unblock(fuc);
	// 0x00020034 // 0x0000000b	WRITE OUT TIMESTAMP

	ram_wr32(fuc, 0x13d8f4, 0x00000000);
	// 0x0002003a // 0x00000002
	ram_wr32(fuc, 0x100b0c, 0x00080028);
	ram_wr32(fuc, 0x611200, 0x00003330);
	ram_wr32(fuc, 0x10f200, 0x008f8800);

	/* Cleanup from script execution.
	 *
	 * NOTE: RM performs these operations from the host, instead of
	 * 	 from the script itself.
	 */
	ram_wr32(fuc, 0x10f824, 0x000079f7);
	ram_wr32(fuc, 0x10f808, 0x08020050);
	ram_wr32(fuc, 0x132000, 0x18010000);
	return 0;
}

int
gf100_ram_calc(struct nvkm_ram *base, u32 freq)
{
	struct gf100_ram *ram = gf100_ram(base);
	struct nvkm_clk *clk = ram->base.fb->subdev.device->clk;
	int ret;

	ret = nvkm_ram_data(&ram->base, nvkm_clk_read(clk, nv_clk_src_mem),
			    &ram->base.former);
	if (ret)
		return ret;

	ret = nvkm_ram_data(&ram->base, freq, &ram->base.target);
	if (ret)
		return ret;

	ram->base.next = &ram->base.target;

	return gf100_ram_calc_xits(ram, &ram->base.former, &ram->base.target);
}

int
gf100_ram_prog(struct nvkm_ram *base)
{
	struct gf100_ram *ram = gf100_ram(base);
	struct nvkm_device *device = ram->base.fb->subdev.device;
	ram_exec(&ram->fuc, nvkm_boolopt(device->cfgopt, "NvMemExec", true));
	return 0;
}

void
gf100_ram_tidy(struct nvkm_ram *base)
{
	struct gf100_ram *ram = gf100_ram(base);
	ram_exec(&ram->fuc, false);
}

void
gf100_ram_put(struct nvkm_ram *ram, struct nvkm_mem **pmem)
{
	struct nvkm_ltc *ltc = ram->fb->subdev.device->ltc;
	struct nvkm_mem *mem = *pmem;

	*pmem = NULL;
	if (unlikely(mem == NULL))
		return;

	mutex_lock(&ram->fb->subdev.mutex);
	if (mem->tag)
		nvkm_ltc_tags_free(ltc, &mem->tag);
	__nv50_ram_put(ram, mem);
	mutex_unlock(&ram->fb->subdev.mutex);

	kfree(mem);
}

int
gf100_ram_get(struct nvkm_ram *ram, u64 size, u32 align, u32 ncmin,
	      u32 memtype, struct nvkm_mem **pmem)
{
	struct nvkm_ltc *ltc = ram->fb->subdev.device->ltc;
	struct nvkm_mm *mm = &ram->vram;
	struct nvkm_mm_node **node, *r;
	struct nvkm_mem *mem;
	int type = (memtype & 0x0ff);
	int back = (memtype & 0x800);
	const bool comp = gf100_pte_storage_type_map[type] != type;
	int ret;

	size  >>= NVKM_RAM_MM_SHIFT;
	align >>= NVKM_RAM_MM_SHIFT;
	ncmin >>= NVKM_RAM_MM_SHIFT;
	if (!ncmin)
		ncmin = size;

	mem = kzalloc(sizeof(*mem), GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	mem->size = size;

	mutex_lock(&ram->fb->subdev.mutex);
	if (comp) {
		/* compression only works with lpages */
		if (align == (1 << (17 - NVKM_RAM_MM_SHIFT))) {
			int n = size >> 5;
			nvkm_ltc_tags_alloc(ltc, n, &mem->tag);
		}

		if (unlikely(!mem->tag))
			type = gf100_pte_storage_type_map[type];
	}
	mem->memtype = type;

	node = &mem->mem;
	do {
		if (back)
			ret = nvkm_mm_tail(mm, 0, 1, size, ncmin, align, &r);
		else
			ret = nvkm_mm_head(mm, 0, 1, size, ncmin, align, &r);
		if (ret) {
			mutex_unlock(&ram->fb->subdev.mutex);
			ram->func->put(ram, &mem);
			return ret;
		}

		*node = r;
		node = &r->next;
		size -= r->length;
	} while (size);
	mutex_unlock(&ram->fb->subdev.mutex);

	mem->offset = (u64)mem->mem->offset << NVKM_RAM_MM_SHIFT;
	*pmem = mem;
	return 0;
}

int
gf100_ram_init(struct nvkm_ram *base)
{
	static const u8  train0[] = {
		0x00, 0xff, 0x55, 0xaa, 0x33, 0xcc,
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00,
	};
	static const u32 train1[] = {
		0x00000000, 0xffffffff,
		0x55555555, 0xaaaaaaaa,
		0x33333333, 0xcccccccc,
		0xf0f0f0f0, 0x0f0f0f0f,
		0x00ff00ff, 0xff00ff00,
		0x0000ffff, 0xffff0000,
	};
	struct gf100_ram *ram = gf100_ram(base);
	struct nvkm_device *device = ram->base.fb->subdev.device;
	int i;

	switch (ram->base.type) {
	case NVKM_RAM_TYPE_GDDR5:
		break;
	default:
		return 0;
	}

	/* prepare for ddr link training, and load training patterns */
	for (i = 0; i < 0x30; i++) {
		nvkm_wr32(device, 0x10f968, 0x00000000 | (i << 8));
		nvkm_wr32(device, 0x10f96c, 0x00000000 | (i << 8));
		nvkm_wr32(device, 0x10f920, 0x00000100 | train0[i % 12]);
		nvkm_wr32(device, 0x10f924, 0x00000100 | train0[i % 12]);
		nvkm_wr32(device, 0x10f918,              train1[i % 12]);
		nvkm_wr32(device, 0x10f91c,              train1[i % 12]);
		nvkm_wr32(device, 0x10f920, 0x00000000 | train0[i % 12]);
		nvkm_wr32(device, 0x10f924, 0x00000000 | train0[i % 12]);
		nvkm_wr32(device, 0x10f918,              train1[i % 12]);
		nvkm_wr32(device, 0x10f91c,              train1[i % 12]);
	}

	return 0;
}

u32
gf100_ram_probe_fbpa_amount(struct nvkm_device *device, int fbpa)
{
	return nvkm_rd32(device, 0x11020c + (fbpa * 0x1000));
}

u32
gf100_ram_probe_fbp_amount(const struct nvkm_ram_func *func, u32 fbpao,
			   struct nvkm_device *device, int fbp, int *pltcs)
{
	if (!(fbpao & BIT(fbp))) {
		*pltcs = 1;
		return func->probe_fbpa_amount(device, fbp);
	}
	return 0;
}

u32
gf100_ram_probe_fbp(const struct nvkm_ram_func *func,
		    struct nvkm_device *device, int fbp, int *pltcs)
{
	u32 fbpao = nvkm_rd32(device, 0x022554);
	return func->probe_fbp_amount(func, fbpao, device, fbp, pltcs);
}

int
gf100_ram_ctor(const struct nvkm_ram_func *func, struct nvkm_fb *fb,
	       struct nvkm_ram *ram)
{
	struct nvkm_subdev *subdev = &fb->subdev;
	struct nvkm_device *device = subdev->device;
	struct nvkm_bios *bios = device->bios;
	const u32 rsvd_head = ( 256 * 1024); /* vga memory */
	const u32 rsvd_tail = (1024 * 1024); /* vbios etc */
	enum nvkm_ram_type type = nvkm_fb_bios_memtype(bios);
	u32 fbps = nvkm_rd32(device, 0x022438);
	u64 total = 0, lcomm = ~0, lower, ubase, usize;
	int ret, fbp, ltcs, ltcn = 0;

	nvkm_debug(subdev, "%d FBP(s)\n", fbps);
	for (fbp = 0; fbp < fbps; fbp++) {
		u32 size = func->probe_fbp(func, device, fbp, &ltcs);
		if (size) {
			nvkm_debug(subdev, "FBP %d: %4d MiB, %d LTC(s)\n",
				   fbp, size, ltcs);
			lcomm  = min(lcomm, (u64)(size / ltcs) << 20);
			total += size << 20;
			ltcn  += ltcs;
		} else {
			nvkm_debug(subdev, "FBP %d: disabled\n", fbp);
		}
	}

	lower = lcomm * ltcn;
	ubase = lcomm + func->upper;
	usize = total - lower;

	nvkm_debug(subdev, "Lower: %4lld MiB @ %010llx\n", lower >> 20, 0ULL);
	nvkm_debug(subdev, "Upper: %4lld MiB @ %010llx\n", usize >> 20, ubase);
	nvkm_debug(subdev, "Total: %4lld MiB\n", total >> 20);

	ret = nvkm_ram_ctor(func, fb, type, total, 0, ram);
	if (ret)
		return ret;

	nvkm_mm_fini(&ram->vram);

	/* Some GPUs are in what's known as a "mixed memory" configuration.
	 *
	 * This is either where some FBPs have more memory than the others,
	 * or where LTCs have been disabled on a FBP.
	 */
	if (lower != total) {
		/* The common memory amount is addressed normally. */
		ret = nvkm_mm_init(&ram->vram, rsvd_head >> NVKM_RAM_MM_SHIFT,
				   (lower - rsvd_head) >> NVKM_RAM_MM_SHIFT, 1);
		if (ret)
			return ret;

		/* And the rest is much higher in the physical address
		 * space, and may not be usable for certain operations.
		 */
		ret = nvkm_mm_init(&ram->vram, ubase >> NVKM_RAM_MM_SHIFT,
				   (usize - rsvd_tail) >> NVKM_RAM_MM_SHIFT, 1);
		if (ret)
			return ret;
	} else {
		/* GPUs without mixed-memory are a lot nicer... */
		ret = nvkm_mm_init(&ram->vram, rsvd_head >> NVKM_RAM_MM_SHIFT,
				   (total - rsvd_head - rsvd_tail) >>
				   NVKM_RAM_MM_SHIFT, 1);
		if (ret)
			return ret;
	}

	ram->ranks = (nvkm_rd32(device, 0x10f200) & 0x00000004) ? 2 : 1;
	return 0;
}

static int
gf100_ram_new_data(struct gf100_ram *ram, u8 ramcfg, int i)
{
	struct nvkm_device *device = ram->base.fb->subdev.device;
	struct nvkm_bios *bios = device->bios;
	struct nvkm_ram_data *cfg;
	struct nvbios_ramcfg *d = &ram->base.diff;
	struct nvbios_ramcfg *p, *n;
	u8  ver, hdr, cnt, len;
	u32 data;
	int ret;

	if (!(cfg = kmalloc(sizeof(*cfg), GFP_KERNEL)))
		return -ENOMEM;
	p = &list_last_entry(&ram->base.cfg, typeof(*cfg), head)->bios;
	n = &cfg->bios;

	/* memory config data for a range of target frequencies */
	data = nvbios_rammapEp(bios, i, &ver, &hdr, &cnt, &len, &cfg->bios);
	if (ret = -ENOENT, !data)
		goto done;
	if (ret = -ENOSYS, ver != 0x10 || hdr < 0x0e)
		goto done;

	/* ... and a portion specific to the attached memory */
	data = nvbios_rammapSp(bios, data, ver, hdr, cnt, len, ramcfg,
			       &ver, &hdr, &cfg->bios);
	if (ret = -EINVAL, !data)
		goto done;
	if (ret = -ENOSYS, ver != 0x10 || hdr < 0x0d)
		goto done;

	/* lookup memory timings, if bios says they're present */
	if (cfg->bios.ramcfg_timing != 0xff) {
		data = nvbios_timingEp(bios, cfg->bios.ramcfg_timing,
				       &ver, &hdr, &cnt, &len,
				       &cfg->bios);
		if (ret = -EINVAL, !data)
			goto done;
		if (ret = -ENOSYS, ver != 0x10 || hdr < 0x19)
			goto done;
	}

	list_add_tail(&cfg->head, &ram->base.cfg);
	if (ret = 0, i == 0)
		goto done;

	d->timing_10_CWL |= p->timing_10_CWL != n->timing_10_CWL;
done:
	if (ret)
		kfree(cfg);
	return ret;
}

int
gf100_ram_new_(const struct nvkm_ram_func *func,
	       struct nvkm_fb *fb, struct nvkm_ram **pram)
{
	struct nvkm_subdev *subdev = &fb->subdev;
	struct nvkm_bios *bios = subdev->device->bios;
	struct gf100_ram *ram;
	int ret, i;

	if (!(ram = kzalloc(sizeof(*ram), GFP_KERNEL)))
		return -ENOMEM;
	*pram = &ram->base;

	ret = gf100_ram_ctor(func, fb, &ram->base);
	if (ret)
		return ret;

	/* parse bios data for all rammap table entries up-front, and
	 * build information on whether certain fields differ between
	 * any of the entries.
	 *
	 * the binary driver appears to completely ignore some fields
	 * when all entries contain the same value.  at first, it was
	 * hoped that these were mere optimisations and the bios init
	 * tables had configured as per the values here, but there is
	 * evidence now to suggest that this isn't the case and we do
	 * need to treat this condition as a "don't touch" indicator.
	 */
	for (i = 0; !ret; i++) {
		ret = gf100_ram_new_data(ram, nvbios_ramcfg_index(subdev), i);
		if (ret && ret != -ENOENT) {
			nvkm_error(subdev, "failed to parse ramcfg data\n");
			return ret;
		}
	}

	ret = nvbios_pll_parse(bios, 0x0c, &ram->refpll);
	if (ret) {
		nvkm_error(subdev, "mclk refpll data not found\n");
		return ret;
	}

	ret = nvbios_pll_parse(bios, 0x04, &ram->mempll);
	if (ret) {
		nvkm_error(subdev, "mclk pll data not found\n");
		return ret;
	}

	ram->fuc.r_0x10fe20 = ramfuc_reg(0x10fe20);
	ram->fuc.r_0x10fe24 = ramfuc_reg(0x10fe24);
	ram->fuc.r_0x137320 = ramfuc_reg(0x137320);
	ram->fuc.r_0x137330 = ramfuc_reg(0x137330);

	ram->fuc.r_0x132000 = ramfuc_reg(0x132000);
	ram->fuc.r_0x132004 = ramfuc_reg(0x132004);
	ram->fuc.r_0x132100 = ramfuc_reg(0x132100);

	ram->fuc.r_0x137390 = ramfuc_reg(0x137390);

	ram->fuc.r_0x10f290 = ramfuc_reg(0x10f290);
	ram->fuc.r_0x10f294 = ramfuc_reg(0x10f294);
	ram->fuc.r_0x10f298 = ramfuc_reg(0x10f298);
	ram->fuc.r_0x10f29c = ramfuc_reg(0x10f29c);
	ram->fuc.r_0x10f2a0 = ramfuc_reg(0x10f2a0);

	ram->fuc.r_0x10f300 = ramfuc_reg(0x10f300);
	ram->fuc.r_0x10f338 = ramfuc_reg(0x10f338);
	ram->fuc.r_0x10f340 = ramfuc_reg(0x10f340);
	ram->fuc.r_0x10f344 = ramfuc_reg(0x10f344);
	ram->fuc.r_0x10f348 = ramfuc_reg(0x10f348);

	ram->fuc.r_0x10f910 = ramfuc_reg(0x10f910);
	ram->fuc.r_0x10f914 = ramfuc_reg(0x10f914);

	ram->fuc.r_0x100b0c = ramfuc_reg(0x100b0c);
	ram->fuc.r_0x10f050 = ramfuc_reg(0x10f050);
	ram->fuc.r_0x10f090 = ramfuc_reg(0x10f090);
	ram->fuc.r_0x10f200 = ramfuc_reg(0x10f200);
	ram->fuc.r_0x10f210 = ramfuc_reg(0x10f210);
	ram->fuc.r_0x10f310 = ramfuc_reg(0x10f310);
	ram->fuc.r_0x10f314 = ramfuc_reg(0x10f314);
	ram->fuc.r_0x10f610 = ramfuc_reg(0x10f610);
	ram->fuc.r_0x10f614 = ramfuc_reg(0x10f614);
	ram->fuc.r_0x10f800 = ramfuc_reg(0x10f800);
	ram->fuc.r_0x10f808 = ramfuc_reg(0x10f808);
	ram->fuc.r_0x10f824 = ramfuc_reg(0x10f824);
	ram->fuc.r_0x10f830 = ramfuc_reg(0x10f830);
	ram->fuc.r_0x10f988 = ramfuc_reg(0x10f988);
	ram->fuc.r_0x10f98c = ramfuc_reg(0x10f98c);
	ram->fuc.r_0x10f990 = ramfuc_reg(0x10f990);
	ram->fuc.r_0x10f998 = ramfuc_reg(0x10f998);
	ram->fuc.r_0x10f9b0 = ramfuc_reg(0x10f9b0);
	ram->fuc.r_0x10f9b4 = ramfuc_reg(0x10f9b4);
	ram->fuc.r_0x10fb04 = ramfuc_reg(0x10fb04);
	ram->fuc.r_0x10fb08 = ramfuc_reg(0x10fb08);
	ram->fuc.r_0x137300 = ramfuc_reg(0x137300);
	ram->fuc.r_0x137310 = ramfuc_reg(0x137310);
	ram->fuc.r_0x137360 = ramfuc_reg(0x137360);
	ram->fuc.r_0x1373ec = ramfuc_reg(0x1373ec);
	ram->fuc.r_0x1373f0 = ramfuc_reg(0x1373f0);
	ram->fuc.r_0x1373f8 = ramfuc_reg(0x1373f8);

	ram->fuc.r_0x61c140 = ramfuc_reg(0x61c140);
	ram->fuc.r_0x611200 = ramfuc_reg(0x611200);

	ram->fuc.r_0x13d8f4 = ramfuc_reg(0x13d8f4);

	switch (ram->base.type) {
	case NVKM_RAM_TYPE_GDDR5:
		ram->fuc.r_mr[0] = ramfuc_reg(0x10f300);
		ram->fuc.r_mr[1] = ramfuc_reg(0x10f330);
		ram->fuc.r_mr[2] = ramfuc_reg(0x10f334);
		ram->fuc.r_mr[3] = ramfuc_reg(0x10f338);
		ram->fuc.r_mr[4] = ramfuc_reg(0x10f33c);
		ram->fuc.r_mr[5] = ramfuc_reg(0x10f340);
		ram->fuc.r_mr[6] = ramfuc_reg(0x10f344);
		ram->fuc.r_mr[7] = ramfuc_reg(0x10f348);
		ram->fuc.r_mr[15] = ramfuc_reg(0x10f34c);
		break;
	default:
		break;
	}

	return 0;
}

static const struct nvkm_ram_func
gf100_ram = {
	.upper = 0x0200000000,
	.probe_fbp = gf100_ram_probe_fbp,
	.probe_fbp_amount = gf100_ram_probe_fbp_amount,
	.probe_fbpa_amount = gf100_ram_probe_fbpa_amount,
	.init = gf100_ram_init,
	.get = gf100_ram_get,
	.put = gf100_ram_put,
	.calc = gf100_ram_calc,
	.prog = gf100_ram_prog,
	.tidy = gf100_ram_tidy,
};

int
gf100_ram_new(struct nvkm_fb *fb, struct nvkm_ram **pram)
{
	return gf100_ram_new_(&gf100_ram, fb, pram);
}
