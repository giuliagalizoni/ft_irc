**Branch:** `giulia_first_draft`

## What's working
- TCP socket created, bound to port 6667 (just for testing), listening
- `poll()` loop running — handles 1 client at a time -- for now I'm using a hardcoded `fds[2]` array, where `fds[0]` is the listening socket and `fds[1]` is the client socket; in the future will change to vector container
- Accepts incoming connection with `accept4()` (non-blocking) -- I haven't figure out if we can use fcntl or if it's just for mac as per the subject; it seems that fcntl is better practice, but gotta check 42 constraints
- Reads client message with `recv`, prints it to stdout
- Sends back `"Hello client\r\n"` reply

## Next steps
- Move to dynamic `fds` vector for multiple clients
- Parse `argv` for port + password

## How to run
```bash
c++ main.cpp -o server
./server
```
In a second terminal:
```bash
nc localhost 6667
```
Then type a message and hit enter — you should see it echoed in the server terminal and receive `Hello client` back.

## Command processing
- Generic command parser implemented
- Commands currently implemented: PASS, NICK, USER, JOIN, PRIVMSG

## Registration
- Password validation against server password
- Registration completed wehn PASS+NICK+USER have been recieved
- Registration announcement triggered only once

## Channels
- Channel creation on first JOIN
- Operator assigned to first user joining the channel
- JOIN notifications broadcasted to all channel members

## Messaging
- Channel messages: PRIVMSG #channel :message
- Direct messages: PRIVMSG nickname :message
- Sender excluded from broadcast
- Nickname uniqueness validation implemented

## How to run
./ircserv 6667 pass

In another terminal: nc -C localhost 6667

## Tests
Registration
    PASS pass
    NICK charlie
    USER charlie 0 * : Charles Chaplin

Channel join
    JOIN #test

Channel message
    PRIVMSG #test :hello everyone

Direct message
    PRIVMSG Bob :hello privately