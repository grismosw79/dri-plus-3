.text
/* Function-entry replacements retain the Windows x64 call ABI. Each original
   trampoline replays whole, non-relative captured instructions exactly. */
.global HookEpsGrade
HookEpsGrade:
    jmp DriEpsGrade
.global NativeEpsGrade
NativeEpsGrade:
    .byte 0x48,0x89,0x5c,0x24,0x18,0x55,0x57,0x41,0x54,0x41,0x55,0x41,0x57,0x48,0x83,0xec,0x60
    jmp *ResumeEpsGrade(%rip)
.global HookEpsGeneric
HookEpsGeneric:
    jmp DriEpsGeneric
.global NativeEpsGeneric
NativeEpsGeneric:
    .byte 0x48,0x89,0x5c,0x24,0x18,0x55,0x56,0x41,0x56,0x48,0x83,0xec,0x70,0x49,0x63,0xf0
    jmp *ResumeEpsGeneric(%rip)
.global HookEpsRequired
HookEpsRequired:
    jmp DriEpsRequired
.global NativeEpsRequired
NativeEpsRequired:
    .byte 0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x55,0x41,0x57,0x48,0x8d,0x6c,0x24,0xc9
    jmp *ResumeEpsRequired(%rip)
.global HookEpsUpdate
HookEpsUpdate:
    jmp DriEpsUpdate
.global NativeEpsUpdate
NativeEpsUpdate:
    .byte 0x48,0x8b,0xc4,0x48,0x89,0x48,0x08,0x55,0x48,0x8d,0xa8,0x38,0xff,0xff,0xff
    jmp *ResumeEpsUpdate(%rip)
.macro SAVE_STATE frame=544
    pushfq
    push %rax
    push %rcx
    push %rdx
    push %rbx
    push %rbp
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    sub $\frame, %rsp
    fxsave64 32(%rsp)
    lea \frame(%rsp), %rcx
.endm
.macro RESTORE_STATE frame=544
    fxrstor64 32(%rsp)
    add $\frame, %rsp
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rbx
    pop %rdx
    pop %rcx
    pop %rax
    popfq
.endm
.global HookStart
HookStart:
    SAVE_STATE
    call DriOnStart
    RESTORE_STATE
    /* Exact non-relative instructions overwritten at RVA 0816F119. */
    .byte 0xc5,0xf9,0xef,0xc0,0x45,0x33,0xed,0xba,0xe0,0,0,0,0x48,0x8b,0xcb
    jmp *ResumeStart(%rip)
.global HookProtect
HookProtect:
    SAVE_STATE
    call DriOnProtect
    RESTORE_STATE
    /* RVA 0816F441; existing Battle eligibility code still runs afterward. */
    .byte 0x4d,0x8d,0x86,0xd0,0,0,0,0x49,0x8b,0xd4,0x48,0x8d,0x4d,0xa0
    jmp *ResumeProtect(%rip)
.global HookAdmit
HookAdmit:
    SAVE_STATE
    call DriOnAdmit
    test %eax,%eax
    jnz .Lskip_admission
    RESTORE_STATE
    /* RVA 0816F804, before the ordinary duplicate/admission checks. */
    .byte 0x48,0x8b,0x44,0x24,0x58,0x4c,0x8b,0xc3,0x49,0x8b,0xd4,0x48,0x8b,0x88,0xc8,0,0,0
    jmp *ResumeAdmit(%rip)
.Lskip_admission:
    RESTORE_STATE
    jmp *SkipAdmit(%rip)
.global HookStars
HookStars:
    SAVE_STATE
    call DriOnPortalStars
    test %eax,%eax
    jnz .Lstars_applied
    RESTORE_STATE
    cmp $1,%edi
    mov %r13,%rcx
    cmovle %eax,%r8d
    call *OriginalStarAssign(%rip)
    jmp *ResumeStars(%rip)
.Lstars_applied:
    RESTORE_STATE
    jmp *ResumeStars(%rip)
