# Gateway for nibbledb

A frontend gateway to scale nibbledb.

### Create a docker network to use

```bash
docker network create nibble
```

### Start one backend

```bash
docker run --rm -it --name foo --network="nibble" jptmoore/nibbledb /home/nibble/nibbledb
```

### Start another backend

```bash
docker run --rm -it --name bar --network="nibble" jptmoore/nibbledb /home/nibble/nibbledb
```

### Start the gateway

```bash
docker run -p 5000:5000 --rm -it --network="nibble" jptmoore/nibblegw /home/nibble/nibblegw --hosts "http://foo:8000, http://bar:8000"
```

### Post some data

```bash
curl -k --request POST --data '[{"value": 1}, {"value": 2}, {"value": 3}, {"value": 4}, {"value": 5}]' http://localhost:5000/ts/foo
curl -k --request POST --data '[{"value": 6}, {"value": 7}, {"value": 8}, {"value": 9}, {"value": 10}]' http://localhost:5000/ts/foo
curl -k --request POST --data '[{"value": 11}, {"value": 12}, {"value": 13}, {"value": 14}, {"value": 15}]' http://localhost:5000/ts/foo
```

The data is distributed across the two backends.


### Run a query

```bash
curl http://localhost:5000/ts/foo/length
```

```json
{"length":15}
```

### Add more backend hosts

```bash
curl -X POST 'http://localhost:5000/ctl/host/add' -d '[{"host":"http://foo:8000"}]'
```
### List the backend hosts

```bash
curl 'http://localhost:5000/ctl/host/list'
```

### Get the number of backend hosts

```bash
curl 'http://localhost:5000/ctl/host/count'
```

### Get stats on the backend servers

```bash
curl http://localhost:5000/info/ts/stats
```

```json
[
    {
        "http://foo:8000/info/ts/stats": [
            {
                "length": [
                    {
                        "foo": 10
                    }
                ]
            },
            {
                "length_in_memory": [
                    {
                        "foo": 10
                    }
                ]
            },
            {
                "length_on_disk": [
                    {
                        "foo": 0
                    }
                ]
            },
            {
                "length_of_index": [
                    {
                        "foo": 0
                    }
                ]
            }
        ]
    },
    {
        "http://bar:8000/info/ts/stats": [
            {
                "length": [
                    {
                        "foo": 5
                    }
                ]
            },
            {
                "length_in_memory": [
                    {
                        "foo": 5
                    }
                ]
            },
            {
                "length_on_disk": [
                    {
                        "foo": 0
                    }
                ]
            },
            {
                "length_of_index": [
                    {
                        "foo": 0
                    }
                ]
            }
        ]
    }
]
```

Take a look at at the [readme](https://github.com/jptmoore/nibbledb) for other commands you can use.