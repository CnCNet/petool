use std;
use std::cast::transmute;
use std::intrinsics::offset;
use std::io;

use common;
use pe::*;

pub unsafe fn genlds(args : &[~str]) -> Result<(), ~str> {

    fail_if!(args.len() != 1, ~"usage: petool genlds <image>");


    let (_, image) = try!(common::open_and_read(&Path::new(args[0].as_slice()), io::Read));
    fail_if!(image.len() < 512,                      ~"File too small.");

    let dos_hdr : &IMAGE_DOS_HEADER = transmute(image.as_ptr());
    fail_if!(dos_hdr.e_magic != IMAGE_DOS_SIGNATURE, ~"File DOS signature invalid.");

    let nt_hdr  : &IMAGE_NT_HEADERS =
        transmute(offset(transmute::<_,*u8>(dos_hdr), dos_hdr.e_lfanew as int));
    fail_if!(nt_hdr.Signature != IMAGE_NT_SIGNATURE, ~"File NT signature invalid.");

    println!("/* GNU ld linker script for {} */", args[0]);
    println!("start = {:#0.X};",
             nt_hdr.OptionalHeader.ImageBase + nt_hdr.OptionalHeader.AddressOfEntryPoint);

    println!("ENTRY(start);");
    println!("SECTIONS");
    std::io::stdio::println("{");

    for i in range(0, nt_hdr.FileHeader.NumberOfSections)
    {
        let cur_sct : &IMAGE_SECTION_HEADER =
            transmute(offset(IMAGE_FIRST_SECTION(nt_hdr), i as int));

        let name = match std::str::from_utf8(cur_sct.Name.as_slice()) {
            None    => "BADNAME!",
            Some(s) => s,
        };

        println!("    {0: <15.} {1:#0.X} : \\{ *({0}) \\}",
                 name, cur_sct.VirtualAddress + nt_hdr.OptionalHeader.ImageBase);
    }

    println!("\\}");
    Ok(())
}