.global HookWeekly
HookWeekly:
    SAVE_STATE
    call DriOnPortalWeekly
    RESTORE_STATE
    test %al,%al
    jz .Lweekly_stop
    mov 0x40(%rsp),%rcx
    mov $1,%edx
    jmp *ResumeWeekly(%rip)
.Lweekly_stop:
    jmp *SkipWeekly(%rip)
.global HookInstant
HookInstant:
    SAVE_STATE
    call DriOnPortalInstant
    test %eax,%eax
    jnz .Linstant_stop
    RESTORE_STATE
    mov -0x39(%rbp),%rcx
    mov $9,%edx
    mov -0x41(%rbp),%r14
    call *NativeField(%rip)
    jmp *ResumeInstant(%rip)
.Linstant_stop:
    RESTORE_STATE
    jmp *SkipInstant(%rip)
.global HookSigning
HookSigning:
    SAVE_STATE
    call DriOnPortalSigning
    test %eax,%eax
    jnz .Lsigning_stop
    RESTORE_STATE
    mov 0x30(%rsp),%rcx
    call *NativeRecordKey(%rip)
    mov 0x10(%r14),%ecx
    jmp *ResumeSigning(%rip)
.Lsigning_stop:
    RESTORE_STATE
    jmp *SkipSigning(%rip)
.global HookDeadline
HookDeadline:
    SAVE_STATE
    call DriOnPortalDeadline
    cmp $-1,%eax
    je .Ldeadline_null
    test %eax,%eax
    jnz .Ldeadline_stop
    RESTORE_STATE
    mov (%r15),%rax
    mov %r15,%rcx
    mov %r15,0x70(%rsp)
    call *(%rax)
    mov 0x38(%rsp),%rcx
    jmp *ResumeDeadline(%rip)
.Ldeadline_null:
    RESTORE_STATE
    jmp *NullDeadline(%rip)
.Ldeadline_stop:
    RESTORE_STATE
    jmp *SkipDeadline(%rip)

.global HookPrepare
HookPrepare:
    SAVE_STATE
    call DriOnPortalPrepare
    RESTORE_STATE
    .byte 0x4d,0x8b,0xcf,0xc6,0x44,0x24,0x28,0x00,0x4c,0x8d,0x85,0x80,0x01,0x00,0x00,0xc6,0x44,0x24,0x20,0x00
    jmp *ResumePrepare(%rip)

.global HookScore
HookScore:
    SAVE_STATE
    call DriOnPortalScore
    RESTORE_STATE
    .byte 0x48,0x8b,0x9c,0x24,0x98,0x00,0x00,0x00,0x43,0x8d,0x04,0x34,0x41,0x89,0x47,0x28,0x45,0x89,0x67,0x2c
    jmp *ResumeScore(%rip)

.global HookQuota
HookQuota:
    SAVE_STATE
    call DriOnPortalQuota
    RESTORE_STATE
    /* Both native quota paths join here. Native max(ecx,edx) stays in place. */
    cmp %ecx,%r10d
    jl .Lquota_accept
    test %r10d,%r10d
    jle .Lquota_accept
    jmp *ResumeQuota(%rip)
.Lquota_accept:
    jmp *AcceptQuota(%rip)

.global HookSelect
HookSelect:
    SAVE_STATE
    call DriOnPortalSelect
    RESTORE_STATE
    test %al,%al
    jnz .Lselect_accept
    mov -0x39(%rbp),%rcx
    test %rcx,%rcx
    jmp *ResumeSelect(%rip)
.Lselect_accept:
    jmp *AcceptSelect(%rip)

.global HookFallbackStart
HookFallbackStart:
    SAVE_STATE
    call DriOnPortalFallbackStart
    test %eax,%eax
    jnz .Lfallback_selected
    RESTORE_STATE
    .byte 0x48,0x8b,0x89,0xc8,0x00,0x00,0x00,0x49,0x8b,0xd8,0x48,0x89,0x78,0xd8
    jmp *ResumeFallbackStart(%rip)
