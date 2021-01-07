#include "efi.h"
#include "fb.h"


#define NULL				(void *)0

#define KERNEL_FILE_NAME	L"kernel.bin"
#define KERNEL_START		0x0000000000110000
#define KERNEL_STACK_BASE	0x0000000000210000


#define SAFETY_READ_UNIT	16384	/* 16KB */

#define FILE_INFO_BUF_SIZE	180



void efi_init(struct EFI_SYSTEM_TABLE *ST)
{
	struct EFI_GUID gop_guid = {0x9042a9de, 0x23dc, 0x4a38,
				    {0x96, 0xfb, 0x7a, 0xde,
				     0xd0, 0x80, 0x51, 0x6a}};
	struct EFI_GUID msp_guid = {0x3fdda605, 0xa76e, 0x4f46,
				    {0xad, 0x29, 0x12, 0xf4,
				     0x53, 0x1b, 0x3d, 0x08}};

	struct EFI_GUID sfsp_guid = {0x0964e5b22, 0x6459, 0x11d2,
					{0x8e, 0x39, 0x00, 0xa0,
					 0xc9, 0x69, 0x72, 0x3b}};


	ST->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
	ST->BootServices->LocateProtocol(&gop_guid, NULL, (void **)&GOP);
	ST->BootServices->LocateProtocol(&msp_guid, NULL, (void **)&MSP);
	ST->BootServices->LocateProtocol(&sfsp_guid, NULL, (void **)&SFSP);
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


#define TRUE 1
#define FALSE 0
void *find_efi_acpi_table()
{
	const struct EFI_GUID efi_acpi_table = {0x8868e871, 0xe4f1,0x11d3,
					{0xbc, 0x22, 0x00, 0x80,
					 0xc7, 0x3c, 0x88, 0x81}};

	for (int i = 0; i < ST->NumberOfTableEntries; i++) {
		struct EFI_GUID *guid = &ST->ConfigurationTable[i].VendorGuid;
		if ((guid->Data1 == efi_acpi_table.Data1)
		    && (guid->Data2 == efi_acpi_table.Data2)
		    && (guid->Data3 == efi_acpi_table.Data3)) {
			unsigned char is_equal = TRUE;
			for (int j = 0; j < 8; j++) {
				if (guid->Data4[j] != efi_acpi_table.Data4[j])
					is_equal = FALSE;
			}
			if (is_equal)
				return ST->ConfigurationTable[i].VendorTable;
		}
	}
	return NULL;
}

struct platform_info {
	struct framebuffer fb;
	void *rsdp;
	unsigned long long nproc;
} pi;

void init_pi()
{
	pi.fb.base = GOP->Mode->FrameBufferBase;
	pi.fb.size = GOP->Mode->FrameBufferSize;
	pi.fb.hr = GOP->Mode->Info->HorizontalResolution;
	pi.fb.vr = GOP->Mode->Info->VerticalResolution;

	pi.rsdp = find_efi_acpi_table();

	unsigned long long nproc, nproc_en;
	MSP->GetNumberOfProcessors(MSP, &nproc, &nproc_en);
	pi.nproc = nproc_en;
}


void safety_file_read(struct EFI_FILE_PROTOCOL *src, void *dst,
					  unsigned long long size)
{
	unsigned char *d = dst;
	unsigned long long status;

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

void load_kernel(/*struct EFI_SYSTEM_TABLE *ST*/)
{
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *kernel_file;
	unsigned int kernel_size;
	unsigned long long fib_size = FILE_INFO_BUF_SIZE;
	unsigned long long fi_buf[FILE_INFO_BUF_SIZE];
	struct EFI_FILE_INFO *fi_ptr;

	struct EFI_GUID fi_guid = {0x09576e92, 0x6d3f, 0x11d2,
				{0x8e, 0x39, 0x00, 0xa0,
				 0xc9, 0x69, 0x72, 0x3b}};


	SFSP->OpenVolume(SFSP, &root);
	root->Open(root, &kernel_file, KERNEL_FILE_NAME, EFI_FILE_MODE_READ, 0);

	struct header {
		void *bss_start;
		unsigned long long bss_size;
	} head;

	kernel_file->GetInfo(kernel_file, &fi_guid, &fib_size, fi_buf);
	fi_ptr = (struct EFI_FILE_INFO *)fi_buf;
	kernel_size = fi_ptr->FileSize - sizeof(head);

	safety_file_read(kernel_file, (void *)&head, sizeof(head));
	safety_file_read(kernel_file, (void *)KERNEL_START, kernel_size);
	
	kernel_file->Close(kernel_file);

	ST->BootServices->SetMem(head.bss_start, head.bss_size, 0);
}


/* I don't know, but if you put mem_desc variable into exit_boot_services, 
 * you get error message saying "undefined reference to `___chkstk_ms'".
 * I guess it is too big to fit in the function as local variable.
 * */
#define MEM_DESC_SIZE		4800
unsigned char mem_desc[MEM_DESC_SIZE];

void exit_boot_services(void *IH, struct EFI_SYSTEM_TABLE *ST)
{
	unsigned long long mem_desc_unit_size;
	unsigned long long map_key;
    unsigned long long mmap_size;
    unsigned int desc_ver;
	unsigned long long status;

    mmap_size = MEM_DESC_SIZE;

    ST->BootServices->GetMemoryMap(&mmap_size,
            (struct EFI_MEMORY_DESCRIPTOR *)mem_desc,
			&map_key, &mem_desc_unit_size, &desc_ver);
    ST->BootServices->ExitBootServices(IH, map_key);
}


void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE  *SystemTable)
{
    void *IH = ImageHandle;
    ST = SystemTable;


	ST->ConOut->ClearScreen(ST->ConOut);


	ST->ConOut->OutputString(ST->ConOut, L"Initializing the bootloader\n");
    efi_init(ST);

	ST->ConOut->OutputString(ST->ConOut, L"Initializing the platform information\n");
	init_pi();

	ST->ConOut->OutputString(ST->ConOut, L"Loading the kernel\n");
    load_kernel();

	ST->ConOut->OutputString(ST->ConOut, L"Exiting the bootloader\n");
    exit_boot_services(IH, ST);


	// jump to kernel code with passing kernel_arg
	unsigned long long kernel_arg = (unsigned long long)&pi;
	unsigned long long _sb = KERNEL_STACK_BASE;
	unsigned long long _ks = KERNEL_START;
	__asm__ (	"	mov	%0, %%rdi\n"
				"	mov	%1, %%rsp\n"
				"	jmp	*%2\n"
				::"m"(kernel_arg), "m"(_sb), "m"(_ks));


    while(1);
}