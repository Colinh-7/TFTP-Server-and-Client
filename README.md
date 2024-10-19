# TFTP Server with Select and Multi-threading Support 

This project implements a TFTP server that utilizes the `select` system call for handling multiple connections and supports multi-threading for parallel request processing.

## Table of Contents
- [Select Implementation](#select)
- [Multi-threading](#multi-threading)
- [Client Request Spam Testing](#client-request-spam-testing)
- [Commands](#commands)

---

## Select

All changes related to the `select` functionality can be found in the `sock.c` file. 

To start the server, run the following commands:

- **Compile the server:**
  ```bash
  make
  ```

- **Run the server:**
  ```bash
  ./bin/tftp --mode SRV --port 6999
  ```

You can launch multiple commands simultaneously by running two client instances (in different directories to avoid file concurrency issues).

### Commands to launch a client sending requests to port 6999:
```bash
./bin/tftp --mode CLT --port 6999
```

---

## Multi-threading

For this feature, we introduced a "Service" structure that manages different threads handling incoming requests.

The server maintains a list of services. When it receives a request, it passes it to one of its available services, which then takes over and processes the request. This allows our server to handle requests in parallel.

To prevent concurrency issues, we have assigned a mutex to each file available at the root of the server. All these mutexes are stored in an AVL tree for faster lookup.

### AVL Tree Structure

Here’s a schematic representation of our AVL tree:

```
                Node
          +-------------+
          |   Mutex     |
          |             |
          |   File      |
          +-------------+
          /           \
      Node             Node
+-------------+     +-------------+
|   Mutex     |     |   Mutex     |
|             |     |             |
|   File      |     |   File      |
+-------------+     +-------------+
   /      \               /     \
Node     Node         Node      Node
```

When a service processes a request, it locks the mutex associated with the corresponding file name. Once the request is completed, it releases the mutex.

---

## Client Request Spam Testing

To test our server, we created a Bash script that launches random requests in the background.

You can use this script if you wish (run.sh).

### How to execute:
```bash
./run.sh [number of clients sending random requests simultaneously] [PORT]
```

### Example:
```bash
./run.sh 3 6999
```
In this example, three clients will send RRQ or WRQ requests randomly at the same time.

---

## Commands

- **Compile the server:**
  ```bash
  make
  ```

- **Run the server:**
  ```bash
  ./bin/tftp --mode SRV --port 6999
  ```
Made with Bryan C.