.Lfallback_selected:
    RESTORE_STATE
    /* Native 081a8ba0 has saved only RBP/RBX/R12 at this entry boundary. */
    add $0xf0,%rsp
    pop %r12
    pop %rbx
    pop %rbp
    ret

.global HookFallback
HookFallback:
    SAVE_STATE
    call DriOnPortalFallback
    test %eax,%eax
    jnz .Lfallback_skip
    RESTORE_STATE
    .byte 0x48,0x8b,0x45,0x97,0x48,0x8b,0x0c,0xf0,0x48,0x89,0x4d,0x6f,0x48,0x85,0xc9
    jmp *ResumeFallback(%rip)
.Lfallback_skip:
    RESTORE_STATE
    jmp *SkipFallback(%rip)

.global HookPortalRetain
HookPortalRetain:
    SAVE_STATE
    call DriOnPortalRetain
    RESTORE_STATE
    /* Native prefix-search back-edge targets this hook ENTRY at 0816f4e3.
       The three setup instructions at 0816f4d9 remain in their original place. */
    cmp %rax,%r15
    jae .Lportal_retain_no_prefix
    mov -0x30(%rbp),%rcx
    mov $10,%edx
    jmp *ResumePortalRetain(%rip)
.Lportal_retain_no_prefix:
    jmp *SkipPortalRetain(%rip)

.global HookPortalAllocate
HookPortalAllocate:
    SAVE_STATE
    call DriOnPortalAllocate
    RESTORE_STATE
    /* Normal, absent-board and absent-array paths join at 0816f7c3. */
    lea 0x98(%rbp),%rcx
    mov %r13,%rsi
    call *NativeBoardOptionalCheck(%rip)
    jmp *ResumePortalAllocate(%rip)

/* HS native condition has already succeeded. Observe accepted ownership; CPU veto follows native failed-condition cleanup. */
.global HookHsOwnership
HookHsOwnership:
    SAVE_STATE
    call DriOnHsOwnership
    testl %eax,%eax
    jnz .Lskip_hs_commit
    RESTORE_STATE
    .byte 0x48,0x8b,0x4c,0x24,0x40,0xba,0x01,0x00,0x00,0x00,0x48,0x8b,0x5c,0x24,0x38
    jmp *ResumeHsOwnership(%rip)
.Lskip_hs_commit:
    RESTORE_STATE
    jmp *SkipHsOwnership(%rip)

/* Shared native SoftCommitted/HardCommitted -> Signed join.
   Replay the relocated field call after restoring all native state. */
.global HookBulkOwnership
HookBulkOwnership:
    SAVE_STATE
    call DriOnBulkOwnership
    RESTORE_STATE
    mov -0x11(%rbp),%rcx
    mov $9,%edx
    mov -0x19(%rbp),%rdi
    call *NativeField(%rip)
    jmp *ResumeBulkOwnership(%rip)

.global HookBulkGuard
HookBulkGuard:
    SAVE_STATE
    call DriOnPortalBulkGuard
    test %eax,%eax
    jnz .Lbulk_guard_skip
    RESTORE_STATE
    mov -0x11(%rbp),%rcx
    mov $9,%edx
    mov -0x19(%rbp),%rbx
    call *NativeField(%rip)
    jmp *ResumeBulkGuard(%rip)
.Lbulk_guard_skip:
    RESTORE_STATE
    jmp *SkipBulkGuard(%rip)

.global HookCleanupProtection
HookCleanupProtection:
    SAVE_STATE
    call DriOnPortalCleanupProtection
    RESTORE_STATE
    test %al,%al
    jz .Lcleanup_allowed
    mov -0x11(%rbp),%rcx
    mov NativeCleanupWrapperVT(%rip),%rbx
    jmp *ResumeCleanupProtection(%rip)
.Lcleanup_allowed:
    jmp *AllowCleanupProtection(%rip)

