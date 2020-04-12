open Lwt.Infix;

open Cohttp;

open Cohttp_lwt_unix;

let http_port = ref(5000);
let log_mode = ref(false);
let tls_mode = ref(false);
let cert_file = ref("/tmp/server.crt");
let key_file = ref("/tmp/server.key");
let backend_uri_list = ref("http://localhost:8000");

type t = {
  db: Backend.t,
  m: Lwt_mutex.t
};

exception Interrupt(string);


module Http_response {
  let json_header = Header.of_list([("Content-Type", "application/json")]);
  let text_header = Header.of_list([("Content-Type", "text/plain")]);
  let ok = (~content="", ()) => {
    Server.respond_string(~status=`OK, ~body=content, ~headers=json_header, ());
  };
  let bad_request = (~content="", ()) => {
    Server.respond_string(~status=`Bad_request, ~body=content, ~headers=text_header, ());
  };
  let not_found = (~content="", ()) => {
    Server.respond_string(~status=`Not_found, ~body=content, ~headers=text_header, ());
  };
  let internal_server_error = (~content="", ()) => {
    Server.respond_string(~status=`Internal_server_error, ~body=content, ~headers=text_header, ());
  };
};

let read_last = (ctx, uri_path, n, xargs) => {
  Backend.read_last(ctx.db, uri_path, int_of_string(n), xargs) >|=
    Ezjsonm.to_string >>= s => Http_response.ok(~content=s, ()) 
};

let read_latest = (ctx, uri_path, xargs) => {
  Backend.read_latest(ctx.db, uri_path, xargs) >|=
    Ezjsonm.to_string >>= s => Http_response.ok(~content=s, ()) 
};

let read_first = (ctx, uri_path, n, xargs) => {
  Backend.read_first(ctx.db, uri_path, int_of_string(n), xargs) >|=
    Ezjsonm.to_string >>= s => Http_response.ok(~content=s, ()) 
};

let read_earliest = (ctx, uri_path, xargs) => {
  Backend.read_earliest(ctx.db, uri_path, xargs) >|=
    Ezjsonm.to_string >>= s => Http_response.ok(~content=s, ()) 
};

let length = (ctx) => {
  Backend.length(ctx.db) >>=
    n => Http_response.ok(~content=Printf.sprintf("{\"length\":%d}", n), ()) 
}

let length_in_memory = (ctx) => {
  Backend.length_in_memory(ctx.db) >>=
    n => Http_response.ok(~content=Printf.sprintf("{\"length\":%d}", n), ()) 
}

let length_of_index = (ctx) => {
  Backend.length_of_index(ctx.db) >>=
    n => Http_response.ok(~content=Printf.sprintf("{\"length\":%d}", n), ()) 
}

let timeseries_sync = (ctx) => {
  Backend.flush(ctx.db) >>=
    () => Http_response.ok()
}

let get_req = (ctx, path_list, uri_path) => {
  switch (path_list) {
  | [_, _, _, "ts", ids, "last", n, ...xargs] => read_last(ctx, "/ts/"++ids++"/last/"++n, n, xargs)
  | [_, _, _, "ts", ids, "latest", ...xargs] => read_latest(ctx, "/ts/"++ids++"/latest", xargs)
  | [_, _, _, "ts", ids, "first", n, ...xargs] => read_first(ctx, "/ts/"++ids++"/first/"++n, n, xargs)
  | [_, _, _, "ts", ids, "earliest", ...xargs] => read_earliest(ctx, "/ts/"++ids++"/earliest", xargs)
  | [_, _, _, "ts", ids, "length"] => length(ctx)
  | [_, _, _, "ts", ids, "memory", "length"] => length_in_memory(ctx)
  | [_, _, _, "ts", ids, "index", "length"] => length_of_index(ctx)
  | [_, _, _, "ts", "sync"] => timeseries_sync(ctx)
  | _ => Http_response.bad_request(~content="Error:unknown path\n", ())
  }
};

let handle_req_worker = (ctx, req, body) => {
  let meth = req |> Request.meth;
  let uri_path = req |> Request.uri |> Uri.to_string;
  let path_list = String.split_on_char('/', uri_path);
  switch (meth) {
  | `GET => get_req(ctx, path_list, uri_path);
  | _ => Http_response.bad_request(~content="Error:unknown method\n", ())
  }
};

let handle_req_safe = (ctx, req, body) => {
  () => Lwt.catch(
    () => handle_req_worker(ctx, req, body),
    fun
    | Failure(m) => Http_response.bad_request(~content=Printf.sprintf("Error:%s\n",m), ())
    | e => Lwt.fail(e)
  );
};

let handle_req = (ctx, req, body) => {
  Lwt_mutex.with_lock(ctx.m, handle_req_safe(ctx, req, body))
};


let server (~ctx) = {
  let callback = (_conn, req, body) => handle_req(ctx, req, body);
  let http = `TCP(`Port(http_port^));
  let https = `TLS((`Crt_file_path(cert_file^), `Key_file_path(key_file^), `No_password, `Port(http_port^)));
  Server.create(~mode=(tls_mode^ ? https : http), Server.make(~callback, ()));
};


let register_signal_handlers = () => {
  Lwt_unix.(on_signal(Sys.sigterm, (_) => raise(Interrupt("Caught SIGTERM"))) |> 
      _ => on_signal(Sys.sighup, (_) => raise(Interrupt("Caught SIGHUP"))) |> 
      _ => on_signal(Sys.sigint, (_) => raise(Interrupt("Caught SIGINT"))));
};

let parse_cmdline = () => {
  let usage = "usage: " ++ Sys.argv[0];
  let speclist = [
    (
      "--cert-file",
      Arg.Set_string(cert_file),
      ": to provide the TLS certificate"
    ),
    (
      "--key-file",
      Arg.Set_string(key_file),
      ": to provide the TLS key"
    ),
    (
      "--http-port",
      Arg.Set_int(http_port),
      ": to set the http port"
    ), 
    ("--enable-debug", Arg.Set(log_mode), ": turn debug mode on"), 
    ("--enable-tls", Arg.Set(tls_mode), ": use https"),
    (
      "--backend-uri-list",
      Arg.Set_string(backend_uri_list),
      ": to provide the location of nibbledb server"
    ), 

  ];
  Arg.parse(speclist, x => raise(Arg.Bad("Bad argument : " ++ x)), usage);
};

let enable_debug = () => {
  Lwt_log_core.default :=
    Lwt_log.channel(
      ~template="$(date).$(milliseconds) [$(level)] $(message)",
      ~close_mode=`Keep,
      ~channel=Lwt_io.stdout,
      ()
    );
  Lwt_log_core.add_rule("*", Lwt_log_core.Debug);
};

let init = () => {
  let () = ignore(register_signal_handlers());
  parse_cmdline();
  log_mode^ ? enable_debug() : ();
  { db: Backend.create(~backend_uri_list=backend_uri_list^),
    m: Lwt_mutex.create()
  };
};

let flush_server = (ctx) => {
  Lwt_main.run {
    Lwt_io.printf("\nShutting down server...\n") >>=
      () => Backend.flush(ctx.db) >>=
        () => Lwt_unix.sleep(1.0) >>=
          () => Lwt_io.printf("OK\n")
  };
};

let run_server = (~ctx) => {
  let () = {
    try (Lwt_main.run(server(~ctx))) {
    | Interrupt(_) => ignore(flush_server(ctx));
    };
  };
};

run_server(~ctx=init());

