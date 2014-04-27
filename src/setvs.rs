use std;
use std::cast::transmute;
use std::intrinsics::offset;
use std::io;

use common;
use pe::*;

pub unsafe fn main(args : &[~str]) -> Result<(), ~str> {
    fail_if!(args.len() != 3,
             ~"usage: petool setvs <image> <section> <VirtualSize>");

    let (mut file, image) =
        try!(common::open_and_read(&Path::new(args[0].as_slice()), io::ReadWrite));

    fail_if!(image.len() < 512,                      ~"File too small.");

    let dos_hdr : &IMAGE_DOS_HEADER = transmute(image.as_ptr());
    fail_if!(dos_hdr.e_magic != IMAGE_DOS_SIGNATURE, ~"File DOS signature invalid.");

    let nt_hdr  : &mut IMAGE_NT_HEADERS =
        transmute(offset(transmute::<_,*u8>(dos_hdr), dos_hdr.e_lfanew as int));
    fail_if!(nt_hdr.Signature != IMAGE_NT_SIGNATURE, ~"File NT signature invalid.");

    let section : &str = args[1].as_slice();
    let vs = read_arg!(2) as u32;

    fail_if!(vs == 0,                                ~"VirtualSize can't be zero.");

    for i in range(0, nt_hdr.FileHeader.NumberOfSections) {
        let cur_sct : &mut IMAGE_SECTION_HEADER =
            transmute(offset(IMAGE_FIRST_SECTION(nt_hdr), i as int));

        let name = match std::str::from_utf8(cur_sct.Name.as_slice()) {
            None    => continue,
            Some(s) => s,
        };
        if name != section { continue; }
        fail_if!(cur_sct.SizeOfRawData > vs, ~"VirtualSize can't be smaller than raw size.");
        // update total virtual size of image
        nt_hdr.OptionalHeader.SizeOfImage += vs - cur_sct.PhysAddrORVirtSize;
        cur_sct.PhysAddrORVirtSize = vs; // update section size
        { // FIXME: implement checksum calculation
            let mut h = nt_hdr.OptionalHeader;
            h.CheckSum = 0;
        }

        try_complain!(file.seek(0, io::SeekSet),     ~"Failed to rewind file for writing");
        try_complain!(file.write(image.as_slice()),  ~"Error writing executable");
        return Ok(());
    }

    Err(format!("No '{}' section in given PE image.", section))
}
