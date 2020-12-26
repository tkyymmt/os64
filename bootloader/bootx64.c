#include "efi.h"
#include "fb.h"


#define NULL				(void *)0

#define KERNEL_FILE_NAME	"kernel.bin"
#define KERNEL_START		0x0000000000110000
#define KERNEL_STACK_BASE	0x0000000000210000

#define MEM_DESC_SIZE		4800

#define SAFETY_READ_UNIT	16384	/* 16KB */

#define FILE_INFO_BUF_SIZE	180



struct EFI_GUID lip_guid = {0x5b1b31a1, 0x9562, 0x11d2,
			    {0x8e, 0x3f, 0x00, 0xa0,
			     0xc9, 0x69, 0x72, 0x3b}};
struct EFI_GUID dpp_guid = {0x09576e91, 0x6d3f, 0x11d2,
			    {0x8e, 0x39, 0x00, 0xa0,
			     0xc9, 0x69, 0x72, 0x3b}};
struct EFI_GUID fi_guid = {0x09576e92, 0x6d3f, 0x11d2,
			   {0x8e, 0x39, 0x00, 0xa0,
			    0xc9, 0x69, 0x72, 0x3b}};
struct EFI_GUID sfsp_guid = {0x0964e5b22, 0x6459, 0x11d2,
			     {0x8e, 0x39, 0x00, 0xa0,
			      0xc9, 0x69, 0x72, 0x3b}};


void efi_init(struct EFI_SYSTEM_TABLE *ST)
{
	struct EFI_GUID gop_guid = {0x9042a9de, 0x23dc, 0x4a38,
				    {0x96, 0xfb, 0x7a, 0xde,
				     0xd0, 0x80, 0x51, 0x6a}};
	struct EFI_GUID spp_guid = {0x31878c87, 0xb75, 0x11d5,
				    {0x9a, 0x4f, 0x0, 0x90,
				     0x27, 0x3f, 0xc1, 0x4d}};
	struct EFI_GUID stiep_guid = {0xdd9e7534, 0x7762, 0x4698,
				      {0x8c, 0x14, 0xf5, 0x85,
				       0x17, 0xa6, 0x25, 0xaa}};
	struct EFI_GUID dpttp_guid = {0x8b843e20, 0x8132, 0x4852,
				      {0x90, 0xcc, 0x55, 0x1a,
				       0x4e, 0x4a, 0x7f, 0x1c}};
	struct EFI_GUID dpftp_guid = {0x5c99a21, 0xc70f, 0x4ad2,
				      {0x8a, 0x5f, 0x35, 0xdf,
				       0x33, 0x43, 0xf5, 0x1e}};
	struct EFI_GUID dpup_guid = {0x379be4e, 0xd706, 0x437d,
				     {0xb0, 0x37, 0xed, 0xb8,
				      0x2f, 0xb7, 0x72, 0xa4}};
	struct EFI_GUID msp_guid = {0x3fdda605, 0xa76e, 0x4f46,
				    {0xad, 0x29, 0x12, 0xf4,
				     0x53, 0x1b, 0x3d, 0x08}};

	ST->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
	ST->BootServices->LocateProtocol(&gop_guid, NULL, (void **)&GOP);
	ST->BootServices->LocateProtocol(&spp_guid, NULL, (void **)&SPP);
	ST->BootServices->LocateProtocol(&stiep_guid, NULL, (void **)&STIEP);
	ST->BootServices->LocateProtocol(&dpttp_guid, NULL, (void **)&DPTTP);
	ST->BootServices->LocateProtocol(&dpftp_guid, NULL, (void **)&DPFTP);
	ST->BootServices->LocateProtocol(&dpup_guid, NULL, (void **)&DPUP);
	ST->BootServices->LocateProtocol(&msp_guid, NULL, (void **)&MSP);
	ST->BootServices->LocateProtocol(&sfsp_guid, NULL, (void **)&SFSP);
}

