// PTE:
	// pointer to a 4-byte buffer
	// bits:
	// 0: validity
	// 1-3: PROT_READ, PROT_WRITE, PROT_EXEC
	// 4-7: unused
	// 8-31: pfn
	
	
int validatePTE(PTE *ptep, int prot, int pfn) {
	// validity = 1, prot = prot, pfn = pfn
	// return ERROR if error else 0
}


int invalidatePTE(PTE *ptep) {
	// validity = 0 (we can ignore the rest)
	// return ERROR if error else 0
}
	