type t;

let create: (~backend_uri_list: string) => t;

let flush: (~ctx: t) => Lwt.t(unit);

let length: (~ctx: t, ~path: string) => Lwt.t(Ezjsonm.t);

let length_in_memory: (~ctx: t) => Lwt.t(int);

let length_of_index: (~ctx: t) => Lwt.t(int);

let read_last: (~ctx: t, ~path: string, ~n: int, ~args: list(string)) => Lwt.t(Ezjsonm.t);

let read_latest: (~ctx: t, ~path: string, ~args: list(string)) => Lwt.t(Ezjsonm.t);

let read_first: (~ctx: t, ~path: string, ~n: int, ~args: list(string)) => Lwt.t(Ezjsonm.t);

let read_earliest: (~ctx: t, ~path: string, ~args: list(string)) => Lwt.t(Ezjsonm.t);

let read_since: (~ctx: t, ~path: string, ~xargs: list(string)) => Lwt.t(Ezjsonm.t);

let read_range: (~ctx: t, ~path: string, ~xargs: list(string)) => Lwt.t(Ezjsonm.t);