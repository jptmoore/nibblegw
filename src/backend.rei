type t;

let create: (~backend_uri_list: string) => t;

let validate_host: (~json: Ezjsonm.t) => option(string);

let add_host: (~ctx: t, ~host: string) => Lwt.t(unit);

let host_list: (~ctx: t) => Lwt.t(Ezjsonm.t);

let host_count: (~ctx: t) => Lwt.t(Ezjsonm.t);

let post: (~ctx: t, ~path: string, ~payload: string) => Lwt.t(unit);

let flush: (~ctx: t, ~path: string) => Lwt.t(unit);

let length: (~ctx: t, ~path: string) => Lwt.t(Ezjsonm.t);

let length_in_memory: (~ctx: t, ~path: string) => Lwt.t(Ezjsonm.t);

let length_of_index: (~ctx: t, ~path: string) => Lwt.t(Ezjsonm.t);

let read_last: (~ctx: t, ~path: string, ~n: int, ~args: list(string)) => Lwt.t(Ezjsonm.t);

let read_latest: (~ctx: t, ~path: string, ~args: list(string)) => Lwt.t(Ezjsonm.t);

let read_first: (~ctx: t, ~path: string, ~n: int, ~args: list(string)) => Lwt.t(Ezjsonm.t);

let read_earliest: (~ctx: t, ~path: string, ~args: list(string)) => Lwt.t(Ezjsonm.t);

let read_since: (~ctx: t, ~path: string, ~xargs: list(string)) => Lwt.t(Ezjsonm.t);

let read_range: (~ctx: t, ~path: string, ~xargs: list(string)) => Lwt.t(Ezjsonm.t);

let delete_since: (~ctx: t, ~path: string, ~xargs: list(string)) => Lwt.t(unit);

let delete_range: (~ctx: t, ~path: string, ~xargs: list(string)) => Lwt.t(unit);

let names: (~ctx: t, ~path: string) => Lwt.t(Ezjsonm.t);

let stats: (~ctx: t, ~path: string) => Lwt.t(Ezjsonm.t);

let status: (~ctx: t, ~path: string) => Lwt.t(Ezjsonm.t);

let health_check: (~ctx: t) => Lwt.t(bool);
