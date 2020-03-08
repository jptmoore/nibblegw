
let validate_json: Ezjsonm.t => option((int64, Ezjsonm.t));

let write: (~timestamp: int64, ~id: string, ~json: Ezjsonm.t) => Lwt.t(unit);

let flush: unit => Lwt.t(unit);

let length: (~id_list: list(string)) => Lwt.t(int);

let length_in_memory: (~id_list: list(string)) => Lwt.t(int);

let length_of_index: (~id_list: list(string)) => Lwt.t(int);

let delete: (~id_list: list(string), ~json: Ezjsonm.t) => Lwt.t(unit);

let read_last: (~id_list: list(string), ~n: int, ~xargs: list(string)) => Lwt.t(Ezjsonm.t);

let read_latest: (~id_list: list(string), ~xargs: list(string)) => Lwt.t(Ezjsonm.t);

let read_first: (~id_list: list(string), ~n: int, ~xargs: list(string)) => Lwt.t(Ezjsonm.t);

let read_earliest: (~id_list: list(string), ~xargs: list(string)) => Lwt.t(Ezjsonm.t);

let read_since: (~id_list: list(string), ~from: int64, ~xargs: list(string)) => Lwt.t(Ezjsonm.t);

let read_range: (~id_list: list(string), ~from: int64, ~to_: int64, ~xargs: list(string)) => Lwt.t(Ezjsonm.t);

