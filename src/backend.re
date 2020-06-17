open Lwt.Infix;

type t = {
  mutable backend_uri_list: list(string),
  mutable backend_count: int
};

type xargs =
  | Standard(string)
  | Filtered(string)
  | FilteredAggregated(string, string)
  | Aggregated(string)
  | Error(string);

let flatten_data = (data) => {
  open List;
  fold_left((acc, x) => 
    rev_append(Ezjsonm.get_list(x=>x, x), acc), [], data)
}

let take = (n, lis) => {
  open List;
  let rec loop = (n, acc, l) =>
    switch (l) {
    | [] => acc
    | [_, ..._] when n == 0 => acc
    | [xs, ...rest] => loop(n - 1, cons(xs, acc), rest)
    };
  rev(loop(n, [], lis));
};

let sort_by_timestamp_worker(x,y,direction) {
  open Ezjsonm;
  let ts = get_float(find(x, ["timestamp"]));
  let ts' = get_float(find(y, ["timestamp"]));
  switch direction {
  | `Last => ts < ts' ? 1 : (-1)
  | `First => ts > ts' ? 1 : (-1)
  }
}

let sort_by_timestamp(lis, ~direction) {
  List.sort((x,y) => sort_by_timestamp_worker(x,y,direction), lis)
}

let create = (~backend_uri_list) => {
  if (backend_uri_list == "") {
    {
      backend_uri_list: [],
      backend_count: 0
    }
  } else {
    let hosts = String.split_on_char(',', backend_uri_list) |>
      List.map(host => String.trim(host));
    { 
      backend_uri_list: hosts,
      backend_count: List.length(hosts)
    }
  }
};

let process_args(args) {
  open String;
  switch (args) {
    | [] => Standard("");
    | ["filter", name, "equals", value] => Filtered("/filter/"++name++"/equals/"++value);
    | ["filter", name, "equals", value, "min"] => FilteredAggregated("/filter/"++name++"/equals/"++value, "/min");
    | ["filter", name, "equals", value, "max"] => FilteredAggregated("/filter/"++name++"/equals/"++value, "/max");
    | ["filter", name, "equals", value, "sum"] => FilteredAggregated("/filter/"++name++"/equals/"++value, "/sum");
    | ["filter", name, "equals", value, "median"] => FilteredAggregated("/filter/"++name++"/equals/"++value, "/median");
    | ["filter", name, "equals", value, "count"] => FilteredAggregated("/filter/"++name++"/equals/"++value, "/count");
    | ["filter", name, "contains", value] => Filtered("/filter/"++name++"/contains/"++value);
    | ["filter", name, "contains", value, "min"] => FilteredAggregated("/filter/++name++/contains/"++value, "/min");
    | ["filter", name, "contains", value, "max"] => FilteredAggregated("/filter/++name++/contains/"++value, "/max");
    | ["filter", name, "contains", value, "sum"] => FilteredAggregated("/filter/++name++/contains/"++value, "/sum");
    | ["filter", name, "contains", value, "median"] => FilteredAggregated("/filter/++name++/contains/"++value, "/median");
    | ["filter", name, "contains", value, "count"] => FilteredAggregated("/filter/++name++/contains/"++value, "/count");
    | ["min"] => Aggregated("/min");
    | ["max"] => Aggregated("/max");
    | ["sum"] => Aggregated("/sum");
    | ["median"] => Aggregated("/median");
    | ["count"] => Aggregated("/count");
    | _ => Error("invalid path");
    }
}


let get_path_from_args(args) {
  switch (process_args(args)) {
    | Standard(path) => (path, "");
    | Filtered(path) => (path, "");
    | FilteredAggregated(path, xargs) => (path, xargs);
    | Aggregated(xargs) => ("", xargs);
    | Error(m) => failwith(m)
    }
}


let count = (data) => {
  open Ezjsonm;
  let count = float_of_int(List.length(data));
  dict([("count", `Float(count))]);
};

let get_value(x) {
  open Ezjsonm;
  get_float(find(x, ["data", "value"]));
}

let sum = (data) => {
  open List;
  let lis = map(x => get_value(x), data);
  let sum = fold_left((+.), 0., lis);
  Ezjsonm.dict([("sum", `Float(sum))]);
};

