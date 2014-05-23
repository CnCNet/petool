use std::intrinsics::transmute;
use std::io;

use collections::enum_set::EnumSet;

use common;
use pe::*;

pub fn main(args : &[~str]) -> io::IoResult<()> {
    fail_if!(args.len() != 2, "usage: petool pe2obj <in> <out>");

    let mut in_file = try!(common::open_pe(&Path::new(args[0].as_slice()), io::Read));

    try!(common::validate_pe(&mut in_file));

    let nt_offset = try!(common::seek_nt_header(&mut in_file));

    let (nt_header, section_headers) = try!(common::read_headers(&mut in_file));

    for &mut cur_sct in section_headers.iter()
    {
        if cur_sct.PointerToRawData != 0 {
            cur_sct.PointerToRawData -= (nt_offset + 4) as u32;
        }

        let flags : EnumSet<CUST_Image_Section_Flags> = unsafe {
            transmute(cur_sct.Characteristics as uint)
        };

        if   flags.contains_elem(IMAGE_SCN_CNT_UNINITIALIZED_DATA) &&
            !flags.contains_elem(IMAGE_SCN_CNT_INITIALIZED_DATA)
        {
            cur_sct.PointerToRawData = 0;
            cur_sct.SizeOfRawData = 0;
        }
    }

    let mut out_file = try!(io::File::open_mode(&Path::new(args[1].as_slice())
                                            , io::Truncate, io::Write));

    // pitty we cant use common::write*

    try!(out_file.write(common::as_bytes(nt_header)
                   .slice_to(nt_header.FileHeader.SizeOfOptionalHeader as uint)
                   .slice_from(4))); // drop first u32 field

    try!(out_file.write(common::array_as_bytes(section_headers)));

    // relies on common::read_headers being the last seek/read/write call for in_file
    try!(out_file.write(try!(in_file.read_to_end()).as_slice()));

    Ok(())
}
