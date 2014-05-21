use std;
use std::io;

use common;

pub fn main(args : &[~str]) -> io::IoResult<()> {
    fail_if!(args.len() != 1, "usage: petool genlds <image>");

    let mut file = try!(common::open_pe(&Path::new(args[0].as_slice()), io::Read));

    try!(common::validate_pe(&mut file));

    let (nt_header, section_headers) = try!(common::read_headers(&mut file));

    println!("/* GNU ld linker script for {} */", args[0]);
    println!("start = {:#0.X};",
             nt_header.OptionalHeader.ImageBase
             + nt_header.OptionalHeader.AddressOfEntryPoint);

    println!("ENTRY(start);");
    println!("SECTIONS");
    std::io::stdio::println("{");

    for cur_sct in section_headers.iter()
    {
        let name = match std::str::from_utf8(cur_sct.Name.as_slice()) {
            None    => "BADNAME!",
            Some(s) => s,
        };

        println!("    {0: <15.} {1:#0.X} : \\{ *({0}) \\}",
                 name, cur_sct.VirtualAddress + nt_header.OptionalHeader.ImageBase);
    }

    println!("\\}");
    Ok(())
}