void safety_file_read(struct EFI_FILE_PROTOCOL *src, void *dst,
					  unsigned long long size)
{
	unsigned char *d = dst;
	while (size > SAFETY_READ_UNIT) {
		unsigned long long unit = SAFETY_READ_UNIT;
		src->Read(src, &unit, (void *)d);
		d += unit;
		size -= unit;
	}

	while (size > 0) {
		unsigned long long tmp_size = size;
		src->Read(src, &tmp_size, (void *)d);
		size -= tmp_size;
	}
}


struct EFI_SYSTEM_TABLE *ST;
void puts(unsigned short *s)
{
	ST->ConOut->OutputString(ST->ConOut, s);
}
#define MAX_STR_BUF	100
void puth(unsigned long long val, unsigned char num_digits)
{
	int i;
	unsigned short unicode_val;
	unsigned short str[MAX_STR_BUF];

	for (i = num_digits - 1; i >= 0; i--) {
		unicode_val = (unsigned short)(val & 0x0f);
		if (unicode_val < 0xa)
			str[i] = L'0' + unicode_val;
		else
			str[i] = L'A' + (unicode_val - 0xa);
		val >>= 4;
	}
	str[num_digits] = L'\0';

	puts(str);
}
void put_param(unsigned short *name, unsigned long long val)
{
	puts(name);
	puts(L": 0x");
	puth(val, 16);
	puts(L"\r\n");
}
unsigned char check_warn_error(unsigned long long status, unsigned short *message)
{
	if (status) {
		puts(message);
		puts(L":");
		puth(status, 16);
		puts(L"\r\n");
	}

	return !status;
}
void assert(unsigned long long status, unsigned short *message)
{
	if (!check_warn_error(status, message))
		while (1);
}
struct EFI_FILE_PROTOCOL *search_volume_contains_file(
	unsigned short *target_filename)
{
	void **sfs_handles;
	unsigned long long sfs_handles_num = 0;
	unsigned long long status;
	status = ST->BootServices->LocateHandleBuffer(
		ByProtocol, &sfsp_guid, NULL, &sfs_handles_num,
		(void ***)&sfs_handles);
	assert(status, L"LocateHandleBuffer");

	put_param(L"Number of volumes", sfs_handles_num);

	unsigned char i;
	struct EFI_FILE_PROTOCOL *fp;
	for (i = 0; i < sfs_handles_num; i++) {
		struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *tmp_sfsp;
		status = ST->BootServices->HandleProtocol(
			sfs_handles[i], &sfsp_guid, (void **)&tmp_sfsp);
		if (!check_warn_error(status, L"HandleProtocol(sfs_handles)"))
			continue;

		status = tmp_sfsp->OpenVolume(tmp_sfsp, &fp);
		if (!check_warn_error(status, L"OpenVolume(tmp_sfsp)"))
			continue;

		struct EFI_FILE_PROTOCOL *_target_fp;
		status = fp->Open(
			fp, &_target_fp, target_filename,
			EFI_FILE_MODE_READ, 0);
		if (!check_warn_error(
			    status, L"This volume don't have target file."))
			continue;

		return fp;
	}

	return NULL;
}
void load_kernel(struct EFI_SYSTEM_TABLE *ST)
{
	struct EFI_FILE_PROTOCOL *root = search_volume_contains_file(KERNEL_FILE_NAME);
	struct EFI_FILE_PROTOCOL *kernel_file = NULL;
	root->Open(root, &kernel_file, KERNEL_FILE_NAME, EFI_FILE_MODE_READ, 0);
	if (kernel_file == NULL)
		ST->ConOut->OutputString(ST->ConOut, L"----------------THEN-------------\n");
	else
		ST->ConOut->OutputString(ST->ConOut, L"-----------------ELSE--------------\n");
	while(1);

	/*

	struct header {
		void *bss_start;
		unsigned long long bss_size;
	} head;

	safety_file_read(kernel_file, (void *)&head, sizeof(head));

	kernel_size -= sizeof(head);
	safety_file_read(kernel_file, (void *)KERNEL_START, kernel_size);
	kernel_file->Close(kernel_file);

	ST->BootServices->SetMem(head.bss_start, head.bss_size, 0);
	*/
}


