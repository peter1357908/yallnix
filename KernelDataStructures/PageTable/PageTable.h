#ifndef _PageTable_h
#define _PageTable_h

int initializeRegionPageTable(struct pte **pageTablep);


void setPageTableEntry(struct pte *pte, u_long valid, u_long prot, u_long pfn);


void setPageTableEntryNoFlush(struct pte *pte, u_long valid, u_long prot, u_long pfn);


void invalidatePageTableEntry(struct pte *pte);

/*  returns 1 for region 1; returns 0 for region 0;
    returns -1 if neither
*/
int getAddressRegion(void *addr);
 
/*  returns SUCCESS/ERROR; looks up addr in pagetable
    and assigns address's prot bit to prot
*/
int getAddressProt(void *addr, int region, u_long *prot);

#endif /* _PageTable_h */
