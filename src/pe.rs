#![allow(non_camel_case_types)]
#![allow(dead_code)]
#![allow(uppercase_variables)]
// based on MingW headers converted to stdint, and then to rust

use std::mem::transmute;
use std::intrinsics::offset;

use collections::enum_set::CLike;


pub static IMAGE_DOS_SIGNATURE          : u16 = 0x5A4D; /* MZ   */
pub static IMAGE_OS2_SIGNATURE          : u16 = 0x454E; /* NE   */
pub static IMAGE_OS2_SIGNATURE_LE       : u16 = 0x454C; /* LE   */
pub static IMAGE_OS2_SIGNATURE_LX       : u16 = 0x584C; /* LX */
pub static IMAGE_VXD_SIGNATURE          : u16 = 0x454C; /* LE   */

pub static IMAGE_NT_SIGNATURE           : u32 = 0x00004550; /* PE00 */

#[repr(uint)] // we store index of bit, instead
pub enum CUST_Image_Section_Flags {
    IMAGE_SCN_CNT_CODE                    = 0x05, //0x00000020,
    IMAGE_SCN_CNT_INITIALIZED_DATA        = 0x06, //0x00000040,
    IMAGE_SCN_CNT_UNINITIALIZED_DATA      = 0x07, //0x00000080,
    IMAGE_SCN_MEM_EXECUTE                 = 0x1D, //0x20000000,
    IMAGE_SCN_MEM_READ                    = 0x1E, //0x40000000,
    IMAGE_SCN_MEM_WRITE                   = 0x1F, //0x80000000
}

impl CLike for CUST_Image_Section_Flags {
    fn to_uint(&self) -> uint {
        *self as uint
    }
    fn from_uint(u : uint) -> CUST_Image_Section_Flags {
        unsafe { transmute(u) }
    }
}


#[repr(uint)] // indicies into Data
pub enum CUST_Image_Directory_Entry_Type {
    IMAGE_DIRECTORY_ENTRY_EXPORT          = 0,    // Export Directory
    IMAGE_DIRECTORY_ENTRY_IMPORT          = 1,    // Import Directory
    IMAGE_DIRECTORY_ENTRY_RESOURCE        = 2,    // Resource Directory
    IMAGE_DIRECTORY_ENTRY_EXCEPTION       = 3,    // Exception Directory
    IMAGE_DIRECTORY_ENTRY_SECURITY        = 4,    // Security Directory
    IMAGE_DIRECTORY_ENTRY_BASERELOC       = 5,    // Base Relocation Table
    IMAGE_DIRECTORY_ENTRY_DEBUG           = 6,    // Debug Directory
    IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    = 7,    // Architecture Specific Data
    IMAGE_DIRECTORY_ENTRY_GLOBALPTR       = 8,    // RVA of GP
    IMAGE_DIRECTORY_ENTRY_TLS             = 9,    // TLS Directory
    IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG     = 10,   // Load Configuration Directory
    IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT    = 11,   // Bound Import Directory in headers
    IMAGE_DIRECTORY_ENTRY_IAT             = 12,   // Import Address Table
    IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT    = 13,   // Delay Load Import Descriptors
    IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR  = 14    // COM Runtime descriptor
}

#[inline]
pub fn index<'a>(
    i : CUST_Image_Directory_Entry_Type,
    arr : &'a [IMAGE_DATA_DIRECTORY, ..IMAGE_NUMBEROF_DIRECTORY_ENTRIES]
        ) -> &'a IMAGE_DATA_DIRECTORY
{
    let i2 : uint = unsafe { transmute(i) };
    &arr[i2]
}

#[macro_export]
macro_rules! field_offset(
    ($t:ty, $f:ident) => (unsafe { transmute<uint, *$t>(& transmute<uint, *$t>(0).$f) })
)

pub static IMAGE_SIZEOF_SHORT_NAME          : u8 = 8;
pub static IMAGE_NUMBEROF_DIRECTORY_ENTRIES : u8 = 16;

#[inline]
pub unsafe fn IMAGE_FIRST_SECTION<'a>(h : &'a IMAGE_NT_HEADERS) -> &'a IMAGE_SECTION_HEADER {
    transmute(offset(transmute::<_,*const u8>(&h.OptionalHeader),
                     h.FileHeader.SizeOfOptionalHeader as int))
}

