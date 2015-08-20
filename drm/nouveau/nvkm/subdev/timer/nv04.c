/*
 * Copyright 2012 Red Hat Inc.
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
#include "nv04.h"

static u64
nv04_timer_read(struct nvkm_timer *tmr)
{
	u32 hi, lo;

	do {
		hi = nv_rd32(tmr, NV04_PTIMER_TIME_1);
		lo = nv_rd32(tmr, NV04_PTIMER_TIME_0);
	} while (hi != nv_rd32(tmr, NV04_PTIMER_TIME_1));

	return ((u64)hi << 32 | lo);
}

static void
nv04_timer_alarm_trigger(struct nvkm_timer *obj)
{
	struct nv04_timer *tmr = container_of(obj, typeof(*tmr), base);
	struct nvkm_alarm *alarm, *atemp;
	unsigned long flags;
	LIST_HEAD(exec);

	/* move any due alarms off the pending list */
	spin_lock_irqsave(&tmr->lock, flags);
	list_for_each_entry_safe(alarm, atemp, &tmr->alarms, head) {
		if (alarm->timestamp <= tmr->base.read(&tmr->base))
			list_move_tail(&alarm->head, &exec);
	}

	/* reschedule interrupt for next alarm time */
	if (!list_empty(&tmr->alarms)) {
		alarm = list_first_entry(&tmr->alarms, typeof(*alarm), head);
		nv_wr32(tmr, NV04_PTIMER_ALARM_0, alarm->timestamp);
		nv_wr32(tmr, NV04_PTIMER_INTR_EN_0, 0x00000001);
	} else {
		nv_wr32(tmr, NV04_PTIMER_INTR_EN_0, 0x00000000);
	}
	spin_unlock_irqrestore(&tmr->lock, flags);

	/* execute any pending alarm handlers */
	list_for_each_entry_safe(alarm, atemp, &exec, head) {
		list_del_init(&alarm->head);
		alarm->func(alarm);
	}
}

static void
nv04_timer_alarm(struct nvkm_timer *obj, u64 time, struct nvkm_alarm *alarm)
{
	struct nv04_timer *tmr = container_of(obj, typeof(*tmr), base);
	struct nvkm_alarm *list;
	unsigned long flags;

	alarm->timestamp = tmr->base.read(&tmr->base) + time;

	/* append new alarm to list, in soonest-alarm-first order */
	spin_lock_irqsave(&tmr->lock, flags);
	if (!time) {
		if (!list_empty(&alarm->head))
			list_del(&alarm->head);
	} else {
		list_for_each_entry(list, &tmr->alarms, head) {
			if (list->timestamp > alarm->timestamp)
				break;
		}
		list_add_tail(&alarm->head, &list->head);
	}
	spin_unlock_irqrestore(&tmr->lock, flags);

	/* process pending alarms */
	nv04_timer_alarm_trigger(&tmr->base);
}

static void
nv04_timer_alarm_cancel(struct nvkm_timer *obj, struct nvkm_alarm *alarm)
{
	struct nv04_timer *tmr = container_of(obj, typeof(*tmr), base);
	unsigned long flags;
	spin_lock_irqsave(&tmr->lock, flags);
	list_del_init(&alarm->head);
	spin_unlock_irqrestore(&tmr->lock, flags);
}

static void
nv04_timer_intr(struct nvkm_subdev *subdev)
{
	struct nv04_timer *tmr = (void *)subdev;
	u32 stat = nv_rd32(tmr, NV04_PTIMER_INTR_0);

	if (stat & 0x00000001) {
		nv04_timer_alarm_trigger(&tmr->base);
		nv_wr32(tmr, NV04_PTIMER_INTR_0, 0x00000001);
		stat &= ~0x00000001;
	}

	if (stat) {
		nv_error(tmr, "unknown stat 0x%08x\n", stat);
		nv_wr32(tmr, NV04_PTIMER_INTR_0, stat);
	}
}

int
nv04_timer_fini(struct nvkm_object *object, bool suspend)
{
	struct nv04_timer *tmr = (void *)object;
	if (suspend)
		tmr->suspend_time = nv04_timer_read(&tmr->base);
	nv_wr32(tmr, NV04_PTIMER_INTR_EN_0, 0x00000000);
	return nvkm_timer_fini(&tmr->base, suspend);
}