let apply_aggregate_data(data, name, fn) {
  open Ezjsonm;
  // remove empty results
  let filtered = List.filter(x => x != dict([]), data);
  if (List.length(filtered) == 0) {
    dict([]);
  } else {
    List.map(x => get_value(x), filtered) |> 
      Array.of_list |> fn |>
        result => dict([(name, `Float(result))]);
  }
}

let aggregate_data(data, ~args) {
  open Oml.Util.Array;
  open Oml.Statistics.Descriptive;  
  switch args {
  | "/min" => apply_aggregate_data(data, "min", min)
  | "/max" => apply_aggregate_data(data, "max", max)
  | "/sum" => sum(data)
  | "/median" => apply_aggregate_data(data, "median", median)
  | "/count" => count(data)
  | _ => failwith("invalid aggregate function")
  }
}

let apply_aggregate_aggregate_data(data, name, fn) {
  open Ezjsonm;
  // remove empty results
  let filtered = List.filter(x => x != dict([]), data);
  if (List.length(filtered) == 0) {
    dict([]);
  } else {
    List.map(x => get_float(find(x, [name])), filtered) |>
      Array.of_list |> fn |>
        result => dict([(name, `Float(result))]);
  }
}


let aggregate_aggregate_data(data, ~arg) {
  open Oml.Util.Array;
  open Oml.Statistics.Descriptive;
  switch arg {
  | "/sum" => apply_aggregate_aggregate_data(data, "sum", sumf)
  | "/min" => apply_aggregate_aggregate_data(data, "min", min)
  | "/max" => apply_aggregate_aggregate_data(data, "max", max)
  | "/median" => apply_aggregate_aggregate_data(data, "median", median)
  | "/count" => apply_aggregate_aggregate_data(data, "count", sumf)
  | "/length" => apply_aggregate_aggregate_data(data, "length", sumf)
  | _ => failwith("invalid aggregate function")
  }
}

let get(uri) {
  Lwt_io.printf("getting from backend uri:%s\n", uri) >>=
    () => Net.get(~uri) >|= Ezjsonm.from_string;
}

let get_with_host(uri) {
  open Ezjsonm;
  Lwt_io.printf("getting from backend uri:%s\n", uri) >>=
    () => Net.get(~uri) >|= 
      from_string >|= x => dict([(uri, x)])
}

let get_no_response(uri) {
  Lwt_io.printf("getting from backend uri:%s\n", uri) >>=
    () => Net.get(~uri);
}

let post_no_response(uri, ~payload) {
  Lwt_io.printf("posting to backend uri:%s\n", uri) >>=
    () => Net.post(~uri, ~payload);
}

let delete_no_response(uri) {
  Lwt_io.printf("deleting from backend uri:%s\n", uri) >>=
    () => Net.delete(~uri);
}

let length = (~ctx, ~path) => {
  Lwt_list.map_p(host => get(host++path), ctx.backend_uri_list) >|= 
    aggregate_aggregate_data(~arg="/length");
}

let length_in_memory = (~ctx, ~path) => {
  length(ctx, path)
}

let length_of_index = (~ctx, ~path) => {
  length(ctx, path)
}

let random_host = (ctx, path) => {
  if (ctx.backend_count == 0) {
    failwith("no backends hosts defined")
  } else {
    let n = Random.int(ctx.backend_count);
    List.nth(ctx.backend_uri_list, n) ++ path
  }
}

let post = (~ctx, ~path, ~payload) => {
  post_no_response(random_host(ctx, path), payload) >>=
    // in case later we decide to return a payload
    data => Lwt.return_unit
}

let flush = (~ctx, ~path) => {
  Lwt_list.map_p(host => get_no_response(host++path), ctx.backend_uri_list) >>=
    // in case later we decide to return a payload
    data => Lwt.return_unit
}

