#ifndef _PageTable_h
#define _PageTable_h

void *initializeRegionPageTable(void);

void setPageTableEntry(struct pte *pte, u_long valid, u_long prot, u_long pfn);

void invalidatePageTableEntry(struct pte *pte);

#endif /* _PageTable_h */
