use std::cast::transmute;
use std::intrinsics::offset;
use std::io;
use std::io::{File, Truncate};

use collections::enum_set::EnumSet;

use common;
use pe::*;

pub unsafe fn main(args : &[~str]) -> Result<(), ~str> {

    fail_if!(args.len() != 2, ~"usage: petool pe2obj <in> <out>");

    let (_, image) = try!(common::open_and_read(&Path::new(args[0].as_slice()), io::Read));
    fail_if!(image.len() < 512,                      ~"File too small.");

    let dos_hdr : &IMAGE_DOS_HEADER = transmute(image.as_ptr());
    fail_if!(dos_hdr.e_magic != IMAGE_DOS_SIGNATURE, ~"File DOS signature invalid.");

    let nt_hdr  : &IMAGE_NT_HEADERS =
        transmute(offset(transmute::<_,*u8>(dos_hdr), dos_hdr.e_lfanew as int));
    fail_if!(nt_hdr.Signature != IMAGE_NT_SIGNATURE, ~"File NT signature invalid.");

    for i in range(0, nt_hdr.FileHeader.NumberOfSections)
    {
        let &mut cur_sct : &IMAGE_SECTION_HEADER =
            transmute(offset(IMAGE_FIRST_SECTION(nt_hdr), i as int));

        if cur_sct.PointerToRawData != 0 {
            cur_sct.PointerToRawData -= (dos_hdr.e_lfanew + 4) as u32;
        }

        let flags : EnumSet<CUST_Image_Section_Flags> =
            transmute(cur_sct.Characteristics as uint);

        if   flags.contains_elem(IMAGE_SCN_CNT_UNINITIALIZED_DATA) &&
            !flags.contains_elem(IMAGE_SCN_CNT_INITIALIZED_DATA)
        {
            cur_sct.PointerToRawData = 0;
            cur_sct.SizeOfRawData = 0;
        }
    }


    let mut out_file = try_complain!(File::open_mode(&Path::new(args[1].as_slice()), Truncate, io::Write),
                                  ~"Could not open output object file");

    let out_buf = image.as_slice().slice_from((dos_hdr.e_lfanew + 4) as uint);

    try_complain!(out_file.write(out_buf), ~"Failed to write object file to output file");

    Ok(())
}
