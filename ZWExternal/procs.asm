.code

ZwReadVirtualMemory proc
	mov r10, rcx
	mov eax, 3Fh
	syscall
	ret
ZwReadVirtualMemory endp

ZwWriteVirtualMemory proc
	mov r10, rcx
	mov eax, 3Ah
	syscall
	ret
ZwWriteVirtualMemory endp

ZwOpenProcess proc
	mov r10, rcx
	mov eax, 26h
	syscall
	ret
ZwOpenProcess endp

ZwProtectVirtualMemory proc
	mov r10, rcx
	mov eax, 50h
	syscall
	ret
ZwProtectVirtualMemory endp

end