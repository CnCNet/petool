use std;
use std::cast;
use std::io;


use collections::enum_set::EnumSet;

use common;
use pe::{
    IMAGE_DOS_HEADER,     IMAGE_NT_HEADERS,
    IMAGE_DOS_SIGNATURE,  IMAGE_NT_SIGNATURE,
    IMAGE_SECTION_HEADER, IMAGE_FIRST_SECTION,
    CUST_Image_Section_Flags,
    IMAGE_SCN_MEM_READ, IMAGE_SCN_MEM_WRITE, IMAGE_SCN_MEM_EXECUTE,
    IMAGE_SCN_CNT_CODE, IMAGE_SCN_CNT_INITIALIZED_DATA, IMAGE_SCN_CNT_UNINITIALIZED_DATA,
};

pub unsafe fn dump(args : ~[~str]) -> Result<(), ~str> {

    fail_if!(args.len() < 2, ~"usage: petool dump <image>");

    let (_, image) = try!(common::open_and_read(&Path::new(args[1]), io::Read));
    fail_if!(image.len() < 512,                        ~"File too small.");

    let dos_hdr : &IMAGE_DOS_HEADER = cast::transmute(image.as_ptr());
    fail_if!(dos_hdr.e_magic != IMAGE_DOS_SIGNATURE, ~"File DOS signature invalid.");

    let nt_hdr  : &IMAGE_NT_HEADERS =
        cast::transmute(std::intrinsics::offset(dos_hdr, dos_hdr.e_lfanew as int));

    fail_if!(nt_hdr.Signature != IMAGE_NT_SIGNATURE, ~"File NT signature invalid.");

    println!(" section    start      end   length    vaddr    vsize  flags");
    println!("------------------------------------------------------------");

    for i in range(0, nt_hdr.FileHeader.NumberOfSections)
    {
        let cur_sct : &IMAGE_SECTION_HEADER =
            cast::transmute(std::intrinsics::offset(IMAGE_FIRST_SECTION(nt_hdr), i as int));

        let preflags : uint = cur_sct.Characteristics as uint;
        let flags : EnumSet<CUST_Image_Section_Flags> = cast::transmute(preflags);

        let name = std::str::raw::from_buf_len(cast::transmute(cur_sct.Name), cur_sct.Name.len());

        println!(
            "{:8.8s} {:8X} {:8X} {:8X} {:8X} {:8X} {}{}{}{}{}{}",
            //"%8.8s %8"PRIX32" %8"PRIX32" %8"PRIX32" %8"PRIX32" %8"PRIX32" %c%c%c%c%c%c",
            name,
            cur_sct.PointerToRawData,
            cur_sct.PointerToRawData + cur_sct.SizeOfRawData,
            cur_sct.SizeOfRawData,
            cur_sct.VirtualAddress + nt_hdr.OptionalHeader.ImageBase,
            cur_sct.PhysAddrORVirtSize,
            if flags.contains_elem(IMAGE_SCN_MEM_READ              ) { 'r' } else { '-' },
            if flags.contains_elem(IMAGE_SCN_MEM_WRITE             ) { 'w' } else { '-' },
            if flags.contains_elem(IMAGE_SCN_MEM_EXECUTE           ) { 'x' } else { '-' },
            if flags.contains_elem(IMAGE_SCN_CNT_CODE              ) { 'c' } else { '-' },
            if flags.contains_elem(IMAGE_SCN_CNT_INITIALIZED_DATA  ) { 'i' } else { '-' },
            if flags.contains_elem(IMAGE_SCN_CNT_UNINITIALIZED_DATA) { 'u' } else { '-' }
        );
    }

    if nt_hdr.OptionalHeader.NumberOfRvaAndSizes >= 2 {
        println!("Import Table: {:8X} ({:8X} bytes)",
                 nt_hdr.OptionalHeader.DataDirectory[1].VirtualAddress,
                 nt_hdr.OptionalHeader.DataDirectory[1].Size
                 );
    }
    Ok(())
}
