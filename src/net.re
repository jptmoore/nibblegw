open Lwt;
open Cohttp_lwt_unix;

let content_format = ref("json");

let post_worker = (~uri, ~payload) => {
  let headers = Cohttp.Header.of_list([("Accept", content_format^), ("Connection", "keep-alive")]);
  let body = Cohttp_lwt.Body.of_string(payload);
  Client.post(~headers=headers, ~body=body, Uri.of_string(uri)) >>= 
    (((_, body)) => body |> Cohttp_lwt.Body.to_string);
};


let post = (~uri, ~payload) => {
  post_worker(~uri, ~payload);
}

let get_worker = (~uri) => {
  let headers = Cohttp.Header.of_list([("Content-Type", content_format^)]);
  Client.get(~headers=headers, Uri.of_string(uri)) >>=
    ((_, body)) => body |> Cohttp_lwt.Body.to_string;
}

let get = (~uri) => {
  get_worker(~uri);
};

let status = (~uri) => {
  Client.get(Uri.of_string(uri)) >|=
    ((response, _)) => Cohttp.Code.code_of_status(response.status)
}

let delete_worker = (~uri) => {
  let headers = Cohttp.Header.of_list([]);
  Client.delete(~headers=headers, Uri.of_string(uri)) >>=
    ((_, body)) => body |> Cohttp_lwt.Body.to_string;
}

let delete = (~uri) => {
  delete_worker(~uri);
};

let validate_host_worker = (name) => {
  switch (String.split_on_char(':', name)) {
    | [name, port] => 
      try {
        ignore(Unix.gethostbyname(name));
        true;
      } {
      | Not_found => false
      };
    | _ => false;
    }
}

let validate_host = (~host) => {
  switch (String.split_on_char('/', host)) {
  | ["http:"|"https:",_,name] => validate_host_worker(name);
  | _ => false;
  }
}