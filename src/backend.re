open Lwt.Infix;

type t = {
  backend_uri: string
};

let create = (~backend_uri) => {
  backend_uri: backend_uri
};

let empty_data = Ezjsonm.list(Ezjsonm.unit,[]);

let get_microseconds = () => {
    let t = Unix.gettimeofday() *. 1000.0 *. 1000.0;
    Int64.of_float(t);
};

let validate_json = (json) => {
    open Ezjsonm;
    open Int64;
    switch (get_dict(value(json))) {
    | [("value",`Float _)] =>
        Some((get_microseconds(), json));
    | [("timestamp",`Float ts), ("value",`Float n)] =>
        Some((of_float(ts), dict([("value",`Float(n))])));
    | [(_, `String _), ("value",`Float _)] =>
        Some((get_microseconds(), json));
    | [("timestamp",`Float ts), (tag_name, `String tag_value), ("value",`Float n)] =>
        Some((of_float(ts), dict([(tag_name, `String(tag_value)), ("value",`Float(n))])));
    | _ => None;
    }
  }

let write = (~timestamp: int64, ~id: string, ~json: Ezjsonm.t) => {
  Lwt.return_unit
}

let flush = () => {
  Lwt.return_unit
}

let length = (~id_list: list(string)) => {
  Lwt.return(0)
}

let length_in_memory = (~id_list: list(string)) => {
  Lwt.return(0)
}

let length_of_index = (~id_list: list(string)) => {
  Lwt.return(0)
}

let delete = (~id_list: list(string), ~json: Ezjsonm.t) => {
  Lwt.return_unit
}

let read_last = (~ctx, ~id_list: list(string), ~n: int, ~xargs: list(string)) => {
  Net.get(~uri=ctx.backend_uri) >|= Ezjsonm.from_string;
}

let read_latest = (~id_list: list(string), ~xargs: list(string)) => {
  Lwt.return(empty_data);
}

let read_first = (~id_list: list(string), ~n: int, ~xargs: list(string)) => {
  Lwt.return(empty_data);
}

let read_earliest = (~id_list: list(string), ~xargs: list(string)) => {
  Lwt.return(empty_data);
}

let read_since = (~id_list: list(string), ~from: int64, ~xargs: list(string)) => {
  Lwt.return(empty_data);
}

let read_range = (~id_list: list(string), ~from: int64, ~to_: int64, ~xargs: list(string)) => {
  Lwt.return(empty_data);
}








