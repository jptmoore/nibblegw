open Lwt.Infix;

type t = {
  backend_uri_list: list(string),
  backend_count: int
};

let create = (~backend_uri_list) => {
  let lis = String.split_on_char(',', backend_uri_list);
  { 
    backend_uri_list: lis,
    backend_count: List.length(lis)
  }
};

let empty_data = Ezjsonm.list(Ezjsonm.unit,[]);

let flush = (~ctx) => {
  Lwt.return_unit
}

let length = (~ctx) => {
  Lwt.return(0)
}

let length_in_memory = (~ctx) => {
  Lwt.return(0)
}

let length_of_index = (~ctx) => {
  Lwt.return(0)
}

let read_last = (~ctx, ~uri_path) => {
  // todo: need swap the hostname to backend list
  Net.get(~uri=uri_path) >|= Ezjsonm.from_string;
}

let read_latest = (~ctx) => {
  Lwt.return(empty_data);
}










