void * initializePageTable(void);

void setPageTableEntry(struct pte *pte, u_long valid, u_long prot, u_long pfn);

void invalidatePageTableEntry(struct pte *pte);

// void deletePageTable(void);
