_This project has been created as part of the 42 curriculum by dagimeno and pavicent and amonteag._ 

# webserv

Project `webserv`: minimal HTTP server in C++ (42 project). Designed to serve static files, handle CGI, accept uploads and support configuration via a file.

# Description

`webserv` is an educational C++ web server implementation covering the fundamentals of:
- HTTP request/response handling
- Serving static content
- Executing CGI scripts
- Handling uploads (`upload`) and deletions (`DELETE`)
- Configuration via `default.conf`

The goal is to provide hands-on understanding of I/O, event multiplexing, child process management for CGI, and HTTP request parsing.

# Instructions

## Requirements
- A C++ compiler (the project uses `c++` with standard `c++98`).
- Build tools: `make`.

On Debian/Ubuntu:

```
sudo apt update
sudo apt install build-essential
```

## Installation
Clone the repository and enter the project directory:

```
git clone <repository>
cd webserv
```

## Compilation
Build with `make` (the project includes a `Makefile`):

```
make
```

The produced executable is `webserv`.

## Usage
Start the server providing the configuration file (the repository includes `default.conf`):

```
./webserv default.conf
```

Notes:
- If you encounter unexpected exceptions when running, check and adapt the paths in `default.conf` so they point to existing locations on your machine.
- Keep the server running in one terminal and use another terminal to run client requests.

### Example test commands
- Basic GET requests:

```
curl -v http://127.0.0.1:9999/
curl -v http://127.0.0.1:9998/
curl -v http://127.0.0.1:8080/
```

- Test a CGI script:

```
curl -v http://127.0.0.1:9999/cgi-bin/test.py
```

- Simple POST (form-like):

```
curl -v -X POST http://127.0.0.1:9999/upload -d 'hola=123'
```

- DELETE (your `delete-outside` location):

```
curl -v -X DELETE http://127.0.0.1:9999/delete-outside
```

## Automated tests
Remove old artifacts before running tests if needed:

```
rm -f www/jfercode/upload/newfile_test.txt www/jfercode/upload/expect.txt www/jfercode/upload/bin.bin
```

Run the included test scripts:

```
bash tests/post_tests.sh
bash tests/chunked_test.sh
bash tests/cgi_post_tests.sh
bash tests/stress_concurrency.sh
```

Check which process listens on the port (example: `9999`):

```
sudo lsof -i :9999
```

# Resources
- Sample configuration: [default.conf](default.conf)
- Test scripts: [tests/](tests/)
- Main source files in [srcs/](srcs/) and headers in [inc/](inc/)

### Notes about CGI and uploads
- For CGI tests, use the scripts under `www/*/cgi-bin/` (for example `www/eg_delacruz/cgi-bin/test.py`).
- Make sure the paths declared in `default.conf` exist on your machine before testing (see "Usage").

---

If you want, I can:
- Run the automated tests and collect results.
- Adjust `default.conf` for your local environment.
- Add a short debugging section with `gdb` or `valgrind` commands.

Original instructions file: [Instrucciones.txt](Instrucciones.txt)
