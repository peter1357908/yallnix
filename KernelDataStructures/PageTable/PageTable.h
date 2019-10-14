// PAGE_TABLE_SIZE (TBD)

// PageTable
    // array of PTEs (of a fixed number)

int initializePageTable() {
	// pagetable = malloc(sizeof(PTE *) * PAGE_TABLE_SIZE)
	// for each PTE *ptep: malloc(4);
	// return ERROR if error else 0
}


bool chunkValidityCheck(void * base, void * limit, bool validity) {
	// check every PTE from base to limit
	// return FALSE if any PTE's validity != validity
	// otherwise return TRUE
}


void deletePageTable() {
	// for each PTE *ptep: free(ptep);
	// free(pagetable)
}
