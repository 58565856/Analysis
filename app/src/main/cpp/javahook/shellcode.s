
.global _dlopen_addr_s       @dlopen������Ŀ������еĵ�ַ     ע:����ȫ�ֱ仯��C�пɶ�д
.global _dlopen_param1_s     @dlopen����1<.so>��Ŀ������еĵ�ַ 
.global _dlopen_param2_s     @dlopen����2��Ŀ������еĵ�ַ
 
.global _dlsym_addr_s        @dlsym������Ŀ������еĵ�ַ
.global _dlsym_param2_s      @dlsym����2��Ŀ������еĵ�ַ,��ʵΪ������
 
.global _dlclose_addr_s      @dlcose��Ŀ������еĵ�ַ
 
.global _inject_start_s      @������ε���ʼ��ַ
.global _inject_end_s        @������εĽ�����ַ
 
.global _inject_function_param_s  @hook_init������Ŀ������еĵ�ַ
 
.global _saved_cpsr_s        @����CPSR,�Ա�ִ����hook_init֮��ָ�����
.global _saved_r0_pc_s       @����r0-r15,�Ա�ִ����hook_init֮��ָ�����

.set size_of_string, _dlopen_param2_s - _dlopen_param1_s


.data
_inject_start_s:
	@debug loop
3:
	@sub r1, r1, #0
	@B 3b
    mov  r0, #1                       @ STDOUT
    ldr  r1, _dlopen_param1_s         @ memory address of string
    movs r2, #20         @ size of string
    mov  r7, #4                       @ write syscall
    swi  #0                           @ invoke syscall

	@ dlopen
	ldr r1, _dlopen_param2_s        @����dlopen�ڶ�������,flag
	ldr r0, _dlopen_param1_s        @����dlopen��һ������ .so
	ldr r3, _dlopen_addr_s          @����dlopen����
	blx r3                          @ִ��dlopen����,����ֵλ��r0��
	subs r4, r0, #0                 @��dlopen�ķ���ֵsoinfo������r4�У��Է������dlcloseʹ��
	beq	2f
 
	@dlsym
	ldr r1, _dlsym_param2_s        @����dlsym�ڶ�������,��һ�������Ѿ���r0����
	ldr r3, _dlsym_addr_s          @����dlsym����
	blx r3                         @ִ��dlsym����,����ֵλ��r0��
	subs r3, r0, #0                @�ѷ���ֵ<hook_init��Ŀ������еĵ�ַ>������r3��
	beq 1f
 
	@call our function
	ldr r0, _inject_function_param_s  @����hook_init��һ������
    blx r3                            @ִ��hook_init
	subs r0, r0, #0
	beq 2f
 
1:
	@dlclose                        
	mov r0, r4                        @��dlopen�ķ���ֵ��Ϊdlcose�ĵ�һ������
	ldr r3, _dlclose_addr_s           @����dlclose����
	blx r3                            @ִ��dlclose����
 
2:
	@restore context
	ldr r1, _saved_cpsr_s             @�ָ�CPSR
	msr cpsr_cf, r1
	ldr sp, _saved_r0_pc_s            @�ָ��Ĵ���r0-r15
	ldmfd sp, {r0-pc}
	
 
    
 
_dlopen_addr_s:                           @��ʼ��_dlopen_addr_s
.word 0x11111111
 
_dlopen_param1_s:
.word 0x11111111
 
_dlopen_param2_s:
.word 0x2                                 @RTLD_GLOBAL
 
_dlsym_addr_s:
.word 0x11111111
 
_dlsym_param2_s:
.word 0x11111111
 
_dlclose_addr_s:
.word 0x11111111
 
_inject_function_param_s:
.word 0x11111111
 
_saved_cpsr_s:
.word 0x11111111
 
_saved_r0_pc_s:
.word 0x11111111
_inject_end_s:                     @���������ַ
 
.space 0x400, 0                    @����οռ��С
 
.end