.global HookPostWeekly
HookPostWeekly:
    SAVE_STATE
    call DriAfterPortalWeekly
    RESTORE_STATE
    .byte 0x4c,0x8b,0x6d,0x60,0x48,0x8d,0x55,0x68,0x4c,0x89,0x7d,0x68,0x49,0x8b,0x8d,0xa0,0,0,0
    jmp *ResumePostWeekly(%rip)

.global HookPostInstant
HookPostInstant:
    SAVE_STATE
    call DriAfterPortalInstant
    RESTORE_STATE
    test %r15,%r15
    jz .Lpost_instant_done
    mov (%r15),%rax
    mov %r15,%rcx
    call *8(%rax)
.Lpost_instant_done:
    jmp *ResumePostInstant(%rip)

.global HookPostNsd
HookPostNsd:
    SAVE_STATE
    call DriAfterPortalNsd
    RESTORE_STATE
    .byte 0x48,0x8d,0x4d,0xe8,0x41,0xc6,0x07,0,0x41,0xc6,0x85,0xa1,0x01,0,0,0
    jmp *ResumePostNsd(%rip)

/* Native allocation has finished; no board iterator is active here. */
.global HookPortalFunding
HookPortalFunding:
    SAVE_STATE
    call DriOnPortalFunding
    RESTORE_STATE
    xor %r14d,%r14d
    mov NativeCleanupWrapperVT(%rip),%r15
    test %r12,%r12
    jz .Lfunding_empty
    jmp *ResumeFunding(%rip)
.Lfunding_empty:
    jmp *SkipFunding(%rip)

/* All school/portal signing iterators have ended; publication has not begun. */
.global HookNsdResolve
HookNsdResolve:
    SAVE_STATE
    call DriBeforeNsdPublish
    RESTORE_STATE
    xor %r14d,%r14d
    mov %r14,0x48(%rsp)
    test %r12,%r12
    jz .Lnsd_empty
    jmp *ResumeNsdResolve(%rip)
.Lnsd_empty:
    jmp *SkipNsdResolve(%rip)

/* Test28: native PP scorer has filled each evidence row before these hooks. */
.global HookPPBeforeAge
HookPPBeforeAge:
    SAVE_STATE
    call DriPPBeforeAge
    test %eax,%eax
    jnz .Lpp_no_age
    RESTORE_STATE
    lea 0x6f(%rbp),%rdx
    mov %r13,%rcx
    call *NativePPAge(%rip)
    xor %r9d,%r9d
    jmp *ResumePPBeforeAge(%rip)
.Lpp_no_age:
    RESTORE_STATE
    lea 0x6f(%rbp),%rdx
    mov %r13,%rcx
    xor %r9d,%r9d
    jmp *ResumePPBeforeAge(%rip)
.global HookPPPublish
HookPPPublish:
    SAVE_STATE
    call DriPPPublish
    RESTORE_STATE
    mov %esi,%r8d
    lea -0x79(%rbp),%rcx
    mov %r12d,%edx
    call *NativePPSetGrade(%rip)
    jmp *ResumePPPublish(%rip)
.global HookStaff28Update
HookStaff28Update:
    jmp DriStaff28Update
.global NativeStaff28Update
NativeStaff28Update:
    .byte 0x48,0x8b,0xc4,0x55,0x57,0x48,0x8d,0x68,0xa1,0x48,0x81,0xec,0xa8,0,0,0
    jmp *ResumeStaff28Update(%rip)
.global HookPt28Grade
HookPt28Grade:
    jmp DriPt28Grade
.global NativePt28Grade
NativePt28Grade:
    .byte 0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x8d,0x6c,0x24,0xc0
    jmp *ResumePt28Grade(%rip)

/* Native CPU target costs also call the proximity getter directly. */
.global HookProximity28Grade
HookProximity28Grade:
    jmp DriProximity28Grade
.global NativeProximity28Grade
NativeProximity28Grade:
    .byte 0x40,0x55,0x53,0x56,0x57,0x41,0x55,0x41,0x56,0x48,0x8d,0x6c,0x24,0xd1
    jmp *ResumeProximity28Grade(%rip)

