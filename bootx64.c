#include "efi.h"


#define NULL				(void *)0

#define KERNEL_FILE_NAME	"kernel.bin"
#define KERNEL_START		0x0000000000110000

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
}

struct EFI_FILE_PROTOCOL *search_volume_contains_file(
	unsigned short *target_filename, struct EFI_SYSTEM_TABLE *ST)
{
	void **sfs_handles;
	unsigned long long sfs_handles_num = 0;
	ST->BootServices->LocateHandleBuffer(
		ByProtocol, &sfsp_guid, NULL, &sfs_handles_num,
		(void ***)&sfs_handles);

	unsigned char i;
	struct EFI_FILE_PROTOCOL *fp;
	for (i = 0; i < sfs_handles_num; i++) {
		struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *tmp_sfsp;
		 ST->BootServices->HandleProtocol(
			sfs_handles[i], &sfsp_guid, (void **)&tmp_sfsp);

		tmp_sfsp->OpenVolume(tmp_sfsp, &fp);

		struct EFI_FILE_PROTOCOL *_target_fp;
		fp->Open(
			fp, &_target_fp, target_filename,
			EFI_FILE_MODE_READ, 0);

		return fp;
	}

	return NULL;
}

unsigned long long get_file_size(struct EFI_FILE_PROTOCOL *file)
{
	unsigned long long fi_size = FILE_INFO_BUF_SIZE;
	unsigned long long fi_buf[FILE_INFO_BUF_SIZE];
	struct EFI_FILE_INFO *fi_ptr;

	file->GetInfo(file, &fi_guid, &fi_size, fi_buf);

	fi_ptr = (struct EFI_FILE_INFO *)fi_buf;

	return fi_ptr->FileSize;
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

void load_kernel(struct EFI_SYSTEM_TABLE *ST)
{
	struct EFI_FILE_PROTOCOL *root = search_volume_contains_file(KERNEL_FILE_NAME, ST);
	struct EFI_FILE_PROTOCOL *kernel_file;
	
	root->Open(root, &kernel_file, KERNEL_FILE_NAME, EFI_FILE_MODE_READ, 0);

	unsigned long long kernel_size = get_file_size(kernel_file);

	struct header {
		void *bss_start;
		unsigned long long bss_size;
	} head;

	safety_file_read(kernel_file, (void *)&head, sizeof(head));

	kernel_size -= sizeof(head);
	safety_file_read(kernel_file, (void *)KERNEL_START, kernel_size);
	kernel_file->Close(kernel_file);

	ST->BootServices->SetMem(head.bss_start, head.bss_size, 0);
}

void exit_boot_services(void *IH, struct EFI_SYSTEM_TABLE *ST)
{
	unsigned char mem_desc[MEM_DESC_SIZE];
	unsigned long long mem_desc_unit_size;
	unsigned long long map_key;
    unsigned long long mmap_size;
    unsigned int desc_ver;

    mmap_size = MEM_DESC_SIZE;
    ST->BootServices->GetMemoryMap(&mmap_size,
            (struct EFI_MEMORY_DESCRIPTOR *)mem_desc,
            &map_key, &mem_desc_unit_size, &desc_ver);
    
    ST->BootServices->ExitBootServices(IH, map_key);
}

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE  *SystemTable)
{
    void *IH = ImageHandle;
    struct EFI_SYSTEM_TABLE *ST = SystemTable;


	ST->ConOut->ClearScreen(ST->ConOut);


	ST->ConOut->OutputString(ST->ConOut, L"Initializing the bootloader\n");
    efi_init(ST);

	ST->ConOut->OutputString(ST->ConOut, L"Loading the kernel\n");
    //load_kernel(ST);

	ST->ConOut->OutputString(ST->ConOut, L"Exiting the bootloader\n");
    exit_boot_services(IH, ST);


    while(1);
}