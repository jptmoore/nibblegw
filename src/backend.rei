type t;

let create: (~backend_uri_list: string) => t;

let flush: (~ctx: t) => Lwt.t(unit);

let length: (~ctx: t) => Lwt.t(int);

let length_in_memory: (~ctx: t) => Lwt.t(int);

let length_of_index: (~ctx: t) => Lwt.t(int);

let read_last: (~ctx: t, ~uri_path: string) => Lwt.t(Ezjsonm.t);

let read_latest: (~ctx: t) => Lwt.t(Ezjsonm.t);