.global HookPPGroupGrade
HookPPGroupGrade:
    jmp DriPPGroupGrade
.global NativePPGroupGrade
NativePPGroupGrade:
    .byte 0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20
    jmp *ResumePPGroupGrade(%rip)

/* Test28R4: before any school's stage-4/5 publication list is collected. */
.global HookNsdCollect
HookNsdCollect:
    SAVE_STATE
    call DriBeforeNsdCollect
    RESTORE_STATE
    test %r12,%r12
    jz .Lnsd_collect_empty
    movzbl 0xc0(%rbp),%ebx
    jmp *ResumeNsdCollect(%rip)
.Lnsd_collect_empty:
    jmp *SkipNsdCollect(%rip)

/* Test28R8: both native interaction passes have ended. Observe executed
   target effort while evaluator/team/candidate references are still valid,
   before the evaluator clears its temporary budget state. Read-only callback. */
.global HookHsPostFunding
HookHsPostFunding:
    SAVE_STATE
    call DriAfterHsFunding
    RESTORE_STATE
    .byte 0x8b,0x9e,0x20,0x02,0x00,0x00,0x48,0x8b,0x8e,0xe0,0x00,0x00,0x00,0xff,0xc3
    jmp *ResumeHsPostFunding(%rip)

/* R9: ordinary HS collection has its own quota join, before portal collection. */
.global HookHsQuota
HookHsQuota:
    SAVE_STATE
    call DriOnHsQuota
    RESTORE_STATE
    cmp %ecx,%r10d
    jl .Lhsquota_accept
    test %r10d,%r10d
    jle .Lhsquota_accept
    jmp *ResumeHsQuota(%rip)
.Lhsquota_accept:
    jmp *AcceptHsQuota(%rip)

/* R13: preserve the complete original prologue for disabled/portal/owned paths. */
.global HookHsShortlist
HookHsShortlist:
    jmp DriHsShortlist
.global NativeHsShortlist
NativeHsShortlist:
    .byte 0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x6c,0x24,0x20,0x41,0x55,0x41,0x56,0x41,0x57
    jmp *ResumeHsShortlist(%rip)

.global HookVisit29Capacity
HookVisit29Capacity:
    SAVE_STATE
    call DriOnVisit29Capacity
    RESTORE_STATE
    .byte 0x4c,0x8d,0x5c,0x24,0x60,0x8b,0xc6,0x49,0x8b,0x5b,0x28,0x49,0x8b,0x6b,0x30
    jmp *ResumeVisit29Capacity(%rip)
.global HookVisit29Dates
HookVisit29Dates:
    SAVE_STATE
    call DriOnVisit29Dates
    RESTORE_STATE
    mov -0x11(%rbp),%rbx
    mov -9(%rbp),%r14
    cmp %r14,%rbx
    je .Lvisit29_no_dates
    jmp *ResumeVisit29Dates(%rip)
.Lvisit29_no_dates:
    jmp *SkipVisit29Dates(%rip)
.global HookVisit29Booked
HookVisit29Booked:
    mov %rbx,%rdx
    mov %r15,%rcx
    mov %eax,%r12d
    call *NativeVisit29Account(%rip)
    SAVE_STATE
    call DriOnVisit29Booked
    RESTORE_STATE
    jmp *ResumeVisit29Booked(%rip)

.global HookVisit29Eligible
HookVisit29Eligible:
    SAVE_STATE
    call DriOnVisit29Eligible
    RESTORE_STATE
    test %al,%al
    je .Lvisit29_ineligible
    mov 0xc8(%rbx),%rcx
    jmp *ResumeVisit29Eligible(%rip)
.Lvisit29_ineligible:
    jmp *SkipVisit29Eligible(%rip)
.global HookVisit29Completed
HookVisit29Completed:
    SAVE_STATE
    call DriOnVisit29Completed
    RESTORE_STATE
    movzbl %al,%edi
    test %al,%al
    je .Lvisit29_unapplied
    mov 0x80(%rbp),%r9d
    jmp *ResumeVisit29Completed(%rip)
