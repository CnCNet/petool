use std;
use std::cast::transmute;
use std::intrinsics::offset;
use std::io;

use collections::enum_set::EnumSet;

use common;
use pe::*;


pub unsafe fn main(args : &[~str]) -> Result<(), ~str> {

    fail_if!(args.len() !=1,                         ~"usage: petool dump <image>");

    let (_, image) = try!(common::open_and_read(&Path::new(args[0].as_slice()), io::Read));
    fail_if!(image.len() < 512,                      ~"File too small.");

    let dos_hdr : &IMAGE_DOS_HEADER = transmute(image.as_ptr());
    fail_if!(dos_hdr.e_magic != IMAGE_DOS_SIGNATURE, ~"File DOS signature invalid.");

    let nt_hdr  : &IMAGE_NT_HEADERS =
        transmute(offset(transmute::<_,*u8>(dos_hdr), dos_hdr.e_lfanew as int));
    fail_if!(nt_hdr.Signature != IMAGE_NT_SIGNATURE, ~"File NT signature invalid.");

    println!(" section    start      end   length    vaddr    vsize  flags");
    println!("------------------------------------------------------------");

    for i in range(0, nt_hdr.FileHeader.NumberOfSections)
    {
        let cur_sct : &IMAGE_SECTION_HEADER =
            transmute(offset(IMAGE_FIRST_SECTION(nt_hdr), i as int));

        let flags : EnumSet<CUST_Image_Section_Flags> =
            transmute(cur_sct.Characteristics as uint);

        let name = match std::str::from_utf8(cur_sct.Name.as_slice()) {
            None    => "BADNAME!",
            Some(s) => s,
        };

        println!(
            "{: >8.8} {:8X} {:8X} {:8X} {:8X} {:8X} {}{}{}{}{}{}",
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
        println!("Import Table: {:8X} ({:X} bytes)",
                 nt_hdr.OptionalHeader.DataDirectory[1].VirtualAddress,
                 nt_hdr.OptionalHeader.DataDirectory[1].Size
                 );
    }
    Ok(())
}
