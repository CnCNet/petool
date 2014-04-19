use std;
use std::cast::transmute;
use std::intrinsics::offset;
use std::io;
use std::io::BufReader;
use std::slice::raw::from_buf_raw;

use common;
use pe::*;

pub unsafe fn patch(args : &[~str]) -> Result<(), ~str> {

    fail_if!(args.len() < 1, ~"usage: petool patch <image> [section]");

    let (mut file, mut image) =
        try!(common::open_and_read(&Path::new(args[0].as_slice()), io::ReadWrite));

    fail_if!(image.len() < 512,                      ~"File too small.");

    let dos_hdr : &IMAGE_DOS_HEADER = transmute(image.as_ptr());
    fail_if!(dos_hdr.e_magic != IMAGE_DOS_SIGNATURE, ~"File DOS signature invalid.");

    let nt_hdr  : &IMAGE_NT_HEADERS =
        transmute(offset(transmute::<_,*u8>(dos_hdr), dos_hdr.e_lfanew as int));
    fail_if!(nt_hdr.Signature != IMAGE_NT_SIGNATURE, ~"File NT signature invalid.");

    let section : &str = if args.len() > 2 { args[1].as_slice() } else { &".patch" };

    let mut patch_sct = Err(~"No '%s' section in given PE image.");

    for i in range(0, nt_hdr.FileHeader.NumberOfSections) {
        let cur_sct : &IMAGE_SECTION_HEADER =
            transmute(offset(IMAGE_FIRST_SECTION(nt_hdr), i as int));

        let name = match std::str::from_utf8(cur_sct.Name.as_slice()) {
            None    => continue,
            Some(s) => s,
        };
        if name == section {
            patch_sct = Ok(cur_sct);
            break;
        }
    }

    let patch_sct = try!(patch_sct); // will return error if not set in loop

    { // naked block to limit lifetimes
        let (pre_patches, rest)       = image.mut_split_at(patch_sct.PointerToRawData   as uint);
        let (patch_buf, post_patches) = rest.mut_split_at(
            (nt_hdr.OptionalHeader.ImageBase + patch_sct.PhysAddrORVirtSize) as uint);
        let patches = &mut BufReader::new(patch_buf);

        try!(apply_all(patches, pre_patches, post_patches, patch_sct.PointerToRawData));

        /* FIXME: implement checksum calculation */
        {
            let mut h = nt_hdr.OptionalHeader;
            h.CheckSum = 0;
        }
    }

    try_complain!(file.seek(0, io::SeekSet),    ~"Failed to rewind file for writing");
    try_complain!(file.write(image.as_slice()), ~"Error writing executable");
    Ok(())
}

unsafe fn apply_all(
    patches : &mut BufReader, pre  : &mut [u8], post : &mut [u8], skip_offset : u32
) -> Result<(), ~str> {
    let err = ~"Could not read next patch";
    loop {
        let addr = match patches.read_le_u32() {
            Ok(addr) => if addr   == 0             { return Ok(()); } else { addr },
            Err(e)   => if e.kind == io::EndOfFile { return Ok(()); } else { return Err(err); }
        };
        let len  = try_complain!(patches.read_le_u32(), err);
        let patch : &[u8] = from_buf_raw(transmute(addr),
                                         len as uint);
        try!(patch_image(addr, patch, pre, post, skip_offset));
    }
}


unsafe fn patch_image(
    address : u32, patch : &[u8], pre  : &mut [u8], post : &mut [u8], skip_offset : u32
) -> Result<(), ~str> {
    let dos_hdr : &IMAGE_DOS_HEADER = transmute(pre.as_ptr());
    let nt_hdr  : &IMAGE_NT_HEADERS =
        transmute(offset(transmute::<_,*u8>(dos_hdr), dos_hdr.e_lfanew as int));

    for i in range(0, nt_hdr.FileHeader.NumberOfSections)
    {

        let cur_sct : &IMAGE_SECTION_HEADER =
            transmute(offset(IMAGE_FIRST_SECTION(nt_hdr), i as int));

        let cur_sct_runtime_base_address =
            cur_sct.VirtualAddress + nt_hdr.OptionalHeader.ImageBase;
        if  address <  cur_sct_runtime_base_address ||
            address >= cur_sct_runtime_base_address + cur_sct.SizeOfRawData {
            continue;
        }
        let common_offset = address - nt_hdr.OptionalHeader.ImageBase + cur_sct.PointerToRawData;
        let patchee = match cur_sct.PointerToRawData.cmp(&skip_offset) {
            Equal   => continue, // can't patch the patches section!
            Less    => pre. mut_slice_from( common_offset                as uint),
            Greater => post.mut_slice_from((common_offset - skip_offset) as uint)
        };
        if cur_sct.SizeOfRawData + cur_sct_runtime_base_address < patch.len() as u32 + address
        {
            return Err(format!(
                "Error: section length ({:u}) is less than patch length ({:X}), maybe expand the image a bit more?\n",
                cur_sct.SizeOfRawData, patch.len()));
        }
        std::slice::bytes::copy_memory(patchee, patch);
        println!("PATCH  {:8u} bytes -> {:8X}", patch.len(), address);
        return Ok(());
    }
    Err(format!("Error: memory address {:8X} not found in image", address))
}
