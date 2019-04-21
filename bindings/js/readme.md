### JS Bindings

## Trying it out

1. Clone the parent folder recursively.
2. `./generate_sample.sh` (Generate some sample data)
3. `cd bindings/js`
4. `npm install && npm start`

5. Make a POST request to the password endpoint with a password:

```
curl -X POST http://localhost:3000/check/password \
  -H 'Content-Type: application/json' \
  -d '{ "password": "test1" }'
```

6. Or make a GET request to the hash endpoint with a hash:

```
curl http://localhost:3000/check/b444ac06613fc8d63795be9ad0beaf55011936ac
```

(Note: `b444ac06613fc8d63795be9ad0beaf55011936ac` is the sha1 hash of test1)