static int
nv04_timer_init(struct nvkm_object *object)
{
	struct nvkm_device *device = nv_device(object);
	struct nv04_timer *tmr = (void *)object;
	u32 m = 1, f, n, d, lo, hi;
	int ret;

	ret = nvkm_timer_init(&tmr->base);
	if (ret)
		return ret;

	/* aim for 31.25MHz, which gives us nanosecond timestamps */
	d = 1000000 / 32;

	/* determine base clock for timer source */
#if 0 /*XXX*/
	if (device->chipset < 0x40) {
		n = nvkm_hw_get_clock(device, PLL_CORE);
	} else
#endif
	if (device->chipset <= 0x40) {
		/*XXX: figure this out */
		f = -1;
		n = 0;
	} else {
		f = device->crystal;
		n = f;
		while (n < (d * 2)) {
			n += (n / m);
			m++;
		}

		nv_wr32(tmr, 0x009220, m - 1);
	}

	if (!n) {
		nv_warn(tmr, "unknown input clock freq\n");
		if (!nv_rd32(tmr, NV04_PTIMER_NUMERATOR) ||
		    !nv_rd32(tmr, NV04_PTIMER_DENOMINATOR)) {
			nv_wr32(tmr, NV04_PTIMER_NUMERATOR, 1);
			nv_wr32(tmr, NV04_PTIMER_DENOMINATOR, 1);
		}
		return 0;
	}

	/* reduce ratio to acceptable values */
	while (((n % 5) == 0) && ((d % 5) == 0)) {
		n /= 5;
		d /= 5;
	}

	while (((n % 2) == 0) && ((d % 2) == 0)) {
		n /= 2;
		d /= 2;
	}

	while (n > 0xffff || d > 0xffff) {
		n >>= 1;
		d >>= 1;
	}

	/* restore the time before suspend */
	lo = tmr->suspend_time;
	hi = (tmr->suspend_time >> 32);

	nv_debug(tmr, "input frequency : %dHz\n", f);
	nv_debug(tmr, "input multiplier: %d\n", m);
	nv_debug(tmr, "numerator       : 0x%08x\n", n);
	nv_debug(tmr, "denominator     : 0x%08x\n", d);
	nv_debug(tmr, "timer frequency : %dHz\n", (f * m) * d / n);
	nv_debug(tmr, "time low        : 0x%08x\n", lo);
	nv_debug(tmr, "time high       : 0x%08x\n", hi);

	nv_wr32(tmr, NV04_PTIMER_NUMERATOR, n);
	nv_wr32(tmr, NV04_PTIMER_DENOMINATOR, d);
	nv_wr32(tmr, NV04_PTIMER_INTR_0, 0xffffffff);
	nv_wr32(tmr, NV04_PTIMER_INTR_EN_0, 0x00000000);
	nv_wr32(tmr, NV04_PTIMER_TIME_1, hi);
	nv_wr32(tmr, NV04_PTIMER_TIME_0, lo);
	return 0;
}

void
nv04_timer_dtor(struct nvkm_object *object)
{
	struct nv04_timer *tmr = (void *)object;
	return nvkm_timer_destroy(&tmr->base);
}

int
nv04_timer_ctor(struct nvkm_object *parent, struct nvkm_object *engine,
		struct nvkm_oclass *oclass, void *data, u32 size,
		struct nvkm_object **pobject)
{
	struct nv04_timer *tmr;
	int ret;

	ret = nvkm_timer_create(parent, engine, oclass, &tmr);
	*pobject = nv_object(tmr);
	if (ret)
		return ret;

	tmr->base.subdev.intr = nv04_timer_intr;
	tmr->base.read = nv04_timer_read;
	tmr->base.alarm = nv04_timer_alarm;
	tmr->base.alarm_cancel = nv04_timer_alarm_cancel;
	tmr->suspend_time = 0;

	INIT_LIST_HEAD(&tmr->alarms);
	spin_lock_init(&tmr->lock);
	return 0;
}

struct nvkm_oclass
nv04_timer_oclass = {
	.handle = NV_SUBDEV(TIMER, 0x04),
	.ofuncs = &(struct nvkm_ofuncs) {
		.ctor = nv04_timer_ctor,
		.dtor = nv04_timer_dtor,
		.init = nv04_timer_init,
		.fini = nv04_timer_fini,
	}
};
