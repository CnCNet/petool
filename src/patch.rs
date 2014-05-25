use std::io;

use common;
use pe;

pub fn main(args : &[~str]) -> io::IoResult<()>
{
    fail_if!(args.len() < 1, "usage: petool patch <image> [section]");

    let path = &Path::new(args[0].as_slice());
    let mut reader = try!(common::open_pe(path, io::Read));
    let mut writer = try!(common::open_pe(path, io::ReadWrite)); // need two positions

    try!(common::validate_pe(&mut reader));

    let (nt_header, section_headers) = try!(common::read_headers(&mut reader));

    let name : &str = if args.len() > 2 { args[1].as_slice() } else { ".patch" };

    try!(writer.seek(try!(reader.tell()) as i64, io::SeekSet)); // sync positions

    let patch_section = match section_headers.iter().find(
        |cur_sct| ::std::str::from_utf8(cur_sct.Name.as_slice()) == Some(name))
    {
        None => return Err(common::new_error("No patch section",
                              Some(format!("searched for name = {}", name)))),
        Some(section) => section
    };

    try!(reader.seek(
        (patch_section.PointerToRawData + nt_header.OptionalHeader.ImageBase) as i64,
        io::SeekSet));

    try!(patch_all(&mut reader, &mut writer, nt_header, section_headers));

    Ok(())
}

fn patch_all<In : Reader, Out : Reader + Writer + Seek>(
    patches         : &mut In,
    out             : &mut Out,
    nt_header       : &pe::IMAGE_NT_HEADERS,
    section_headers : &[pe::IMAGE_SECTION_HEADER]
        ) -> io::IoResult<()>
{
    static err_msg : &'static str = "Could not read next patch";
    loop {
        let address = try_complain!(patches.read_le_u32(), err_msg.to_owned());
        if address == 0 { return Ok(()); } // patches are null teriminated

        let length = try_complain!(patches.read_le_u32(), err_msg.to_owned());
        let patch_data = patches.read_exact(length as uint);

        try!(patch(try!(patch_data).as_slice(), address,
                   out, nt_header, section_headers));
    }
}

fn patch<Out : Reader + Writer + Seek>(
    patch           : &[u8],
    address         : u32,
    out             : &mut Out,
    nt_header       : &pe::IMAGE_NT_HEADERS,
    section_headers : &[pe::IMAGE_SECTION_HEADER]
        ) -> io::IoResult<()>
{
    let dest_section = match section_headers.iter().find(
        |cur_sct| {
            let runtime_base_address =
                cur_sct.VirtualAddress + nt_header.OptionalHeader.ImageBase;
            address >= runtime_base_address &&
                address < runtime_base_address + cur_sct.SizeOfRawData
        })
    {
        None => return Err(common::new_error("patch begin address out of bounds",
                                             Some(format!("address = {:8X}", address)))),
        Some(section) => section
    };
    // pitty I need to define it again
    let runtime_base_address = dest_section.VirtualAddress + nt_header.OptionalHeader.ImageBase;
    if dest_section.SizeOfRawData + runtime_base_address < patch.len() as u32 + address
    {
        return Err(common::new_error(
            "patch longer than remaining length of section, maybe expand the image a bit more?",
            Some(format!("section length = {:u}; patch length = {:X}",
                         dest_section.SizeOfRawData, patch.len()))));
    }
    try!(out.seek((address - runtime_base_address + dest_section.PointerToRawData) as i64,
                  io::SeekSet));
    try!(out.write(patch));
    println!("PATCH  {:8u} bytes -> {:8X}", patch.len(), address);
    Ok(())
}
