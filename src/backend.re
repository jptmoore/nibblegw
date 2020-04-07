open Lwt.Infix;

type t = {
  backend_uri_list: list(string),
  backend_count: int
};


let add_backend_host(host, path, xargs) {
  host ++ path;
}

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

let convert = (data) => {
  open List;
  let rec loop = (acc, l) => {
    switch (l) {
    | [] => acc;
    | [ json, ...rest] => {
          let item = Ezjsonm.value(json);
          loop(cons(item, acc), rest);
        };
    }
  };
  loop([], data);
};


let read_last_worker(host, path, xargs) {
  let uri = add_backend_host(host, path, xargs)
  Lwt_io.printf("accessing backend uri:%s\n", uri) >>= 
    () => Net.get(~uri) >|= Ezjsonm.from_string;
}


let read_last = (~ctx, ~path, ~xargs) => {
  Lwt_list.map_p(host => read_last_worker(host, path, xargs), ctx.backend_uri_list) >|=
    convert >|= Ezjsonm.list(x => x);
}

let read_latest = (~ctx) => {
  Lwt.return(empty_data);
}










