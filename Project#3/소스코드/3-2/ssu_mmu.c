#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

unsigned int *page_table = NULL; // Global page table
unsigned int vpn_mask;
unsigned int offset_mask;
unsigned int shift;
unsigned int PTBR = 0; // Page Table Base Register

#define VALID_BIT 0
#define ACCESS_BIT 1
#define PFN_SHIFT 12

#define NOT_VALID -1
#define NOT_ACCESSIBLE -2
#define SUCCESS 0

void alloc_page_table(int address_space_bits, int page_bytes)
{
    // Calculate offset bits
    int offset_bits = (int)log2(page_bytes);
    int vpn_bits = address_space_bits - offset_bits;
    int num_entries = 1 << vpn_bits; // number of PTEs

    // Allocate memory for page table
    page_table = (unsigned int *)malloc(num_entries * sizeof(unsigned int));
    if(page_table == NULL) {
        fprintf(stderr, "Failed to allocate memory for page table.\n");
        exit(1);
    }
    // Initialize allocated memory to zero
    memset(page_table, 0, num_entries * sizeof(unsigned int));
}

void init_mmu_variables(int address_space_bits, int page_bytes)
{
    shift = (unsigned int)log2(page_bytes);
    offset_mask = (1 << shift) - 1;
    vpn_mask = ~offset_mask; // Since we're dealing with 32-bit addresses
}

void init_page_table(int address_space_bits, int page_bytes)
{
    int offset_bits = (int)log2(page_bytes);
    int vpn_bits = address_space_bits - offset_bits;
    int num_entries = 1 << vpn_bits; // Number of PTEs

    // Maximum PFN that can be represented in 20 bits
    unsigned int max_pfn = (1 << (32 - PFN_SHIFT)) - 1; // 0xFFFFF

    // For each VPN
    for(int vpn = 0; vpn < num_entries; vpn++) {
        unsigned int pte = 0;

        // Calculate PFN
        unsigned int pfn = vpn * 2;

        // Check if PFN exceeds the 20-bit limit
        if(pfn > max_pfn){
            // Mark PTE as invalid
            pte = 0x0;
        }
        else{
            // Set Valid bit
            pte |= 0x1; // Bit 0

            // Set PFN
            pte |= (pfn << PFN_SHIFT); // Bits 12-31

            // Set Access bit if vpn is not divisible by 4
            if(vpn % 4 != 0) {
                pte |= 0x2; // Bit 1
            }
        }

        // Assign PTE to page_table[vpn]
        page_table[vpn] = pte;
    }
}

int mmu_address_translation(unsigned int virtual_address, unsigned int *physical_address)
{
    // Extract VPN and offset
    unsigned int VPN = (virtual_address & vpn_mask) >> shift;
    unsigned int offset = virtual_address & offset_mask;

    // Fetch PTE
    unsigned int PTE = page_table[VPN];

    // Check if PTE.Valid == False
    if((PTE & 0x1) == 0) {
        // Invalid
        return NOT_VALID;
    }
    // Check if CanAccess(PTE.ProtectBits) == False
    if((PTE & 0x2) == 0) {
        // Not accessible
        return NOT_ACCESSIBLE;
    }
    // Access is OK
    unsigned int PFN = (PTE >> PFN_SHIFT) & ((1 << (32 - PFN_SHIFT)) - 1);
    // PhysAddr = (PFN << shift) | offset
    *physical_address = (PFN << shift) | offset;

    return SUCCESS;
}

int main(int argc, char *argv[])
{
    if(argc != 3) {
        fprintf(stderr, "Usage: %s [address_space_size_in_bits] [page_size_in_bytes]\n", argv[0]);
        exit(1);
    }

    int address_space_bits = atoi(argv[1]);
    int page_bytes = atoi(argv[2]);

    printf("SSU_MMU Simulator\n");

    // Allocate page table
    alloc_page_table(address_space_bits, page_bytes);

    // Initialize mmu variables
    init_mmu_variables(address_space_bits, page_bytes);

    // Initialize page table entries
    init_page_table(address_space_bits, page_bytes);

    char input[256];
    unsigned int virtual_address;
    unsigned int physical_address;
    while(1) {
        printf("Input a virtual address of hexadecimal value without \"0x\" (-1 to exit): ");
        scanf("%s", input);

        if(strcmp(input, "-1") == 0)
            break;

        virtual_address = (unsigned int)strtoul(input, NULL, 16);

        unsigned int VPN = (virtual_address & vpn_mask) >> shift;
        unsigned int offset = virtual_address & offset_mask;

        unsigned int PTE = page_table[VPN];

        unsigned int valid = PTE & 0x1;
        unsigned int access = (PTE >> 1) & 0x1;
        unsigned int PFN = (PTE >> PFN_SHIFT) & ((1 << (32 - PFN_SHIFT)) - 1);

        int result = mmu_address_translation(virtual_address, &physical_address);

        printf("Virtual address: 0x%x (vpn:%08x, pfn: %08x, valid: %d, access: %d) -> ", virtual_address, VPN, PFN, valid, access);

        if(result == SUCCESS) {
            printf("Physical address: 0x%x\n", physical_address);
        } else if(result == NOT_VALID) {
            printf("Segmentation Fault.\n");
        } else if(result == NOT_ACCESSIBLE) {
            printf("Protection Fault.\n");
        } else {
            printf("Unknown Error.\n");
        }
    }

    // Free allocated memory
    free(page_table);

    return 0;
}

