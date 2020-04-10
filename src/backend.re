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

let flatten(data) {
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

let sort_by_timestamp_worker(x,y) {
  open Ezjsonm;
  let dict_x = get_dict(x);
  let dict_y = get_dict(y);
  let ts_x = List.assoc("timestamp", dict_x);
  let ts_y =  List.assoc("timestamp", dict_y);
  let ts = get_float(ts_x);
  let ts' = get_float(ts_y);
  ts < ts' ? 1 : (-1)
}

let sort_by_timestamp(lis) {
  List.sort((x,y) => sort_by_timestamp_worker(x,y), lis)
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

let length = (~ctx) => {
  Lwt.return(0)
}

let length_in_memory = (~ctx) => {
  Lwt.return(0)
}

let length_of_index = (~ctx) => {
  Lwt.return(0)
}

let read_last_worker(uri) {
  Lwt_io.printf("accessing backend uri:%s\n", uri) >>=
    () => Net.get(~uri) >|= Ezjsonm.from_string;
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

let aggregate(data, ~args) {
  data;  
}

let read_last = (~ctx, ~path, ~n, ~args) => {
  let (filtered_path, xarg_path) = get_path_from_args(args);
  Lwt_list.map_p(host => read_last_worker(String.trim(host)++path++filtered_path), ctx.backend_uri_list) >|=
    flatten >|= sort_by_timestamp >|= take(n) >|= aggregate(~args=xarg_path) >|= Ezjsonm.list(x=>x)
}

let read_latest = (~ctx) => {
  Lwt.return(empty_data);
}