.Lvisit29_unapplied:
    jmp *SkipVisit29Completed(%rip)

.global HookPortal29GenericAction
HookPortal29GenericAction:
    SAVE_STATE
    call DriAfterPortal29GenericAction
    RESTORE_STATE
    mov 0x7f(%rbp),%r9d
    lea -0x11(%rbp),%rax
    mov 0xc8(%r14),%rcx
    jmp *resume_portal29_generic_action(%rip)
.global HookPortal29Pitch
HookPortal29Pitch:
    SAVE_STATE
    call DriAfterPortal29Pitch
    RESTORE_STATE
    mov -0x7c(%rbp),%r9d
    lea -0x50(%rbp),%rax
    mov 0xc8(%rsi),%rcx
    jmp *resume_portal29_pitch(%rip)

.global HookAcademyReturnTarget
HookAcademyReturnTarget:
    SAVE_STATE
    call DriOnAcademy29ReturnTarget
    test %eax,%eax
    jnz .Lacademy_return_target_stop
    RESTORE_STATE
    .byte 0x48,0x8b,0x4c,0x24,0x50,0x48,0x8d,0x54,0x24,0x40,0x48,0x83,0xc1,0x08
    jmp *ResumeAcademyReturnTarget(%rip)
.Lacademy_return_target_stop:
    RESTORE_STATE
    jmp *SkipAcademyReturnTarget(%rip)

/* Test31 universal native action eligibility. Entry RSP is 8 mod 16;
   the 552-byte local frame aligns FXSAVE and the Windows callback ABI.
   Registers + 128 remains the original entry RSP (stack args +0x28/+0x30). */
.global HookOfferAction
HookOfferAction:
    SAVE_STATE 552
    call DriOnOfferAction
    test %eax,%eax
    jnz .Loffer_action_veto
    RESTORE_STATE 552
    .byte 0x48,0x89,0x5c,0x24,0x10,0x4c,0x89,0x4c,0x24,0x20,0x4c,0x89,0x44,0x24,0x18
    jmp *ResumeOfferAction(%rip)
.Loffer_action_veto:
    RESTORE_STATE 552
    xor %eax,%eax
    ret

/* Test31 initializer-only stage transition, after native condition passes. */
.global HookInitialHsStage
HookInitialHsStage:
    SAVE_STATE
    call DriOnInitialHsStage
    test %eax,%eax
    jnz .Linitial_hs_hold
    RESTORE_STATE
    .byte 0x48,0x8b,0x4d,0x0f,0xba,0x09,0x00,0x00,0x00,0x4c,0x8b,0x75,0x07,0x44,0x8b,0xf8
    jmp *ResumeInitialHsStage(%rip)
.Linitial_hs_hold:
    RESTORE_STATE
    jmp *SkipInitialHsStage(%rip)

/* Test31 earned outside-list HS opportunity, including original-false cases.
   Replay the captured condition call before inspecting its result. */
.global HookHsCondition
HookHsCondition:
    mov %rax,0x30(%rsp)
    lea 0x30(%rsp),%rcx
    call *NativeHsCondition(%rip)
    SAVE_STATE
    call DriAfterHsCondition
    RESTORE_STATE
    jmp *ResumeHsCondition(%rip)

/* Test33: first indexed scholarship pass has released its final candidate and
   exited; XMM6/7/8 were restored by native code. The physical productive pass
   has not initialized its index or loaded any candidate yet. Replan only at
   this guarded boundary, then replay all four stolen instructions exactly. */
.global HookHsBetweenFunding
HookHsBetweenFunding:
    SAVE_STATE
    call DriBetweenHsFunding
    RESTORE_STATE
    .byte 0x48,0x8b,0x7d,0xe0,0x33,0xc0,0x89,0x86,0x28,0x02,0x00,0x00,0x44,0x8b,0xe8
    jmp *ResumeHsBetweenFunding(%rip)
