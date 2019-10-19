void * initializePageTable(void);

void setPageTableEntry(struct pte *pte, u_long valid, u_long prot, u_long pfn);

void invalidatePageTableEntry(struct pte *pte);

// struct pte * findPageTableEntry(u_long);

// bool chunkValidityCheck(void * base, void * limit, bool validity);

// void deletePageTable(void);