#[packed] // align 2
pub struct IMAGE_DOS_HEADER {
    pub e_magic                     : u16,
    pub e_cblp                      : u16,
    pub e_cp                        : u16,
    pub e_crlc                      : u16,
    pub e_cparhdr                   : u16,
    pub e_minalloc                  : u16,
    pub e_maxalloc                  : u16,
    pub e_ss                        : u16,
    pub e_sp                        : u16,
    pub e_csum                      : u16,
    pub e_ip                        : u16,
    pub e_cs                        : u16,
    pub e_lfarlc                    : u16,
    pub e_ovno                      : u16,
    pub e_res                       : [u16, ..4],
    pub e_oemid                     : u16,
    pub e_oeminfo                   : u16,
    pub e_res2                      : [u16, ..10],
    pub e_lfanew                    : u16
} //IMAGE_DOS_HEADER,*PIMAGE_DOS_HEADER;

#[packed] // align 4
pub struct IMAGE_FILE_HEADER {
    pub Machine                     : u16,
    pub NumberOfSections            : u16,
    pub TimeDateStamp               : u32,
    pub PointerToSymbolTable        : u32,
    pub NumberOfSymbols             : u32,
    pub SizeOfOptionalHeader        : u16,
    pub Characteristics             : u16,
 } //IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#[packed] // align 4
pub struct IMAGE_DATA_DIRECTORY {
    pub VirtualAddress              : u32,
    pub Size                        : u32,
} //IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#[packed] // align 4
pub struct IMAGE_OPTIONAL_HEADER {
    pub Magic                       : u16,
    pub MajorLinkerVersion          : u8,
    pub MinorLinkerVersion          : u8,
    pub SizeOfCode                  : u32,
    pub SizeOfInitializedData       : u32,
    pub SizeOfUninitializedData     : u32,
    pub AddressOfEntryPoint         : u32,
    pub BaseOfCode                  : u32,
    pub BaseOfData                  : u32,
    pub ImageBase                   : u32,
    pub SectionAlignment            : u32,
    pub FileAlignment               : u32,
    pub MajorOperatingSystemVersion : u16,
    pub MinorOperatingSystemVersion : u16,
    pub MajorImageVersion           : u16,
    pub MinorImageVersion           : u16,
    pub MajorSubsystemVersion       : u16,
    pub MinorSubsystemVersion       : u16,
    pub Win32VersionValue           : u32,
    pub SizeOfImage                 : u32,
    pub SizeOfHeaders               : u32,
    pub CheckSum                    : u32,
    pub Subsystem                   : u16,
    pub DllCharacteristics          : u16,
    pub SizeOfStackReserve          : u32,
    pub SizeOfStackCommit           : u32,
    pub SizeOfHeapReserve           : u32,
    pub SizeOfHeapCommit            : u32,
    pub LoaderFlags                 : u32,
    pub NumberOfRvaAndSizes         : u32,
    pub DataDirectory               : [IMAGE_DATA_DIRECTORY, ..IMAGE_NUMBEROF_DIRECTORY_ENTRIES],
} //IMAGE_OPTIONAL_HEADER,*PIMAGE_OPTIONAL_HEADER;

#[packed] // align 4
pub struct IMAGE_NT_HEADERS {
    pub Signature                   : u32,
    pub FileHeader                  : IMAGE_FILE_HEADER,
    pub OptionalHeader              : IMAGE_OPTIONAL_HEADER
} //IMAGE_NT_HEADERS,*PIMAGE_NT_HEADERS;

#[packed] // align 4
pub struct IMAGE_SECTION_HEADER {
    pub Name                        : [u8, ..IMAGE_SIZEOF_SHORT_NAME],
    pub PhysAddrORVirtSize          : u32,
//      union {
//          u32 PhysicalAddress;
//          u32 VirtualSize;
//      } Misc;
    pub VirtualAddress              : u32,
    pub SizeOfRawData               : u32,
    pub PointerToRawData            : u32,
    pub PointerToRelocations        : u32,
    pub PointerToLinenumbers        : u32,
    pub NumberOfRelocations         : u16,
    pub NumberOfLinenumbers         : u16,
    pub Characteristics             : u32,
} //IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;
