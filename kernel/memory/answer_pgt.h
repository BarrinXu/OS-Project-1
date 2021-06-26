void enable_paging() {
    // TODO: Homework 2: Enable paging
    // After initializing the page table, write to register SATP register for kernel registers.
    // Flush the TLB to invalidate the existing TLB Entries
    // Suggested: 2 LoCs
    w_satp((uint64) kernel_pagetable);
    flush_tlb();
}

// Return the address of the PTE in page table *pagetable*
// The Risc-v Sv48 scheme has four levels of page table.
// For VA:
//   47...63 zero
//   39...47 -- 9  bits of level-3 index
//   30...38 -- 9  bits of level-2 index
//   21...29 -- 9  bits of level-1 index
//   12...20 -- 9  bits of level-0 index
//   0...11  -- 12 bits of byte offset within the page
// Return the last-level page table entry.
static pte_t* pt_query(pagetable_t pagetable, vaddr_t va, int alloc){
    if(va >= MAXVA) BUG_FMT("get va[0x%lx] >= MAXVA[0x%lx]", va, MAXVA);
    // Suggested: 18 LoCs
    for(int i=3; i; i--)
    {
        pte_t *now_entry=&pagetable[PX(i,va)];
        if((*now_entry)&PTE_V)
            pagetable= (pagetable_t) ((*now_entry) & (~0xFFF));
        else
        {
            if(!alloc)
                return NULL;
            pagetable=mm_kalloc();
            if(pagetable==NULL)
                return NULL;
            memset(pagetable,0,PGSIZE);
            *now_entry=(uint64)pagetable|alloc;
        }
    }
    return &pagetable[PX(0,va)];
}
int pt_map_pages(pagetable_t pagetable, vaddr_t va, paddr_t pa, uint64 size, int perm){
    // Suggested: 11 LoCs
    //?
    for(vaddr_t st= PGROUNDDOWN(va),ed= PGROUNDDOWN(va+size-1);st<=ed; st+=PGSIZE){
        pte_t *now_entry= pt_query(pagetable,st,perm|PTE_V);
        if(now_entry==NULL)
            return -1;
        *now_entry= PA2PTE(pa)|perm|PTE_V;
        pa+=PGSIZE;
    }
    return 0; // Do not modify
}

paddr_t pt_query_address(pagetable_t pagetable, vaddr_t va){
    // Suggested: 3 LoCs
    pte_t *now_entry=pt_query(pagetable, va, 1);
    if(now_entry==NULL||(!((*now_entry)&PTE_V)))
        return NULL;
    return PTE2PA(*now_entry)|(va&0xFFF);
}

int pt_unmap_addrs(pagetable_t pagetable, vaddr_t va){
    // Suggested: 2 LoCs
    pte_t *now_entry= pt_query(pagetable,va,0);
    if(now_entry==NULL)
        BUG("pt_unmap_addrs: Error!");
    *now_entry=(*now_entry)&(~PTE_V);
    return 0; // Do not modify
}

int pt_map_addrs(pagetable_t pagetable, vaddr_t va, paddr_t pa, int perm){
    // Suggested: 2 LoCs
    pt_map_pages(pagetable,va,pa,1,perm);
    return 0; // Do not modify
}
