#ifndef __NVKM_VMM_H__
#define __NVKM_VMM_H__
#include "priv.h"

struct nvkm_vmm_page {
	u8 shift;
#define NVKM_VMM_PAGE_COMP                                                 0x01
	u8 type;
};

struct nvkm_vmm_func {
	void *(*dtor)(struct nvkm_vmm *);

	const struct nvkm_vmm_page *(*page)(struct nvkm_vmm *);
	u64 page_block;
};

int nvkm_vmm_ctor(const struct nvkm_vmm_func *, struct nvkm_mmu *,
		  int vma_bits, u64 addr, u64 size, struct lock_class_key *,
		  struct nvkm_vmm *vmm);
void *nvkm_vmm_dtor(struct nvkm_vmm *);

struct nvkm_vmm_user {
	int (*ctor)(struct nvkm_mmu *, u64 addr, u64 size, void *args, u32 argc,
		    struct lock_class_key *, struct nvkm_vmm **);
};
#endif
