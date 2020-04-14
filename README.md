# Gateway for nibbledb

Early POC but will add features such as dynamically adding and removing backends for scaleability.

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
docker run -p 5000:5000 --rm -it --network="nibble" jptmoore/nibblegw /home/nibble/nibblegw --backend-uri-list "http://foo:8000, http://bar:8000"
```

### Post some data

```bash
curl -k --request POST --data '[{"value": 1}, {"value": 2}, {"value": 3}, {"value": 4}, {"value": 5}]' http://localhost:5000/ts/foo
curl -k --request POST --data '[{"value": 6}, {"value": 7}, {"value": 8}, {"value": 9}, {"value": 10}]' http://localhost:5000/ts/foo
curl -k --request POST --data '[{"value": 11}, {"value": 12}, {"value": 13}, {"value": 14}, {"value": 15}]' http://localhost:5000/ts/foo
```

### Run a query

```bash
curl http://localhost:5000/ts/foo/length
```

```json
{"length":15}
```