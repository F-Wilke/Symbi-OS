Vol 3A 4-37

Pagefault eror codes:
31-16 Reserved
15    SGX   =0 The fault is not related to SGX. =1 The fault resulted from violation of SGX-specific access-control
requirements.
14-8  Reserved
7     HLAT  =0 The fault occurred during ordinary paging or due to access rights.=1 The fault occurred during HLAT paging.
6     SS    =0 The fault was not caused by a shadow-stack access.=1 The fault was caused by a shadow-stack access.
5     PK    =0 The fault was not caused by protection keys.=1 There was a protection-key violation.
4     I/D   =0 I/D 0 The fault was not caused by an instruction fetch. =1 The fault was caused by an instruction fetch.
3     RSVD  =0 The fault was not caused by reserved bit violation. =1 The fault was caused by a reserved bit set to 1 in some paging-structure entry.
2     U/S   =0 A supervisor-mode access caused the fault. =1 A user-mode access caused the fault.
1     W/R   =0 The access causing the fault was a read. =1 The access causing the fault was a write.
0     P     =0 The fault was caused by a non-present page. =1 The fault was caused by a page-level protection violation.


-  I/D flag (bit 4). This flag is 1 if (1) the access causing the page-fault exception was an instruction fetch; and (2) either
(a) CR4.SMEP = 1; or (b) both (i) CR4.PAE = 1 (either PAE paging, 4-level paging, or 5-level paging is in use);
and (ii) IA32_EFER.NXE = 1. Otherwise, the flag is 0. This flag describes the access causing the page-fault
exception, not the access rights specified by paging.

 For supervisor-mode accesses:
— Data may be read (implicitly or explicitly) from any supervisor-mode address with a protection key for
which read access is permitted (see Section 4.6.2).
— Data reads from user-mode pages.
Access rights depend on the value of CR4.SMAP:
• If CR4.SMAP = 0, data may be read from any user-mode address with a protection key for which read
access is permitted (see Section 4.6.2).
• If CR4.SMAP = 1, access rights depend on the value of EFLAGS.AC and whether the access is implicit or
explicit:
— If EFLAGS.AC = 1 and the access is explicit, data may be read from any user-mode address with a
protection key for which read access is permitted (see Section 4.6.2).
— If EFLAGS.AC = 0 or the access is implicit, data may not be read from any user-mode address.
— Data writes to supervisor-mode addresses.
Access rights depend on the value of CR0.WP:
• If CR0.WP = 0, data may be written to any supervisor-mode address with a protection key for which
write access is permitted (see Section 4.6.2).
• If CR0.WP = 1, data may be written to any supervisor-mode address with a translation for which the
R/W flag (bit 1) is 1 in every paging-structure entry controlling the translation and with a protection key
for which write access is permitted (see Section 4.6.2); data may not be written to any supervisor-
mode address with a translation for which the R/W flag is 0 in any paging-structure entry controlling the
translation.
— Data writes to user-mode addresses.
Access rights depend on the value of CR0.WP:
• If CR0.WP = 0, access rights depend on the value of CR4.SMAP:
— If CR4.SMAP = 0, data may be written to any user-mode address with a protection key for which
write access is permitted (see Section 4.6.2).
— If CR4.SMAP = 1, access rights depend on the value of EFLAGS.AC and whether the access is
implicit or explicit:
• If EFLAGS.AC = 1 and the access is explicit, data may be written to any user-mode address
with a protection key for which write access is permitted (see Section 4.6.2).
• If EFLAGS.AC = 0 or the access is implicit, data may not be written to any user-mode address.
• If CR0.WP = 1, access rights depend on the value of CR4.SMAP:
— If CR4.SMAP = 0, data may be written to any user-mode address with a translation for which the
R/W flag is 1 in every paging-structure entry controlling the translation and with a protection key
for which write access is permitted (see Section 4.6.2); data may not be written to any user-mode
address with a translation for which the R/W flag is 0 in any paging-structure entry controlling the
translation.
— If CR4.SMAP = 1, access rights depend on the value of EFLAGS.AC and whether the access is
implicit or explicit:
• If EFLAGS.AC = 1 and the access is explicit, data may be written to any user-mode address
with a translation for which the R/W flag is 1 in every paging-structure entry controlling the
translation and with a protection key for which write access is permitted (see Section 4.6.2);
data may not be written to any user-mode address with a translation for which the R/W flag is
0 in any paging-structure entry controlling the translation.
• If EFLAGS.AC = 0 or the access is implicit, data may not be written to any user-mode address.
— Instruction fetches from supervisor-mode addresses.
• For 32-bit paging or if IA32_EFER.NXE = 0, instructions may be fetched from any supervisor-mode
address.
• For other paging modes with IA32_EFER.NXE = 1, instructions may be fetched from any supervisor-
mode address with a translation for which the XD flag (bit 63) is 0 in every paging-structure entry
controlling the translation; instructions may not be fetched from any supervisor-mode address with a
translation for which the XD flag is 1 in any paging-structure entry controlling the translation.
61;8203;1c— Instruction fetches from user-mode addresses.
Access rights depend on the values of CR4.SMEP:
• If CR4.SMEP = 0, access rights depend on the paging mode and the value of IA32_EFER.NXE:
— For 32-bit paging or if IA32_EFER.NXE = 0, instructions may be fetched from any user-mode
address.
— For other paging modes with IA32_EFER.NXE = 1, instructions may be fetched from any user-
mode address with a translation for which the XD flag is 0 in every paging-structure entry
controlling the translation; instructions may not be fetched from any user-mode address with a
translation for which the XD flag is 1 in any paging-structure entry controlling the translation.
• If CR4.SMEP = 1, instructions may not be fetched from any user-mode address.
— Supervisor-mode shadow-stack accesses are allowed only to supervisor-mode shadow-stack addresses
(see above).



If CR4.SMEP(20) == 1 && EC.U/S(2)== 0 && EC.I/D(4) == 1 && EC.P(0) == 1 then we have possibly had an SMEP protection failure unset CR4.SMEP and restart access
If CR4.SMAP(21) == 1 && EC.U/S(2)== 0 && EC.I/D(4) == 0 && EC.P(0) == 1 then we have possibly had an SMAP protection failure unset CR4.SMEP and restart access

Once elevated then 
P  (0) == 1
U/S(2) == 0 
SMEP_SMAP_POSSIBLE_FAULT_EC_MASK ( 1<<0 | 1<<2 ) 
SMEP_SMAP_POSSIBLE_FAULT_EC_VAL ( 1<<0 | 0<<2 ) 

if (EC & SMEP_SMAP_POSSIBLE_FAULT_EC_MASK) == SMEP_SMAP_POSSIBLE_FAULT_EC_VAL) {
   CR4.SMEP(20) = 0; CR4.SMAP(21) =0;
   return from interrupt
   }



State of stack when we jump to ef_adaptor_asm_fix_ist_err_code:
      -32   rcx
      -24   rsi
   RSP-16   rdi
   RSP-8    original handler
   RSP+0  → Error Code 
   RSP+8  → RIP
   RSP+16 → CS
   RSP+24 → RFLAGS
   RSP+32 → RSP
   RSP+40 → SS
