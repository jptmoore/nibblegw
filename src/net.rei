let post: (~uri: string, ~payload: string) => Lwt.t(string)
let get: (~uri: string) => Lwt.t(string)
let delete: (~uri: string) => Lwt.t(string)
let status: (~uri: string) => Lwt.t(int)