let read_n = (ctx, path, n, args, direction) => {
  let (filter_path, xarg_path) = get_path_from_args(args);
  let data = Lwt_list.map_p(host => get(host++path++filter_path), ctx.backend_uri_list) >|=
    flatten_data >|= sort_by_timestamp(~direction) >|= take(n);
  if (xarg_path == "") {
    data >|= Ezjsonm.list(x=>x);
  } else {
    data >|= aggregate_data(~args=xarg_path);
  }
}

let read_last = (~ctx, ~path, ~n, ~args) => {
  read_n(ctx, path, n, args, `Last)
}

let read_latest = (~ctx, ~path, ~args) => {
  read_n(ctx, path, 1, args, `Last);
}

let read_first = (~ctx, ~path, ~n, ~args) => {
  read_n(ctx, path, n, args, `First)
}

let read_earliest = (~ctx, ~path, ~args) => {
  read_n(ctx, path, 1, args, `First);
}



let read_since_range = (~ctx, ~path, ~xargs) => {
  let (filter_path, xarg_path) = get_path_from_args(xargs);
  let data = Lwt_list.map_p(host => get(host++path++filter_path++xarg_path), ctx.backend_uri_list);
  if (xarg_path == "") {
    data >|= flatten_data >|= sort_by_timestamp(~direction=`Last) >|= Ezjsonm.list(x=>x);
  } else {
    data >|= aggregate_aggregate_data(~arg=xarg_path)
  }
}


let read_since = (~ctx, ~path, ~xargs) => {
  read_since_range(ctx, path, xargs)
}

let read_range = (~ctx, ~path, ~xargs) => {
  read_since_range(ctx, path, xargs)
}

let delete_since_range = (~ctx, ~path, ~xargs) => {
  let (filter_path, xarg_path) = get_path_from_args(xargs);
  if (xarg_path == "") {
    Lwt_list.map_p(host => delete_no_response(host++path++filter_path), ctx.backend_uri_list) >>=
      data => Lwt.return_unit;
  } else {
    failwith("invalid path")
  }
}

let delete_since = (~ctx, ~path, ~xargs) => {
  delete_since_range(ctx, path, xargs)
}

let delete_range = (~ctx, ~path, ~xargs) => {
  delete_since_range(ctx, path, xargs)
}

let ts_names_value = (x) => {
  open Ezjsonm;
  get_strings(find(x, ["timeseries"]));
}

let names = (~ctx, ~path) => {
  open Ezjsonm;
  Lwt_list.map_p(host => get(host++path), ctx.backend_uri_list) >|=
    List.fold_left((acc, x) => 
      List.rev_append(ts_names_value(x), acc), []) >|=
        List.sort_uniq((x,y) => compare(x,y)) >|=
          x => dict([("timeseries", strings(x))])
}

let stats = (~ctx, ~path) => {
  Lwt_list.map_p(host => get_with_host(host++path), ctx.backend_uri_list) >|= 
    Ezjsonm.list(x=>x)
}

let status = (~ctx, ~path) => {
  Lwt_list.map_p(host => get_with_host(host++path), ctx.backend_uri_list) >|= 
    Ezjsonm.list(x=>x)
}

let health_check = (~ctx) => {
  Lwt_list.map_p(host => Net.status(~uri=host++"/info/status"), ctx.backend_uri_list) >|=
    List.filter(status => (status == 200)) >|=
      List.length >|= n => ctx.backend_count == n
}

let validate_host = (~json) => {
  open Ezjsonm;
  switch (get_dict(value(json))) {
  | [("host",`String host)] => 
      Some(host)
  | _ => None;
  }
};

let add_host = (~ctx, ~host) => {
  open Ezjsonm;
  Lwt_io.printf("got:%s\n", host) >>= () =>
  Net.status(~uri=host++"/info/status") >|=
    status => if (status == 200) {
      open List;
      if (exists(x=> x == host, ctx.backend_uri_list)) {
        failwith("host already exists")
      } else {
        ctx.backend_uri_list = cons(host, ctx.backend_uri_list);
        ctx.backend_count = ctx.backend_count + 1;
      }
    } else {
      failwith("failed to add host")
    }
}