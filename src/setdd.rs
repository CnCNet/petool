use std::io;

use common;

pub fn main(args : &[~str]) -> io::IoResult<()> {
    fail_if!(args.len() != 4,
             "usage: petool setdd <image> <#DataDirectory> <VirtualAddress> <Size>");

    let mut file = try!(common::open_pe(&Path::new(args[0].as_slice()), io::Read));

    try!(common::validate_pe(&mut file));

    let (mut nt_header, _) = try!(common::read_headers(&mut file));

    let dd = read_arg!(1);
    fail_if!(nt_header.OptionalHeader.NumberOfRvaAndSizes <= dd as u32,
             "Missing data directory", Some(format!("Data directory \\#{} is missing.", dd)));

    nt_header.OptionalHeader.DataDirectory[dd].VirtualAddress = read_arg!(2) as u32;
    nt_header.OptionalHeader.DataDirectory[dd].Size           = read_arg!(3) as u32;

    /* FIXME: implement checksum calculation */
    nt_header.OptionalHeader.CheckSum = 0;

    try!(common::seek_nt_header(&mut file));

    try_complain!(file.write(common::as_bytes(nt_header).slice_to(
        nt_header.FileHeader.SizeOfOptionalHeader as uint)),
                  "Could not write back headers to executable".to_owned());
    Ok(())
}
