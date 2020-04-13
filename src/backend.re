open Lwt.Infix;

type t = {
  backend_uri_list: list(string),
  backend_count: int
};

type xargs =
  | Standard(string)
  | Filtered(string)
  | FilteredAggregated(string, string)
  | Aggregated(string)
  | Error(string);

let flatten_data(data) {
  open Ezjsonm;
  let rec loop = (acc, l) => {
    switch (l) {
      | [] => acc;
      | [x, ...xs] => loop(List.rev_append(get_list(x=>x, x), acc), xs);
      }
  };
  loop([], data);
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

let length_of_index = (~ctx) => {
  Lwt.return(0)
}

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

let apply_aggregate_data(data, name, fn) {
  open Ezjsonm;
  let lis = List.map(x=> get_value(x), data);
  lis |> Array.of_list |> fn |>
    result => dict([(name, `Float(result))]);
}

let aggregate_data(data, ~args) {
  open Oml.Util.Array;
  open Oml.Statistics.Descriptive;  
  switch args {
  | "/min" => apply_aggregate_data(data, "min", min)
  | "/max" => apply_aggregate_data(data, "max", max)
  | "/sum" => apply_aggregate_data(data, "sum", sumf)
  | "/median" => apply_aggregate_data(data, "median", median)
  | "/count" => count(data)
  | _ => failwith("invalid aggregate function")
  }
}

let apply_aggregate_aggregate_data(data, name, fn) {
  open Ezjsonm;
  List.map(x => get_float(find(x, [name])), data) |>
    Array.of_list |> fn |>
      result => dict([(name, `Float(result))]);
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
  Lwt_io.printf("accessing backend uri:%s\n", uri) >>=
    () => Net.get(~uri) >|= Ezjsonm.from_string;
}

let length = (~ctx, ~path) => {
  Lwt_list.map_p(host => get(String.trim(host)++path), ctx.backend_uri_list) >|= 
    aggregate_aggregate_data(~arg="/length");
}

let length_in_memory = (~ctx, ~path) => {
  length(ctx, path)
}

let read_n = (ctx, path, n, args, direction) => {
  let (filter_path, xarg_path) = get_path_from_args(args);
  let data = Lwt_list.map_p(host => get(String.trim(host)++path++filter_path), ctx.backend_uri_list) >|=
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
  read_n(ctx, path, n, args, `Last)
}

let read_earliest = (~ctx, ~path, ~args) => {
  read_n(ctx, path, 1, args, `Last);
}



let read_since_range = (~ctx, ~path, ~xargs) => {
  let (filter_path, xarg_path) = get_path_from_args(xargs);
  let data = Lwt_list.map_p(host => get(String.trim(host)++path++filter_path++xarg_path), ctx.backend_uri_list);
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



