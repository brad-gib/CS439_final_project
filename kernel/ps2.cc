// #include "ps2.h"
// #include "stdint.h"
// #include "config.cc"
// #include "idt.h"
// #include "debug.h"
// #include "threads.h"
// #include "process.h"
// #include "machine.h"
// #include "ext2.h"
// #include "elf.h"
// #include "libk.h"
// #include "file.h"
// #include "heap.h"
// #include "shared.h"
// #include "kernel.h"

void PS2::init(void) {
    //determine if ps/2 controller exists
    RSD* rsdp = findRSD();
    uint32_t acpi_version = rsdp->Revision == 0 ? 1 : 2;
    char* name;
    SDT* rsdt = findSDT(rsdp, name);
}