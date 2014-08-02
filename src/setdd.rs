use std::mem::transmute;
use std::intrinsics::offset;
use std::io;

use common;
use pe::*;

pub unsafe fn main(args : &[Box<str>]) -> Result<(), Box<str>> {
    fail_if!(args.len() != 4,
             box "usage: petool setdd <image> <#DataDirectory> <VirtualAddress> <Size>");

    let (mut file, image) =
        try!(common::open_and_read(&Path::new(args[0].as_slice()), io::ReadWrite));

    fail_if!(image.len() < 512,                      box "File too small.");

    let dos_hdr : &IMAGE_DOS_HEADER = transmute(image.as_ptr());
    fail_if!(dos_hdr.e_magic != IMAGE_DOS_SIGNATURE, box "File DOS signature invalid.");

    let nt_hdr  : &mut IMAGE_NT_HEADERS =
        transmute(offset(transmute::<_,*const u8>(dos_hdr), dos_hdr.e_lfanew as int));
    fail_if!(nt_hdr.Signature != IMAGE_NT_SIGNATURE, box "File NT signature invalid.");

    let dd = common::read_arg(args, 1);
    fail_if!(nt_hdr.OptionalHeader.NumberOfRvaAndSizes <= dd as u32,
             format!("Data directory \\#{} is missing.", dd));

    nt_hdr.OptionalHeader.DataDirectory[dd].VirtualAddress = common::read_arg(args, 2) as u32;
    nt_hdr.OptionalHeader.DataDirectory[dd].Size           = common::read_arg(args, 3) as u32;

    /* FIXME: implement checksum calculation */
    nt_hdr.OptionalHeader.CheckSum = 0;

    try_complain!(file.seek(0, io::SeekSet),    box "Failed to rewind file for writing");
    try_complain!(file.write(image.as_slice()), box "Error writing executable");
    Ok(())
}
