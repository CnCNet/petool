//use std;
use std::io;

use common;

pub fn main(args : &[~str]) -> io::IoResult<()> {
    fail_if!(args.len() != 3,
             "usage: petool setvs <image> <section> <VirtualSize>");

    let mut file = try!(common::open_pe(&Path::new(args[0].as_slice()), io::Read));

    try!(common::validate_pe(&mut file));

    let (mut nt_header, section_headers) = try!(common::read_headers(&mut file));

    let name : &str = args[1].as_slice();
    let vs = read_arg!(2) as u32;

    fail_if!(vs == 0,                                "VirtualSize can't be zero.");

    let &mut section = match section_headers.iter().find(
        |cur_sct| ::std::str::from_utf8(cur_sct.Name.as_slice()) == Some(name))
    {
        None => return Err(common::new_error("section not in image",
                                      Some(format!("section = {}", name)))),
        Some(section) => section
    };

    fail_if!(section.SizeOfRawData > vs, "VirtualSize can't be smaller than raw size.");

    // update total virtual size of image
    nt_header.OptionalHeader.SizeOfImage += vs - section.PhysAddrORVirtSize;
    section.PhysAddrORVirtSize = vs; // update section size

    // FIXME: implement checksum calculation
    nt_header.OptionalHeader.CheckSum = 0;

    try!(common::write_headers(&mut file, nt_header, section_headers));

    Ok(())
}
