#ifndef __NVKM_PM_H__
#define __NVKM_PM_H__
#include <core/engine.h>

struct nvkm_perfdom;
struct nvkm_perfctr;
struct nvkm_pm {
	struct nvkm_engine engine;

	struct nvkm_perfctx *context;
	void *profile_data;

	struct list_head domains;
	struct list_head sources;
	u32 sequence;
};

static inline struct nvkm_pm *
nvkm_pm(void *obj)
{
	return (void *)nvkm_engine(obj, NVDEV_ENGINE_PM);
}

extern struct nvkm_oclass *nv40_pm_oclass;
extern struct nvkm_oclass *nv50_pm_oclass;
extern struct nvkm_oclass *g84_pm_oclass;
extern struct nvkm_oclass *gt200_pm_oclass;
extern struct nvkm_oclass *gt215_pm_oclass;
extern struct nvkm_oclass *gf100_pm_oclass;
extern struct nvkm_oclass *gf108_pm_oclass;
extern struct nvkm_oclass *gf117_pm_oclass;
extern struct nvkm_oclass *gk104_pm_oclass;
extern struct nvkm_oclass gk110_pm_oclass;
#endif