/* I don't know, but if you put mem_desc variable into exit_boot_services, 
 * you get error message saying "undefined reference to `___chkstk_ms'".
 * I guess it is too big to fit in the function as local variable.
 * */
unsigned char mem_desc[MEM_DESC_SIZE];

void exit_boot_services(void *IH, struct EFI_SYSTEM_TABLE *ST)
{
	unsigned long long mem_desc_unit_size;
	unsigned long long map_key;
    unsigned long long mmap_size;
    unsigned int desc_ver;

    mmap_size = MEM_DESC_SIZE;
    ST->BootServices->GetMemoryMap(&mmap_size,
            (struct EFI_MEMORY_DESCRIPTOR *)mem_desc,            &map_key, &mem_desc_unit_size, &desc_ver);
    
    ST->BootServices->ExitBootServices(IH, map_key);
}


struct framebuffer fb;
void init_fb()
{
	fb.base = GOP->Mode->FrameBufferBase;
	fb.size = GOP->Mode->FrameBufferSize;
	fb.hr = GOP->Mode->Info->HorizontalResolution;
	fb.vr = GOP->Mode->Info->VerticalResolution;
}


#define MAX_FILE_NAME_LEN  4
#define MAX_FILE_NUM       10
#define MAX_FILE_BUF       1024

struct FILE {
    unsigned short name[MAX_FILE_NAME_LEN];
};

extern struct FILE file_list[MAX_FILE_NUM];

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE  *SystemTable)
{
    void *IH = ImageHandle;
    ST = SystemTable;


	ST->ConOut->ClearScreen(ST->ConOut);


	ST->ConOut->OutputString(ST->ConOut, L"Initializing the bootloader\n");
    efi_init(ST);

	ST->ConOut->OutputString(ST->ConOut, L"Initializing the framebuffer\n");
	init_fb();



	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *kernel_file;
	unsigned long long status;
	unsigned long long buf_size = MAX_FILE_BUF;
 	unsigned char file_buf[MAX_FILE_BUF];

	SFSP->OpenVolume(SFSP, &root);

	status = root->Open(root, &kernel_file, "aaa", EFI_FILE_MODE_READ, 0);
	if (status) {
		ST->ConOut->OutputString(ST->ConOut, L"ERRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRor\n");
		check_warn_error(status, L"kernel_file error");
	}
/*
	struct header {
		void *bss_start;
		unsigned long long bss_size;
	} head;

	safety_file_read(kernel_file, (void *)&head, sizeof(head));

	kernel_size -= sizeof(head);
	safety_file_read(kernel_file, (void *)KERNEL_START, kernel_size);
	kernel_file->Close(kernel_file);

	ST->BootServices->SetMem(head.bss_start, head.bss_size, 0);

	unsigned long long fib_size = FILE_INFO_BUF_SIZE;
	unsigned long long fi_buf[FILE_INFO_BUF_SIZE];
	struct EFI_FILE_INFO *fi_ptr;

	kernel_file->GetInfo(file, &fi_guid, &fib_size, fi_buf);

	fi_ptr = (struct EFI_FILE_INFO *)fi_buf;
*/
	// dunno why, closing the kernel_file causes exception
	//kernel_file->Close(kernel_file);
	root->Close(root);
	while(1);




	ST->ConOut->OutputString(ST->ConOut, L"Loading the kernel\n");
    load_kernel(ST);

	ST->ConOut->OutputString(ST->ConOut, L"Exiting the bootloader\n");
    exit_boot_services(IH, ST);


	// jump to kernel code
	unsigned long long kernel_arg_fb = (unsigned long long)&fb;
	unsigned long long _sb = KERNEL_STACK_BASE;
	unsigned long long _ks = KERNEL_START;
	__asm__ (	"	mov	%0, %%rdx\n"
				"	mov	%1, %%rsp\n"
				"	jmp	*%2\n"
				::"m"(kernel_arg_fb), "m"(_sb), "m"(_ks));


    while(1);
}