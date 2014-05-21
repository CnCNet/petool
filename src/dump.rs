use std;
use std::intrinsics::transmute;
use std::io;

use collections::enum_set::EnumSet;

use common;
use pe::*;


pub fn main(args : &[~str]) -> io::IoResult<()> {
    fail_if!(args.len() !=1,                         "usage: petool dump <image>");

    let mut file = try!(common::open_pe(&Path::new(args[0].as_slice()), io::Read));

    try!(common::validate_pe(&mut file));

    let (nt_header, section_headers) = try!(common::read_headers(&mut file));

    println!(" section    start      end   length    vaddr    vsize  flags");
    println!("------------------------------------------------------------");
    for cur_sct in section_headers.iter()
    {
        let flags : EnumSet<CUST_Image_Section_Flags> = unsafe {
            transmute(cur_sct.Characteristics as uint)
        };

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
            cur_sct.VirtualAddress + nt_header.OptionalHeader.ImageBase,
            cur_sct.PhysAddrORVirtSize,
            if flags.contains_elem(IMAGE_SCN_MEM_READ              ) { 'r' } else { '-' },
            if flags.contains_elem(IMAGE_SCN_MEM_WRITE             ) { 'w' } else { '-' },
            if flags.contains_elem(IMAGE_SCN_MEM_EXECUTE           ) { 'x' } else { '-' },
            if flags.contains_elem(IMAGE_SCN_CNT_CODE              ) { 'c' } else { '-' },
            if flags.contains_elem(IMAGE_SCN_CNT_INITIALIZED_DATA  ) { 'i' } else { '-' },
            if flags.contains_elem(IMAGE_SCN_CNT_UNINITIALIZED_DATA) { 'u' } else { '-' }
        );
    }

    if nt_header.OptionalHeader.NumberOfRvaAndSizes >= 2 {
        println!("Import Table: {:8X} ({:X} bytes)",
                 nt_header.OptionalHeader.DataDirectory[1].VirtualAddress,
                 nt_header.OptionalHeader.DataDirectory[1].Size
                 );
    }
    Ok(())
}
